#
# Common definitions
#

ifeq ($(strip $(V)),)
	E = @echo
	Q = @
else
	E = @\#
	Q =
endif
export E Q

FIND		:= find
CSCOPE		:= cscope
TAGS		:= ctags
RM		:= rm
HEXDUMP		:= hexdump
ECHO		:= echo
NM		:= nm
AWK		:= awk
SH		:= sh
MAKE		:= make

uname_M         := $(shell uname -m)

# Additional ARCH settings for x86
ARCH ?= $(shell echo $(uname_M) | sed -e s/i.86/i386/ -e s/sun4u/sparc64/ \
                  -e s/arm.*/arm/ -e s/sa110/arm/ \
                  -e s/s390x/s390/ -e s/parisc64/parisc/ \
                  -e s/ppc.*/powerpc/ -e s/mips.*/mips/ \
                  -e s/sh[234].*/sh/ )

no-deps-targets := tags cscope clean cleanout cleandep realclean

CFLAGS		+= -I./include -fno-strict-aliasing

LIBS		+= -lrt -lpthread -lprotobuf-c

DEFINES		+= -D_FILE_OFFSET_BITS=64
DEFINES		+= -D_GNU_SOURCE

ifneq ($(WERROR),0)
	WARNINGS += -Werror
endif

ifeq ($(DEBUG),1)
	DEFINES += -DCR_DEBUG
	CFLAGS	+= -O0 -ggdb3
else
	CFLAGS	+= -O2
endif

ifeq ($(STATIC),1)
	LINKFLAGS += -static
endif

ifeq ($(DEBUG_RESTORE),1)
	DEFINES += -DCONFIG_DEBUG_RESTORE
endif

WARNINGS	+= -Wall
ROOT		?= $(shell pwd)

ARCH_DIR        := $(ROOT)/arch/$(ARCH)

include $(ARCH_DIR)/Makefile.inc

CFLAGS		+= $(WARNINGS) $(DEFINES) -I$(ARCH_DIR)

export ROOT CC LD ECHO MAKE CFLAGS LIBS ARCH DEFINES
