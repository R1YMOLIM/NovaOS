# Makefile root
# names for C subdirectories
SUBDIRS = boot

# Directory for final build
BUILD_DIR = build

all: setup $(SUBDIRS) kernel_rust
	@echo "--- All modules built successfully! ---"

# Create Build directory 
setup:
	@mkdir -p $(BUILD_DIR)

# Templete for build C modules
$(SUBDIRS):
	@echo "--- Build C-module: $@ ---"
	$(MAKE) -C $@

# Build kernel rust
kernel_rust:
	@echo "--- Build Rust Kernel ---"
	cargo build --release
	@echo "--- Copying kernel to build directory ---"
	cp target/x86_64-unknown-none/release/kernel $(BUILD_DIR)/

# Generation LSP for C
lsp:
	@echo "--- Generating compile_commands.json ---"
	-@bear -- $(MAKE) all || compiledb $(MAKE) all

run: all
	@echo "--- Starting QEMU ---"
	$(MAKE) -C boot run

clean:
	@echo "--- Cleaning project ---"
	rm -rf $(BUILD_DIR)
	rm -rf target
	@for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir clean; \
	done

.PHONY: all setup $(SUBDIRS) kernel_rust lsp run clean
