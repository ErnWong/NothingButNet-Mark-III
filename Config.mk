#
# Project directories
#

BINDIR = $(ROOT)/bin
SRCDIR = $(ROOT)/src
INCDIR = $(ROOT)/include
FIRMDIR = $(ROOT)/firmware
TESTDIR = $(ROOT)/tests
SRCDIR_TEST = $(ROOT)/tests
BINDIR_TEST = $(ROOT)/tests/bin
LIBDIR_TEST = $(ROOT)/tests/libtap
SRCDIR_SIZE = $(ROOT)/tests
BINDIR_SIZE = $(ROOT)/tests/bin

LIBSRC_TEST = $(LIBDIR_TEST)/tap.c
LIBOBJ_TEST = $(BINDIR_TEST)/tap.o

SUBDIRS = $(SRCDIR)


#
# Files
#

ASMEXT = s
CEXT = c
CPPEXT = cpp
HEXT = h
OEXT = o
CEXT_TEST = test.c
OEXT_TEST = test.o
CEXT_SIZE = size.c
OEXT_SIZE = size.o

OUTBIN = output.bin
OUTNAME = output.elf
OUTNAME_SIZE = size$(EXESUFFIX)


#
# Device
#

# Makefile for IFI VeX Cortex Microcontroller (STM32F103VD series)
DEVICE = VexCortex

# Libraries to include in the link (use -L and -l) e.g. -lm, -lmyLib
LIBRARIES = $(FIRMDIR)/libccos.a -lgcc -lm

# Prefix for ARM tools (must be on the path)
MCUPREFIX = arm-none-eabi-
# Assembler
MCUAFLAGS = -mthumb -mcpu=cortex-m3 -mlittle-endian
# Compiler
MCUCFLAGS = -mthumb -mcpu=cortex-m3 -mlittle-endian
# Linker
MCULFLAGS = -nostartfiles -Wl,-static -Bfirmware -Wl,-u,VectorTable -Wl,-T -Xlinker $(FIRMDIR)/cortex.ld
# Prepares the elf file by converting it to a binary that java can write
MCUPREPARE = $(OBJCOPY) $(OUT) -O binary $(BINDIR)/$(OUTBIN)

# Advanced sizing flags
SIZEFLAGS =
# Uploads program using java
UPLOAD = @java -jar $(FIRMDIR)/uniflash.jar vex $(BINDIR)/$(OUTBIN)



#
# Program Flags
#

AFLAGS   := $(MCUAFLAGS)
ARFLAGS  := $(MCUCFLAGS)
CCFLAGS  := -c -Wall $(MCUCFLAGS) -Os -ffunction-sections -fsigned-char -fomit-frame-pointer -fsingle-precision-constant
CFLAGS   := $(CCFLAGS) -std=gnu99 -Werror=implicit-function-declaration
CPPFLAGS := $(CCFLAGS) -fno-exceptions -fno-rtti -felide-constructors
LDFLAGS  := -Wall $(MCUCFLAGS) $(MCULFLAGS) -Wl,--gc-sections

CFLAGS_TEST := -c -Wall -std=gnu99 -Werror=implicit-function-declaration
LDFLAGS_TEST := -Wall -Wl,--gc-sections
CFLAGS_SIZE := $(CFLAGS_TEST)
LDFLAGS_SIZE := $(LDFLAGS_TEST)


#
# Tools
#

AR := $(MCUPREFIX)ar
AS := $(MCUPREFIX)as
CC := $(MCUPREFIX)gcc
CPPCC := $(MCUPREFIX)g++
OBJCOPY := $(MCUPREFIX)objcopy
CC_TEST := gcc
CC_SIZE := gcc
