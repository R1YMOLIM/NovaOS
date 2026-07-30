#kernel/Makefile
LD = ld.lld

CFLAGS = \
    -ffreestanding \
    -fno-stack-protector \
    -fno-builtin \
    -fno-pic \
    -fno-pie \
    -mno-red-zone \
    -mcmodel=kernel \
    -m64 \
    -Wall \
    -Wextra \
    -c -I ../include/
LDFLAGS = \
    -T linker.ld \

# Directories and files
BUILD_ROOT = ../build
BUILD_DIR  = $(BUILD_ROOT)
TARGET     = $(BUILD_DIR)/kernel
SRC        = main_microkernel.c

# Object files
OBJ        = $(BUILD_ROOT)/main_microkernel.o

# Default target
all: $(TARGET)

# Compile C source directly into the build directory
$(OBJ): $(SRC)
	@mkdir -p $(BUILD_ROOT)
	$(CC) $(CFLAGS) $< -o $@

# Link object file into executable
$(TARGET): $(OBJ)
	@mkdir -p $(BUILD_DIR)
	$(LD) $(LDFLAGS) $< -o $@

lsp_build:
	$(CC) $(CFLAGS) $(SRC) -o /dev/null

.PHONY: all lsp_build
