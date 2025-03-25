EXEC		= obj/kernel.exe
CC			= gcc
AS			= nasm
OBJCOPY     = objcopy
LD_PARAMS	= -melf_x86_64 -Tlinker.ld
AS_PARAMS	= -felf64 -O0
CC_PARAMS	= -m64 -c -std=c2x -O0 -nostdlib -ffreestanding -fno-builtin -fno-stack-protector \
			  -fno-stack-check -fno-exceptions -mno-stack-arg-probe -mno-red-zone -Iinclude -Wall -Wpedantic -Werror# -Wno-packed-bitfield-compat
OBJECTS		= obj/kernel.o \
			  obj/common/screen.o \
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
