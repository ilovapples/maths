#include "arena/arena.h"

#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>

typedef struct ArenaBucket {
	uint8_t *base;
	struct ArenaBucket *next;
} ArenaBucket;

// this could technically be done with just a pointer to the last bucket,
// given it's circularly linked, but I think this is clearer.
typedef struct Arena {
	ArenaBucket *first;
	ArenaBucket *current;
	size_t index;
	size_t bucket_init_size;
} Arena;


Arena *arena_make(size_t bucket_init_size)
{
	Arena *ret = calloc(1, sizeof(Arena));
	ret->bucket_init_size = bucket_init_size;

	ret->current = calloc(1, sizeof(ArenaBucket));
	ret->current->base = malloc(ret->bucket_init_size);
	ret->first = ret->current;

	ret->index = 0;

	return ret;
}

void free_arena_buckets(ArenaBucket *first);

void arena_destroy(Arena *arena)
{
	free_arena_buckets(arena->first);
	free(arena);
}

void free_arena_buckets(ArenaBucket *first)
{
	if (first == NULL) return;

	ArenaBucket *temp, *cur = first;
	do
	{
		temp = cur->next;
		free(cur->base);
		free(cur);
		cur = temp;
	} while (cur != NULL);
}

#define MAX(a,b) ((a<b) ? b : a)

#define ARENA_REALLOC_FACTOR 2
inline void *arena_alloc(Arena *arena, size_t size, size_t align)
{
	if (align != 0)
		arena->index += (align - (arena->index % align)); // round index so 'align' is fulfilled and zig doesn't flag it as UB
	void *ret_ptr = &arena->current->base[arena->index];

	if (arena->index + size > arena->bucket_init_size)
	{
		// current bucket is full; allocate a new one
		ArenaBucket *new_bucket = calloc(1, sizeof(ArenaBucket));

		new_bucket->base = malloc(MAX(arena->bucket_init_size, size));

		arena->current = arena->current->next = new_bucket;

		arena->index = size;
		return arena->current->base;
	}
	arena->index += size;

	return ret_ptr;
}

