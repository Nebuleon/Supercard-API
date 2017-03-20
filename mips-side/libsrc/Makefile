ifeq ($(strip $(SCDS2_TOOLCHAIN)),)
$(error "Please set SCDS2_TOOLCHAIN in your environment. export SCDS2_TOOLCHAIN=<path to scds2 toolchain>")
endif
ifeq ($(strip $(SCDS2_TOOLS)),)
$(error "Please set SCDS2_TOOLS in your environment. export SCDS2_TOOLS=<path to scds2 tools>")
endif

.PHONY: all libds2 zlib clean lib

all: libds2 zlib

libds2:
	$(MAKE) -C libds2

zlib:
	$(MAKE) -C zlib -f Makefile.ds2

install: all | lib
	$(MAKE) -C libds2 install
	$(MAKE) -C zlib -f Makefile.ds2 install

clean:
	$(MAKE) -C libds2 clean
	$(MAKE) -C zlib -f Makefile.ds2 clean

lib:
	-mkdir ../lib
