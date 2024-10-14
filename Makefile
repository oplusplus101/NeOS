
BOOTLOADER_EXEC = src/boot/bootloader.efi
OVMFDIR = OVMF
IMG = NeOS.img

CC        = gcc
LD        = ld
OBJCOPY   = objcopy
AS        = nasm
CC_PARAMS = -m64 -c -std=c99 -O0 -nostdlib -ffreestanding -fno-builtin -fno-stack-protector \
			-fno-stack-check -fno-exceptions -mno-stack-arg-probe -mno-red-zone -Iinclude

LOADER_EXEC      = obj/loader.exe
LOADER_OBJECTS   = obj/loader/loader.o \
				   obj/common/screen.o \
				   obj/hardware/gdt.o \
				   obj/hardware/gdtasm.o
LOADER_LD_PARAMS = -melf_x86_64 -Tlinker.ld

KERNEL_EXEC = obj/kernel.exe
KERNEL_LD_PARAMS = -melf_x86_64 -Tlinker.ld
AS_PARAMS = -felf64 -O0
KENREL_OBJECTS   = obj/kernel/kernel.o \
				   obj/common/screen.o \
				   obj/hardware/gdt.o \
				   obj/hardware/gdtasm.o \
				   obj/hardware/idt.o \
				   obj/hardware/idtasm.o

run: $(IMG)
	qemu-system-x86_64 -m 1G -cpu qemu64 -monitor stdio \
					   -drive file=$(IMG) \
					   -drive if=pflash,format=raw,unit=0,file="$(OVMFDIR)/OVMF_CODE-pure-efi.fd",readonly=on \
					   -drive if=pflash,format=raw,unit=1,file="$(OVMFDIR)/OVMF_VARS-pure-efi.fd" -net none

$(IMG): $(BOOTLOADER_EXEC) $(LOADER_EXEC)
	dd if=/dev/zero of=$(IMG) bs=1k count=1440
	mformat -i $(IMG) -f 1440 ::
	mmd -i $(IMG) ::/EFI
	mmd -i $(IMG) ::/EFI/BOOT
	mcopy -i $(IMG) $(BOOTLOADER_EXEC) ::/EFI/BOOT/BOOTX64.EFI
	mcopy -i $(IMG) $(LOADER_EXEC) ::/NEOSLDR.SYS

$(LOADER_EXEC): $(LOADER_OBJECTS)
	$(LD) $(LOADER_LD_PARAMS) $(LOADER_OBJECTS) -o $(LOADER_EXEC).elf
	$(OBJCOPY) -O pei-x86-64 $(LOADER_EXEC).elf $(LOADER_EXEC)
	rm $(LOADER_EXEC).elf


$(BOOTLOADER_EXEC): src/boot/bootloader.c
	cd src/boot && make

$(KERNEL_EXEC): $(KENREL_OBJECTS)
	$(LD) $(KERNEL_LD_PARAMS) $(KENREL_OBJECTS) -o $(KERNEL_EXEC).elf
	$(OBJCOPY) -O pei-x86-64 $(KERNEL_EXEC).elf $(KERNEL_EXEC)
	rm $(KERNEL_EXEC).elf

obj/%.o: src/%.c
	mkdir -p $(@D)
	$(CC) -o $@ $< $(CC_PARAMS)

obj/%.o: src/%.asm
	mkdir -p $(@D)
	$(AS) -o $@ $< $(AS_PARAMS)

clean:
	rm $(IMG) $(BOOTLOADER_EXEC) src/boot/bootloader.o obj -rf
