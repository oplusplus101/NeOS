
BOOTLOADER_EXEC = src/boot/bootloader.efi
OVMFDIR = OVMF
IMG = NeOS.img

KERNEL_EXEC = obj/kernel.exe
KERNEL_CC_PARAMS = -m64 -c -std=c99 -O0 -nostdlib -ffreestanding -fno-builtin -fno-stack-protector \
				   -fno-stack-check -fno-exceptions -mno-stack-arg-probe -mno-red-zone -Iinclude
KERNEL_LD_PARAMS = -melf_x86_64 -Tlinker.ld
KERNEL_AS_PARAMS = -felf64 -O0
KERNEL_CC        = gcc
KERNEL_LD        = ld
KERNEL_OBJCOPY   = objcopy
KERNEL_AS        = nasm
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

$(IMG): $(BOOTLOADER_EXEC) $(KERNEL_EXEC)
	dd if=/dev/zero of=$(IMG) bs=1k count=1440
	mformat -i $(IMG) -f 1440 ::
	mmd -i $(IMG) ::/EFI
	mmd -i $(IMG) ::/EFI/BOOT
	mcopy -i $(IMG) $(BOOTLOADER_EXEC) ::/EFI/BOOT/BOOTX64.EFI
	mcopy -i $(IMG) $(KERNEL_EXEC) ::/NEOSKRNL.SYS

$(BOOTLOADER_EXEC): src/boot/bootloader.c
	cd src/boot && make

$(KERNEL_EXEC): $(KENREL_OBJECTS)
	$(KERNEL_LD) $(KERNEL_LD_PARAMS) $(KENREL_OBJECTS) -o $(KERNEL_EXEC).elf
	$(KERNEL_OBJCOPY) -O pei-x86-64 $(KERNEL_EXEC).elf $(KERNEL_EXEC)
	rm $(KERNEL_EXEC).elf

obj/%.o: src/%.c
	mkdir -p $(@D)
	$(KERNEL_CC) -o $@ $< $(KERNEL_CC_PARAMS)

obj/%.o: src/%.asm
	mkdir -p $(@D)
	$(KERNEL_AS) -o $@ $< $(KERNEL_AS_PARAMS)

clean:
	rm $(IMG) $(BOOTLOADER_EXEC) src/boot/bootloader.o obj -rf
