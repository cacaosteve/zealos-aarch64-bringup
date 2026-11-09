#include "aiwn_hash.h"
#include "aiwn_mem.h"
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#ifdef __aarch64__
extern CHash *ZcHashFindFallback(char *str, CHashTable *table, int64_t type,
                                int64_t inst);
#endif
int64_t HashStr(char *str) {
  /* Match ZealOS SYS_HASH_STR so compiler and guest tables use the same
   * buckets. Unsigned arithmetic preserves the original 64-bit carry and
   * wraparound without signed-overflow undefined behavior. */
  uint64_t res = 0;
  if (!str)
    return 0;
  while (*str) {
    uint64_t carry = res >> 63;
    res = (res << 1) + (unsigned char)*str++ + carry;
  }
  return (int64_t)(res + (res >> 16) + ((res >> 15) & 1));
}

void HashDel(CHash *h) {
#ifdef AIWN_BOOTSTRAP
  // TODO
  A_FREE(h->str);
  A_FREE(h);
#else
// TODO
#endif
}

CHashTable *HashTableNew(int64_t sz, void *task) {
  CHash **body = A_CALLOC(sz * sizeof(CHash *), task);
  CHashTable *ret = A_CALLOC(sizeof(CHashTable), task);
  ret->body = body;
  ret->mask = sz - 1;
  return ret;
}

CHash *HashFind(char *str, CHashTable *table, int64_t type, int64_t inst) {
  if (!table)
    return NULL;
#ifdef __aarch64__
  /* The ARM64 guest-task bridge can receive a ZealOS task-table mirror whose
   * bucket body is a guest address, not a host-mapped pointer. Let the runtime
   * resolve those lookups through the active managed compiler table. */
  if ((uintptr_t)table < UINT64_C(0x100000000) || !table->body ||
      (uintptr_t)table->body < UINT64_C(0x100000000))
    return ZcHashFindFallback(str, table, type, inst);
#endif
  int64_t b = HashStr(str) & table->mask;
  CHash *h;
  for (h = table->body[b]; h; h = h->next) {
    if (!strcmp(str, h->str))
      if (h->type & type)
        if (--inst == 0)
          return h;
  }
  if (table->next)
    return HashFind(str, table->next, type, inst);
  return NULL;
}
// Doesn't check ->next
CHash *HashSingleTableFind(char *str, CHashTable *table, int64_t type,
                           int64_t inst) {
  int64_t b = HashStr(str) & table->mask;
  CHash *h;
  for (h = table->body[b]; h; h = h->next) {
    if (!strcmp(str, h->str))
      if (h->type & type)
        if (--inst == 0)
          return h;
  }
  return NULL;
}
CHash **HashBucketFind(char *str, CHashTable *table) {
  int64_t b = HashStr(str) & table->mask;
  return &table->body[b];
}

void HashAdd(CHash *h, CHashTable *table) {
  CHash **b = HashBucketFind(h->str, table);
  h->next = *b;
  *b = h;
}

int64_t HashRemDel(CHash *h, CHashTable *table, int64_t inst) {
  /* ZealOS selects an instance by name and non-flag type bits in this
   * table alone, then removes it only if it is the exact supplied entry. */
  if (!h || !h->str || !table || !table->body || inst <= 0)
    return 0;
  uint32_t types = (uint32_t)h->type & UINT32_C(0x00ffffff);
  CHash **link = HashBucketFind(h->str, table);
  for (CHash *cur = *link; cur; link = &cur->next, cur = *link) {
    if (!((uint32_t)cur->type & types) || strcmp(cur->str, h->str))
      continue;
    if (--inst)
      continue;
    cur->use_cnt = (int32_t)((uint32_t)cur->use_cnt + 1u);
    if (cur != h)
      return 0;
    *link = cur->next;
    HashDel(cur);
    return 1;
  }
  return 0;
}

void HashTableDel(CHashTable *table) {
  int64_t i;
  CHash *next, *cur;
  for (i = 0; i != table->mask + 1; i++) {
    for (cur = table->body[i]; cur; cur = next) {
      next = cur->next;
      HashDel(cur);
    }
  }
  A_FREE(table->body);
  A_FREE(table);
}
