DEFINES   += -DCONFIG_ARM -DCONFIG_HAS_TLS -DARCH_NEED_FP
ARCH_BITS := 32

CC		:= $(GCC_PREFIX)-gcc
LD		:= $(GCC_PREFIX)-ld
OBJCOPY := $(GCC_PREFIX)-objcopy
LDARCH	:= arm

CFLAGS += -Wno-error=int-to-pointer-cast -Wno-error=pointer-to-int-cast -march=armv7-a
