#include "../include/arena.h"
#include <stdlib.h>
#include <string.h>

// This is our Arena allocator that we use for memory management
// in the AST and also some part of our interpreter INFO: compiled to bytecode
// soon

Arena arena_init(size_t capacity) {
  Arena arena;
  arena.buffer = malloc(capacity);
  arena.capacity = capacity;
  arena.offset = 0;
  return arena;
}

void *arena_alloc(Arena *arena, size_t size) {
  // Standard alignment (usually 8 bytes for 64-bit systems)
  size_t align = sizeof(void *);

  // Calculate the next aligned offset
  size_t current_ptr = (size_t)arena->buffer + arena->offset;
  size_t aligned_ptr = (current_ptr + align - 1) & ~(align - 1);
  size_t new_offset = aligned_ptr - (size_t)arena->buffer;

  // Check for out-of-memory errors
  if (new_offset + size > arena->capacity) {
    return NULL; // Out of memory
  }

  // Move the offset forward and return the pointer
  void *ptr = &arena->buffer[new_offset];
  arena->offset = new_offset + size;
  return ptr;
}

void *arena_realloc(Arena *arena, void *ptr, size_t old_size, size_t new_size) {
  if (ptr == NULL) {
    return arena_alloc(arena, new_size);
  }

  if (new_size <= old_size) {
    return ptr; // shrinking or same size, nothing to do
  }

  // Check if ptr is the most recent allocation:
  // it must sit exactly at (arena->offset - old_size)
  uint8_t *block_end = (uint8_t *)ptr + old_size;
  uint8_t *arena_end = arena->buffer + arena->offset;

  if (block_end == arena_end) {
    // It's the last allocation, we can grow in place
    size_t extra = new_size - old_size;
    if (arena->offset + extra > arena->capacity) {
      return NULL; // out of memory
    }
    arena->offset += extra;
    return ptr;
  }

  // Not the last allocation, fall back to alloc + copy
  void *new_ptr = arena_alloc(arena, new_size);
  if (new_ptr == NULL) {
    return NULL;
  }
  memcpy(new_ptr, ptr, old_size);
  return new_ptr;
}

// Free everything at once
void arena_reset(Arena *arena) { arena->offset = 0; }

// Release OS memory
void arena_free(Arena *arena) {
  free(arena->buffer);
  arena->buffer = NULL;
  arena->capacity = 0;
  arena->offset = 0;
}
