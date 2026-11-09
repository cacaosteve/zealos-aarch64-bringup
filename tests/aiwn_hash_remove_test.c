/* Exercise native table unlinking independently of the guest ABI. */
#include "aiwn_hash.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void *__AIWNIOS_CAlloc(int64_t n, void *task) {
  (void)task;
  return calloc(1, (size_t)n);
}
void __AIWNIOS_Free(void *p) { free(p); }
#ifdef __aarch64__
CHash *ZcHashFindFallback(char *name, CHashTable *table, int64_t mask,
                          int64_t instance) {
  (void)name; (void)table; (void)mask; (void)instance;
  abort();
}
#endif

static CHash *entry(const char *s, int32_t type) {
  CHash *h = calloc(1, sizeof(*h));
  assert(h);
  h->str = strdup(s);
  assert(h->str);
  h->type = type;
  return h;
}

int main(void) {
  CHashTable *parent = HashTableNew(1, NULL);
  CHashTable *child = HashTableNew(1, NULL);
  CHash *inherited = entry("same", 1);
  CHash *older = entry("same", 1);
  CHash *other = entry("collision", 1);
  CHash *newer = entry("same", 2);
  child->next = parent;
  HashAdd(inherited, parent);
  HashAdd(older, child);
  HashAdd(other, child);
  HashAdd(newer, child);

  /* The supplied type mask selects the newer entry, but identity must match. */
  older->type = (int32_t)UINT32_C(0x01000003);
  assert(!HashRemDel(older, child, 1));
  assert(newer->use_cnt == 1 && older->use_cnt == 0);
  assert(child->body[0] == newer && newer->next == other && other->next == older);
  assert(HashRemDel(older, child, 2));
  assert(child->body[0] == newer && newer->next == other && !other->next);

  /* Searches never fall through to the parent; a missing instance is inert. */
  assert(!HashRemDel(inherited, child, 1));
  assert(parent->body[0] == inherited && inherited->use_cnt == 0);
  assert(!HashRemDel(newer, child, 2));
  assert(!HashRemDel(newer, child, 0));
  assert(!HashRemDel(NULL, child, 1));
  assert(HashRemDel(newer, child, 1));
  assert(child->body[0] == other);
  assert(HashRemDel(other, child, 1));
  assert(!child->body[0]);
  assert(HashRemDel(inherited, parent, 1));
  HashTableDel(child);
  HashTableDel(parent);
  puts("native HashRemDel identity, instance, type and unlink checks pass");
  return 0;
}
