ifeq ($(strip $(SCDS2_TOOLCHAIN)),)
$(error "Please set SCDS2_TOOLCHAIN in your environment. export SCDS2_TOOLCHAIN=<path to scds2 toolchain>")
endif
ifeq ($(strip $(SCDS2_TOOLS)),)
$(error "Please set SCDS2_TOOLS in your environment. export SCDS2_TOOLS=<path to scds2 tools>")
endif

.PHONY: all libsrc clean install

all: scds2_start.o libsrc

scds2_start.o: scds2_start.S
	$(SCDS2_TOOLCHAIN)/bin/mipsel-scds2-elf-gcc -g -Iinclude -c $< -o $@

libsrc:
	$(MAKE) -C libsrc

install: all
	cp -a scds2_start.S scds2_start.o scds2_rules scds2.specs scds2.ld Makefile $(SCDS2_TOOLS)
	-rm -rf $(SCDS2_TOOLS)/include $(SCDS2_TOOLS)/libsrc $(SCDS2_TOOLS)/examples
	cp -ar include libsrc examples $(SCDS2_TOOLS)
	-mkdir $(SCDS2_TOOLS)/tools
	cp -a tools/makeplug $(SCDS2_TOOLS)/tools
	$(MAKE) -C libsrc install

clean:
	rm scds2_start.o
	rm -rf lib
	$(MAKE) -C libsrc clean
