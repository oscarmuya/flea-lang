#include "../include/arena.h"
#include <stdio.h>
#include <string.h>

// A minimal, zero-dependency testing framework using macros
int tests_run = 0;
int tests_failed = 0;

#define test_assert(message, test)                                             \
  do {                                                                         \
    tests_run++;                                                               \
    if (!(test)) {                                                             \
      printf("  ❌ FAIL: %s (Line %d)\n", message, __LINE__);                  \
      tests_failed++;                                                          \
    } else {                                                                   \
      printf("  ✅ PASS: %s\n", message);                                      \
    }                                                                          \
  } while (0)

// --- Your Test Functions ---

void test_arena_allocation(void) {
  printf("\nRunning Arena Tests:\n");

  // Setup (Example: change to match your actual arena API)
  Arena arena = arena_init(1024 * 1024);

  void *ptr1 = arena_alloc(&arena, 32);
  test_assert("Allocation should return a non-null pointer", ptr1 != NULL);

  void *ptr2 = arena_alloc(&arena, 64);
  test_assert("Successive allocations should yield distinct addresses",
              ptr1 != ptr2);

  arena_free(&arena);
}

int main(void) {
  printf("=== STARTING UNIT TESTS ===\n");

  test_arena_allocation();
  // Add more test functions here as your project grows

  printf("\n=== TEST SUMMARY ===\n");
  printf("Total tests run: %d\n", tests_run);
  if (tests_failed > 0) {
    printf("❌ FAILURE: %d test(s) failed!\n", tests_failed);
    return 1; // Exit code 1 alerts your Makefile that tests failed
  }

  printf("🎉 SUCCESS: All tests passed!\n");
  return 0;
}
