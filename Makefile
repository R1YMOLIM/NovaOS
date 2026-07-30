# Makefile root
# names for C subdirectories
SUBDIRS = boot kernel

# tool for C
CC = clang

# export tool for all C modules
export CC

BUILD_DIR = build

all: setup $(SUBDIRS) 
	@echo "--- All modules built successfully! ---"

# Create Build directory 
setup:
	@mkdir -p $(BUILD_DIR)

# Templete for build C modules
$(SUBDIRS):
	@echo "--- Build C-module: $@ ---"
	$(MAKE) -C $@

# Generation LSP for C
lsp_build:
	@echo "--- LSP for C module: $@ ---"
	$(MAKE) -C boot lsp_build
	$(MAKE) -C kernel lsp_build

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

.PHONY: all setup $(SUBDIRS) lsp_build run clean
