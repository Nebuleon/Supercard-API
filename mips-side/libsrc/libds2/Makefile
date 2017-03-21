ifeq ($(strip $(SCDS2_TOOLCHAIN)),)
$(error "Please set SCDS2_TOOLCHAIN in your environment. export SCDS2_TOOLCHAIN=<path to scds2 toolchain>")
endif
ifeq ($(strip $(SCDS2_TOOLS)),)
$(error "Please set SCDS2_TOOLS in your environment. export SCDS2_TOOLS=<path to scds2 tools>")
endif

PREFIX         := $(SCDS2_TOOLCHAIN)/bin/mipsel-scds2-elf-

CC       := $(PREFIX)gcc
AS       := $(PREFIX)as
AR       := $(PREFIX)ar
OBJCOPY  := $(PREFIX)objcopy
STRIP    := $(PREFIX)strip
NM       := $(PREFIX)gcc-nm
RANLIB   := $(PREFIX)gcc-ranlib

INCLUDES := -I../../include -iquote. -Ilibfat/include -iquotelibfat/source

SOURCE_DIRS := . libc ds2_ds libfat/source stdio_libfat

CFLAGS   := -Wall -G 0 -O2 -fomit-frame-pointer -ffunction-sections \
            $(INCLUDES)

# Force the use of the memcpy function inside the standard library itself,
# to reuse its code in the instruction cache.
CFLAGS   += -mmemcpy

# Compile libfat for the DSTwo
CFLAGS   += -DSCDS2

ASFLAGS  := -G 0 $(INCLUDES)

CFILES   := $(foreach dir, $(SOURCE_DIRS), $(wildcard $(dir)/*.c))
SFILES   := $(foreach dir, $(SOURCE_DIRS), $(wildcard $(dir)/*.S))

OBJECTS  := $(CFILES:.c=.o) $(SFILES:.S=.o)

.PHONY: all clean install

all: libds2.a

libds2.a: $(OBJECTS)
	@rm -f libds2.a
	@$(AR) rcs libds2.a $(OBJECTS)

clean:
	@echo clean ...
	@rm -f libds2.a $(OBJECTS)

install:
	-mkdir $(SCDS2_TOOLS)/lib
	cp libds2.a $(SCDS2_TOOLS)/lib

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.S
	$(CC) $(ASFLAGS) -D__ASSEMBLY__ -c $< -o $@
