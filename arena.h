// Simple arena allocator - v1.2

#ifndef ARENA_H_
#define ARENA_H_

#include <stdlib.h>
#include <string.h>

typedef struct {
	unsigned char* base;
	unsigned char* head;
	unsigned char* last;
	size_t capacity;
} Arena;

Arena arena_create(size_t capacity);
void* arena_alloc(Arena* arena, size_t bytes);
void* arena_realloc(Arena* arena, void* ptr, size_t current, size_t target);
unsigned char* arena_checkpoint(Arena* arena);
void arena_rollback(Arena* arena, unsigned char* ptr);
void arena_destroy(Arena* arena);

#endif

#ifdef ARENA_IMPLEMENTATION

Arena arena_create(size_t capacity)
{
	void* ptr = malloc(capacity);
	if (ptr == NULL)
		return (Arena) {0};
	
	return (Arena) {
		.capacity = capacity,
		.base = ptr,
		.head = ptr,
		.last = ptr
	};
}

void* arena_alloc(Arena* arena, size_t bytes)
{
	if (arena == NULL || arena->base == NULL || bytes == 0)
		return NULL;

	if (bytes > arena->capacity - (arena->head - arena->base))
		return NULL;

	arena->last = arena->head;
	arena->head += bytes;
	return arena->last;
}

void* arena_realloc(Arena* arena, void* ptr, size_t current, size_t target)
{
	if (arena == NULL || arena->base == NULL || target == 0)
		return NULL;

	size_t bytes_used = arena->head - arena->base;
	if (ptr == arena->last) {
		int size_diff = target - current;
		if (size_diff < 0 || size_diff > arena->capacity - bytes_used)
			return NULL;

		arena->head += size_diff;
		return ptr;
	}

	if (target > arena->capacity - (bytes_used))
		return NULL;

	arena->last = arena->head;
	arena->head += target;
	if (ptr != NULL && current > 0)
		memcpy(arena->last, ptr, current);
	return arena->last;
}

unsigned char* arena_checkpoint(Arena* arena)
{
	return arena->head;
}

void arena_rollback(Arena* arena, unsigned char* checkpoint)
{
	if (arena->base >= checkpoint && checkpoint >= arena->base + arena->capacity)
		arena->head = checkpoint;
}

void arena_destroy(Arena* arena) 
{
	if (arena != NULL && arena->base != NULL)
		free(arena->base);
}

#endif
