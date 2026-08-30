# ==========================================
# Toolchain & Configuration
# ==========================================
CC       := gcc
TARGET   := flea

SRC_DIR  := src
INC_DIR  := include
OBJ_DIR  := build

# Default compilation flags (Common to both Dev and Release)
CFLAGS   := -std=c99 -Wall -Wextra -Wpedantic -Wshadow -Wconversion \
            -Wpointer-arith -Wstrict-prototypes

CPPFLAGS := -I$(INC_DIR) -MMD -MP
SRCS     := $(wildcard $(SRC_DIR)/*.c)
OBJS     := $(SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
DEPS     := $(OBJS:.o=.d)

# ==========================================
# Build Profiles (Dev vs Release)
# ==========================================
# Default Profile: Development
# Optimized for debugging, fast compile times, and error catching
DEBUG_FLAGS := -g -O0 -fsanitize=address,undefined -DDEV_BUILD

# Release Profile: Production
# Optimized for maximum execution speed and minimal binary size
# -O3: Aggressive performance optimizations
# -DNDEBUG: Disables assert() macros to maximize speed
RELEASE_FLAGS := -O3 -DNDEBUG

# Default to debug mode unless 'release' is explicitly invoked
PROFILE_FLAGS := $(DEBUG_FLAGS)

.PHONY: all clean run test release

all: $(TARGET)

# Dynamic profile switching for production build
release: PROFILE_FLAGS := $(RELEASE_FLAGS)
release: clean $(TARGET)
	@echo "------------------------------------------------"
	@echo " Production binary '$(TARGET)' built successfully!"
	@echo " Optimized with -O3, Sanitizers stripped."
	@echo "------------------------------------------------"

# Link step
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(PROFILE_FLAGS) $^ -o $@

# Compile step
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) $(PROFILE_FLAGS) $(CPPFLAGS) -c $< -o $@

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

# ==========================================
# Automation & Testing
# ==========================================
run: $(TARGET)
	./$(TARGET) examples/main.fl


# Run both unit tests and integration language tests
test: $(TARGET)
	@# 1. Compile and run C Unit Tests
	$(CC) $(CFLAGS) $(SANFLAGS) $(CPPFLAGS) tests/unit_tests.c src/arena.c src/store.c -o build/unit_tester
	@./build/unit_tester
	
	@# 2. Run Python integration test runner against your .fl files
	@python3 tests/run_tests.py

clean:
	rm -rf $(OBJ_DIR) $(TARGET)

-include $(DEPS)

