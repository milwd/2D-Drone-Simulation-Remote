# Compiler and flags
CC = gcc
CFLAGS = -Wall -g
LDFLAGS = -lrt -lncurses

# Directories
SRC_DIR = src
BIN_DIR = bins
OBJ_DIR = src

# Find all source files
SRC_FILES = $(wildcard $(SRC_DIR)/*.c)
# Derive object files names from the source files
OBJ_FILES = $(SRC_FILES:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

# Output binary name
TARGET = $(BIN_DIR)/my_program

# Create necessary directories
$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

# Default target
all: $(TARGET)

# Compile source files into object files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@
	
# Link the object files to create the final binary
$(TARGET): $(OBJ_FILES) | $(BIN_DIR)
	$(CC) $(OBJ_FILES) -o $(TARGET) $(LDFLAGS)



# Clean up
clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

# Phony targets
.PHONY: all clean
