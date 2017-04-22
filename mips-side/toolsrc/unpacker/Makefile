ifeq ($(strip $(SCDS2_TOOLS)),)
$(error "Please set SCDS2_TOOLS in your environment. export SCDS2_TOOLS=<path to scds2 tools>")
endif
ifeq ($(strip $(SCDS2_TOOLCHAIN)),)
$(error "Please set SCDS2_TOOLCHAIN in your environment. export SCDS2_TOOLCHAIN=<path to scds2 toolchain>")
endif

PREFIX         := $(SCDS2_TOOLCHAIN)/bin/mipsel-scds2-elf-

export CC      := $(PREFIX)gcc
export LD      := $(PREFIX)gcc
export AS      := $(PREFIX)as
export AR      := $(PREFIX)ar
export OBJCOPY := $(PREFIX)objcopy
export STRIP   := $(PREFIX)strip
export NM      := $(PREFIX)gcc-nm
export RANLIB  := $(PREFIX)gcc-ranlib

# Flags for compiling C code
CFLAGS         += -I../../include -G 0 -mno-abicalls -mno-long-calls -DSCDS2

# Flags for assembling assembly code
ASFLAGS        += -I../../include -G 0 -DSCDS2

# Linker flags
LDFLAGS        += -specs=unpacker.specs -T unpacker.ld -Wl,--gc-sections
LIBPATHS       += -L../../libsrc/libds2
LIBS           += -lgcc -lds2

%.dat: %.elf
	$(OBJCOPY) -x -O binary $< $@

%.elf:
	$(LD) $(LDFLAGS) $^ $(LIBPATHS) $(LIBS) -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.S
	$(CC) -D__ASSEMBLY__ $(ASFLAGS) -c $< -o $@

NAME     := unpacker

TARGET   := $(NAME).dat

INCLUDES := -I. -I../../libsrc/zlib

# Optimise for size; build fixed Huffman trees at run time (if they're even
# needed); and don't use any CRC-32 code
CFLAGS   += -Wall -Os -fomit-frame-pointer -ffunction-sections \
            -DBUILDFIXED -DNO_GZIP $(INCLUDES)

ASFLAGS  += $(INCLUDES)

CFILES   := main.c ../../libsrc/zlib/adler32.c ../../libsrc/zlib/inffast.c \
            ../../libsrc/zlib/inflate.c ../../libsrc/zlib/inftrees.c \
            ../../libsrc/zlib/zutil.c

OBJECTS  := main.o adler32.o inffast.o inflate.o inftrees.o zutil.o

START    := start.o

.PHONY: all clean install

all: $(TARGET)

$(TARGET): $(NAME).elf

$(NAME).elf: $(OBJECTS) | $(START)

# This Makefile is using C sources from the directory used to make libz.a, but
# compiles them with different flags. As a result, each in-tree object file is
# specified on its own and marked as dependent upon its out-of-tree C source
# file. The recipe is also duplicated in each entry because, for some reason,
# the %.o: %.c recipe is not used otherwise.

adler32.o: ../../libsrc/zlib/adler32.c
	$(CC) $(CFLAGS) -c $< -o $@

crc32.o: ../../libsrc/zlib/crc32.c
	$(CC) $(CFLAGS) -c $< -o $@

inffast.o: ../../libsrc/zlib/inffast.c
	$(CC) $(CFLAGS) -c $< -o $@

inflate.o: ../../libsrc/zlib/inflate.c
	$(CC) $(CFLAGS) -c $< -o $@

inftrees.o: ../../libsrc/zlib/inftrees.c
	$(CC) $(CFLAGS) -c $< -o $@

zutil.o: ../../libsrc/zlib/zutil.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	-rm -f $(TARGET) $(START) $(NAME).elf $(OBJECTS)

install: all
	-mkdir $(SCDS2_TOOLS)/tools
	cp $(TARGET) $(SCDS2_TOOLS)/tools
