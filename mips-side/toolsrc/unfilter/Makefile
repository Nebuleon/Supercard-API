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
LDFLAGS        += -specs=unfilter.specs -T unfilter.ld -Wl,--gc-sections
LIBPATHS       += -L../../libsrc/libds2
LIBS           += -lgcc -lds2

%.dat: %.elf
	$(OBJCOPY) -x -O binary $< $@
	chmod a-x $@

%.elf:
	$(LD) $(LDFLAGS) $^ $(LIBPATHS) $(LIBS) -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.S
	$(CC) -D__ASSEMBLY__ $(ASFLAGS) -c $< -o $@

NAME     := unfilter

TARGET   := $(NAME).dat

INCLUDES := -I.

CFLAGS   += -Wall -O2 -fomit-frame-pointer -ffunction-sections \
            $(INCLUDES)

ASFLAGS  += $(INCLUDES)

CFILES   := main.c

OBJECTS  := main.o

START    := start.o

.PHONY: all clean install

all: $(TARGET)

$(TARGET): $(NAME).elf

$(NAME).elf: $(OBJECTS) | $(START)

clean:
	-rm -f $(TARGET) $(START) $(NAME).elf $(OBJECTS)

install: all
	-mkdir $(SCDS2_TOOLS)/tools
	cp $(TARGET) $(SCDS2_TOOLS)/tools
