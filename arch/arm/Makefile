SYS-DEF		:= $(SRC_DIR)/include/syscall.def
SYS-ASM-COMMON	:= syscall-common.S
SYS-TYPES	:= $(SRC_DIR)/include/syscall-types.h

SYS-CODES	:= $(SRC_DIR)/include/syscall-codes.h
SYS-PROTO	:= $(SRC_DIR)/include/syscall.h

SYS-ASM		:= syscalls.S
SYS-GEN		:= $(SRC_DIR)/gen-syscalls.pl

SYS-OBJ		:= syscalls.o

CFLAGS		+= -c -fpie -Wstrict-prototypes -Wa,--noexecstack -D__ASSEMBLY__ -nostdlib -fomit-frame-pointer -I$(shell pwd)

.DEFAULT_GOAL	:= arm

$(SYS-ASM): $(SYS-GEN) $(SYS-DEF) $(SYS-ASM-COMMON) $(SYS-TYPES)
	$(E) "  GEN     " $@
	$(Q) perl			\
		$(SYS-GEN)		\
		$(SYS-DEF)		\
		$(SYS-CODES)		\
		$(SYS-PROTO)		\
		$(SYS-ASM)		\
		$(SYS-ASM-COMMON)	\
		$(SYS-TYPES)		\
		$(ARCH_BITS)

%.o: %.S
	$(E) "  CC      " $@
	$(Q) $(CC) $(CFLAGS)  $^ -o $@

arm: $(SYS-OBJ) parasite-head.o

clean:
	$(E) "  CLEAN SYSCALLS"
	$(Q) $(RM) -f $(SYS-ASM)
	$(Q) $(RM) -f $(SYS-CODES)
	$(Q) $(RM) -f $(SYS-PROTO)
	$(Q) $(RM) -f $(SYS-OBJ)

.PHONY: clean arm
