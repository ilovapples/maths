#ifndef ARENA__ARENA_H
#define ARENA__ARENA_H

#include <stddef.h>
#include <stdint.h>

#include "cpp_compat.h"

MML__CPP_COMPAT_BEGIN_DECLS

typedef struct Arena Arena;

// bucket_init_size is the number of bytes to allocate
// for each bucket, if a given allocation won't fit in
// the current bucket but would fit in a new one
Arena *arena_make(size_t bucket_init_size);
void arena_destroy(Arena *arena);

void *arena_alloc(Arena *arena, size_t size, size_t align);

#define arena_alloc_T(_a, _n, _T) ((_T *)arena_alloc((_a), (_n)*sizeof(_T), _Alignof(_T)))

MML__CPP_COMPAT_END_DECLS

#endif /* ARENA__ARENA_H */
