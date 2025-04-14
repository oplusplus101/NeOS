
CC			= gcc
LD			= ld
OBJCOPY		= objcopy
AS			= nasm
AS_PARAMS	= -felf64 -O0
LD_PARAMS	= -melf_x86_64 -Tlinker.ld
CC_PARAMS	= -m64 -c -std=c2x -O0 -nostdlib -ffreestanding -fno-builtin -fno-stack-protector \
			  -fno-stack-check -fno-exceptions -fshort-wchar \
			  -mno-stack-arg-probe -mno-red-zone \
			  -Iinclude -Wall -Wpedantic -Werror# -Wno-packed-bitfield-compat

EXEC		= obj/loader.exe
OBJECTS		= obj/loader.o \
			  obj/common/exceptions.o \
			  obj/common/string.o \
			  obj/common/ini.o \
			  obj/common/screen.o \
			  obj/memory/paging.o \
			  obj/memory/bitmap.o \
			  obj/memory/heap.o \
			  obj/memory/list.o \
			  obj/hardware/gdt.o \
			  obj/hardware/pci.o \
			  obj/hardware/gdtasm.o \
			  obj/hardware/idt.o \
			  obj/hardware/idtasm.o \
			  obj/hardware/storage/atapi.o \
			  obj/hardware/storage/nvme.o \
			  obj/hardware/storage/drive.o \
			  obj/hardware/storage/ahci.o \
			  obj/filesystem/gpt.o \
			  obj/filesystem/fat32.o
	
$(EXEC): $(OBJECTS)
	$(LD) $(LD_PARAMS) $(OBJECTS) -o $(EXEC).elf
	$(OBJCOPY) -O pei-x86-64 $(EXEC).elf $(EXEC)
	rm $(EXEC).elf

obj/%.o: src/%.c
	mkdir -p $(@D)
	$(CC) -o $@ $< $(CC_PARAMS)

obj/%.o: src/%.asm
	mkdir -p $(@D)
	$(AS) -o $@ $< $(AS_PARAMS)

clean:
	rm obj $(EXEC) -rf
