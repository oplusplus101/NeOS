
BOOTLOADER_EXEC	= bootloader/bootloader.efi
KERNEL_EXEC		= kernel/obj/kernel.exe
LOADER_EXEC		= loader/obj/loader.exe
OVMFDIR			= ./OVMF
IMG_FORMAT		= qcow2
IMG				= NeOS.$(IMG_FORMAT)
ESP				= /dev/nbd0p1
OSDRIVE			= osdrive
OSPART			= /dev/nbd0p2

CC			= gcc
LD			= ld
OBJCOPY		= objcopy
AS			= nasm
CC_PARAMS	= -m64 -c -std=c2x -O0 -nostdlib -ffreestanding -fno-builtin -fno-stack-protector \
			  -fno-stack-check -fno-exceptions -mno-stack-arg-probe -mno-red-zone -Iinclude -Wall -Wpedantic -Werror# -Wno-packed-bitfield-compat
QEMU		= qemu-system-x86_64
QEMU_PARAMS = -m 1G -cpu qemu64 -monitor stdio \
			  -drive if=pflash,format=raw,unit=0,file="$(OVMFDIR)/OVMF_CODE.fd",readonly=on \
			  -drive if=pflash,format=raw,unit=1,file="$(OVMFDIR)/OVMF_VARS.fd" -net none \
			  -drive id=disk,file=$(IMG),if=none,format=vmdk -device ahci,id=ahci -device ide-hd,drive=disk,bus=ahci.0
#			  -drive file=$(IMG),format=raw,if=none,id=nvm -device nvme,serial=cafebabe,drive=nvm

$(IMG): $(BOOTLOADER_EXEC) $(LOADER_EXEC) init-ospart
	make nbd-connect
	sudo dd if=/dev/zero of=$(ESP) bs=1k count=1440
	sudo mformat -i $(ESP) -f 1440 ::
	mkdir -p esp
	sudo mount $(ESP) esp
	sudo mkdir -p esp/EFI
	sudo mkdir -p esp/EFI/BOOT
	sudo cp $(BOOTLOADER_EXEC) esp/EFI/BOOT/BOOTX64.EFI
	sudo cp $(LOADER_EXEC) esp/NEOSLDR.SYS
	sudo cp startup.nsh esp/STARTUP.NSH
	sudo umount esp
	rm -rf esp
	make nbd-disconnect

run: $(IMG)
	$(QEMU) $(QEMU_PARAMS)

debug: $(IMG)
	$(QEMU) -gdb tcp::9000 $(QEMU_PARAMS)


$(BOOTLOADER_EXEC): bootloader/bootloader.c
	cd bootloader && make

$(LOADER_EXEC):
	cd loader && make

$(KERNEL_EXEC):
	cd kernel && make

init-ospart: $(KERNEL_EXEC) $(OSDRIVE)
	make nbd-connect
	mkdir -p ospart && sleep 0.5
	sudo mount $(OSPART) ospart

	sudo cp -r $(OSDRIVE)/* ospart
	sudo cp $(KERNEL_EXEC) ospart/NeOS/NeOS.sys

	# TODO: Copy over all the drivers and modules
	sudo umount ospart
	rm -rf ospart
	make nbd-disconnect

clean:
	cd loader && make clean
	cd kernel && make clean
	rm $(BOOTLOADER_EXEC) -rf

generate-diskimage:
	rm $(IMG) -f || true
	qemu-img create $(IMG) 1G -f $(IMG_FORMAT)
	make nbd-connect
	sudo sfdisk /dev/nbd0 < partitions.sfdisk
	sudo mkfs.vfat -F 32 $(OSPART)
	make nbd-disconnect

nbd-connect:
	sync
	sudo modprobe nbd max_part=8
	sudo qemu-nbd --connect=/dev/nbd0 $(IMG)
	sleep .1

nbd-disconnect:
	sudo qemu-nbd -d /dev/nbd0
	sudo nbd-client -d /dev/nbd0
	sudo modprobe -r nbd
	sync
	sleep .1
