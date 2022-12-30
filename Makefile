SRC = $(wildcard *.c)
ASM = $(wildcard *.s)
OBJ1 = $(SRC:.c=.o) $(ASM:.s=.o)
OBJ = $(OBJ1:entry.o=)

CPUS = 1
BLKCOUNT = 0

QEMUOPTS = -machine virt -bios none -kernel kernel -m 128M -smp $(CPUS) -nographic
QEMUOPTS += -global virtio-mmio.force-legacy=false
QEMUOPTS += -drive file=vhd,if=none,format=raw,id=x0
QEMUOPTS += -device virtio-blk-device,drive=x0,bus=virtio-mmio-bus.0

QEMU = qemu-system-riscv64
ifndef TOOLPREFIX
TOOLPREFIX := $(shell if riscv64-unknown-elf-objdump -i 2>&1 | grep 'elf64-big' >/dev/null 2>&1; \
	then echo 'riscv64-unknown-elf-'; \
	elif riscv64-linux-gnu-objdump -i 2>&1 | grep 'elf64-big' >/dev/null 2>&1; \
	then echo 'riscv64-linux-gnu-'; \
	elif riscv64-unknown-linux-gnu-objdump -i 2>&1 | grep 'elf64-big' >/dev/null 2>&1; \
	then echo 'riscv64-unknown-linux-gnu-'; \
	else echo "***" 1>&2; \
	echo "*** Error: Couldn't find a riscv64 version of GCC/binutils." 1>&2; \
	echo "*** To turn off this error, run 'gmake TOOLPREFIX= ...'." 1>&2; \
	echo "***" 1>&2; exit 1; fi)
endif

CC = $(TOOLPREFIX)gcc
AS = $(TOOLPREFIX)as
LD = $(TOOLPREFIX)ld
OBJCOPY = $(TOOLPREFIX)objcopy
OBJDUMP = $(TOOLPREFIX)objdump

CFLAGS = -Wall -Werror -O0 -fno-omit-frame-pointer -ggdb -gdwarf-2
CFLAGS += -mcmodel=medany
CFLAGS += -ffreestanding -fno-common -nostdlib -mno-relax
CFLAGS += -I.
CFLAGS += $(shell $(CC) -fno-stack-protector -E -x c /dev/null >/dev/null 2>&1 && echo -fno-stack-protector)

build: kernel

run: kernel vhd
	qemu-system-riscv64 $(QEMUOPTS)
# sleep 1
# cat out
# wc out
# ps -ef | grep qemu | grep -v grep | awk '{print $2}' | xargs kill

debug:
	make qemu && make gdb

qemu: kernel vhd
	$(QEMU) $(QEMUOPTS) -s -S &

# Do `make gdb` separately with `make qemu` otherwie ctrl+c would terminate qemu immediately
gdb:
	gdb -ex "target extended-remote localhost:1234" \
							-ex "symbol-file kernel.o"

vhd:
	dd bs=1M if=/dev/zero of=$@ count=$(BLKCOUNT)

kernel: kernel.o
	$(OBJCOPY) -O binary $< $@

kernel.o: entry.o $(OBJ)
	$(LD) -Tlink.ld -o $@ $^

%.o : %.c
	$(CC) -c $(CFLAGS) -o $@ $< -g

%.o : %.s
	$(AS) -o $@ $< -g

clean:
	@rm kernel *.o out vhd 2>/dev/null || :

kill:
	@ps -ef | grep qemu | grep -v grep | awk '{print $$2}' | xargs kill

