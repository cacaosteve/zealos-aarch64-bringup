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
  int64_t res = 5381;
  while (*str)
    res += (res << 5) + res + *str++;
  return res;
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

static int64_t _HashDelStr(char *str, CHashTable *table, int64_t inst) {
  CHash **b = HashBucketFind(str, table), *h2;
  CHash *prev = NULL;
  for (h2 = *b; h2; h2 = h2->next) {
    if (!strcmp(h2->str, str)) {
      if (!--inst) {
        if (prev)
          prev->next = h2->next;
        HashDel(h2);
        return 1;
      }
    }
  next:
    prev = h2;
  }
  return 0;
}

int64_t HashRemDel(CHash *h, CHashTable *table, int64_t inst) {
  int64_t res = _HashDelStr(h->str, table, inst);
  HashAdd(h, table);
  return res;
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
