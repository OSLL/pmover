DEFINES   += -DCONFIG_ARM -DCONFIG_HAS_TLS
ARCH_BITS := 32

CC 	:= arm-linux-gnueabi-gcc
LD 	:= arm-linux-gnueabi-ld
OBJCOPY := arm-linux-gnueabi-objcopy

CFLAGS += -Wno-error=int-to-pointer-cast -Wno-error=pointer-to-int-cast