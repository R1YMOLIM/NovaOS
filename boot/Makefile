# boot/Makefile
# Toolchain definition
CC      = clang
LD      = lld-link

# Compilation and linking flags
CFLAGS  = -target x86_64-unknown-windows -ffreestanding -fshort-wchar -mno-red-zone -c
LDFLAGS = -flavor link -entry:efi_main -subsystem:efi_application -dll

# Directories and files
BUILD_ROOT = ../build
BUILD_DIR  = $(BUILD_ROOT)/EFI/BOOT
TARGET     = $(BUILD_DIR)/BOOTX64.EFI
SRC        = main_boot.c

# Object and UEFI variable files
OBJ        = $(BUILD_ROOT)/main_boot.o
VARS       = $(BUILD_ROOT)/my_vars.fd

# Find OVMF paths safely
OVMF_DIR       := $(shell for d in /usr/share/OVMF /usr/share/ovmf /usr/share/OVMF/x64 /usr/share/ovmf/x64; do if [ -d $$d ]; then echo $$d; break; fi; done)
OVMF_CODE_PATH := $(shell find $(OVMF_DIR) -name "OVMF_CODE*.fd" 2>/dev/null | head -n 1)
OVMF_VARS_PATH := $(shell find $(OVMF_DIR) -name "OVMF_VARS*.fd" 2>/dev/null | head -n 1)

# Default target
all: $(TARGET)

# Compile C source directly into the build directory
$(OBJ): $(SRC)
	@mkdir -p $(BUILD_ROOT)
	$(CC) $(CFLAGS) $< -o $@

# Link object file into EFI executable
$(TARGET): $(OBJ)
	@mkdir -p $(BUILD_DIR)
	$(LD) $(LDFLAGS) $< -out:$@

# Prepare local variable file inside the build directory and start QEMU
run: all
	@if [ -z "$(OVMF_CODE_PATH)" ] || [ -z "$(OVMF_VARS_PATH)" ]; then \
		echo "=========================================================="; \
		echo " Error: Components OVMF UEFI not found on your system!!"; \
		echo " Please, install that:"; \
		echo "   sudo apt update && sudo apt install ovmf"; \
		echo "=========================================================="; \
		exit 1; \
	fi
	@if [ ! -f $(VARS) ]; then \
		cp "$(OVMF_VARS_PATH)" $(VARS); \
		chmod +w $(VARS); \
	fi
	qemu-system-x86_64 \
		-drive if=pflash,format=raw,readonly=on,file=$(OVMF_CODE_PATH) \
		-drive if=pflash,format=raw,file=$(VARS) \
		-drive file=fat:rw:$(BUILD_ROOT),format=raw \
		-net none

# Clean wipes the entire build folder
clean:
	rm -rf $(BUILD_ROOT)

.PHONY: all run clean

