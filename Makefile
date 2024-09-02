
BOOTLOADER_EXEC = src/boot/bootloader.efi
OVMFDIR = OVMF
IMG = NeOS.img

KERNEL_EXEC = obj/kernel.exe
KERNEL_CC_PARAMS = -c -std=c99 -nostdlib -fno-builtin -Iinclude -mno-red-zone
KERNEL_LD_PARAMS = -m64
KERNEL_CC = x86_64-w64-mingw32-gcc
KERNEL_LD = x86_64-w64-mingw32-ld
KENREL_OBJECTS = obj/kernel/kernel.o

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
	cd src/boot && make || true

$(KERNEL_EXEC): $(KENREL_OBJECTS)
	$(KERNEL_LD) $(KENREL_OBJECTS) -o $(KERNEL_EXEC)

obj/%.o: src/%.c
	mkdir -p $(@D)
	$(KERNEL_CC) -o $@ $< $(KERNEL_CC_PARAMS)

clean:
	rm $(IMG) $(BOOTLOADER_EXEC) src/boot/bootloader.o obj -rf
