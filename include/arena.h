#ifndef FLEA_ARENA_H
#define FLEA_ARENA_H

#include <stddef.h>
#include <stdint.h>

typedef struct {
  uint8_t *buffer;
  size_t capacity;
  size_t offset;
} Arena;

Arena arena_init(size_t capacity);

void *arena_alloc(Arena *arena, size_t size);

void *arena_realloc(Arena *arena, void *ptr, size_t old_size, size_t new_size);

void arena_reset(Arena *arena);

void arena_free(Arena *arena);

#endif
