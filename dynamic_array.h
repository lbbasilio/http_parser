// Simple dynamic array - v0.1
//
// Since this file only defines macros, any struct with the fields
// items: array of pointer of items
// count: number of items
// capacity: size of the items array
// fields may be used instead of the Dynamic_Array

#ifndef DYNAMIC_ARRAY_H_
#define DYNAMIC_ARRAY_H_

#ifndef DA_INIT_CAP
#define DA_INIT_CAP 64
#endif // DA_INIT_CAP

#include "arena.h"
#include <stddef.h>

typedef struct {
	const void** items;
	size_t count;
	size_t capacity;
} Dynamic_Array;

#define da_append(da, item, arena) \
	do { \
		if ((da)->count >= (da)->capacity) { \
			(da)->capacity = (da)->capacity == 0 ? DA_INIT_CAP : (da)->capacity * 2; \
			(da)->items = arena_realloc(arena, (da)->items, ((da)->capacity >> 1) * sizeof(*(da)->items), (da)->capacity * sizeof(*(da)->items)); \
		} \
		(da)->items[(da)->count++] = item; \
	} while (0)

#endif // DYNAMIC_ARRAY_H_

#ifdef DYNAMIC_ARRAY_IMPLEMENTATION
#endif // DYNAMIC_ARRAY_IMPLEMENTATION
