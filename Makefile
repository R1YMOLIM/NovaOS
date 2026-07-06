# Makefile root
SUBDIRS = boot

all:
	@for dir in $(SUBDIRS); do \
		echo "--- Build module: $$dir ---"; \
		$(MAKE) -C $$dir; \
	done

run:
	@echo "--- Starting QEMU ---"
	$(MAKE) -C boot run

clean:
	@for dir in $(SUBDIRS); do \
		echo "--- Clean module: $$dir ---"; \
		$(MAKE) -C $$dir clean; \
	done

.PHONY: all run clean $(SUBDIRS)
