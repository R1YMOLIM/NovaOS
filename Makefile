# Makefile root
SUBDIRS = boot

all:
	@for dir in $(SUBDIRS); do \
		echo "--- Build module: $$dir ---"; \
		$(MAKE) -C $$dir; \
	done

lsp:
	@echo "--- Generating compile_commands.json ---"
	-@bear -- $(MAKE) all || compiledb $(MAKE) all

run:
	@echo "--- Starting QEMU ---"
	$(MAKE) -C boot run

clean:
	@for dir in $(SUBDIRS); do \
		echo "--- Clean module: $$dir ---"; \
		$(MAKE) -C $$dir clean; \
	done

.PHONY: all run clean $(SUBDIRS)
