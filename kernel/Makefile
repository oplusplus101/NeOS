EXEC		= obj/kernel.exe
CC			= gcc
AS			= nasm
OBJCOPY     = objcopy
LD_PARAMS	= -melf_x86_64 -Tlinker.ld
AS_PARAMS	= -felf64 -O0
CC_PARAMS	= -m64 -c -std=c2x -O0 -nostdlib -ffreestanding -fno-builtin -fstack-protector-all \
			  -fno-exceptions -mno-stack-arg-probe -mno-red-zone -Iinclude -Wall -Wpedantic -Werror -fshort-wchar
OBJECTS		= obj/kernel.o \
			  obj/exe.o \
			  obj/timer.o \
			  obj/process.o \
			  obj/syscalls.o \
			  obj/syscallimpl.o \
			  obj/common/ini.o \
			  obj/common/screen.o \
			  obj/common/exceptions.o \
			  obj/memory/paging.o \
			  obj/memory/bitmap.o \
			  obj/memory/heap.o \
			  obj/memory/list.o \
			  obj/hardware/gdt.o \
			  obj/hardware/gdtasm.o \
			  obj/hardware/idt.o \
			  obj/hardware/idtasm.o

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
