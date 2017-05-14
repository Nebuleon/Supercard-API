ifeq ($(strip $(SCDS2_TOOLS)),)
$(error "Please set SCDS2_TOOLS in your environment. export SCDS2_TOOLS=<path to scds2 tools>")
endif

# Making this for the native host
PREFIX         :=

export CC      := $(PREFIX)gcc
export LD      := $(PREFIX)gcc
export AS      := $(PREFIX)as
export AR      := $(PREFIX)ar
export OBJCOPY := $(PREFIX)objcopy
export STRIP   := $(PREFIX)strip
export NM      := $(PREFIX)gcc-nm
export RANLIB  := $(PREFIX)gcc-ranlib

# Flags for compiling C code
CFLAGS         +=

# Linker flags
LDFLAGS        +=
LIBPATHS       +=
LIBS           +=

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

TARGET   := filter

INCLUDES :=

SOURCE_DIRS := .

CFLAGS   += -Wall -O2 -fomit-frame-pointer -Wextra -Wpedantic --std=c99 \
            $(INCLUDES)

CFILES   := $(foreach dir, $(SOURCE_DIRS), $(wildcard $(dir)/*.c))

OBJECTS  := $(CFILES:.c=.o)

.PHONY: all clean install

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(LD) $(LDFLAGS) $^ $(LIBPATHS) $(LIBS) -o $@

clean:
	-rm -f $(TARGET) $(OBJECTS)

install: all
	-mkdir $(SCDS2_TOOLS)/tools
	cp $(TARGET) $(SCDS2_TOOLS)/tools
