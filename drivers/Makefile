
SRC = $(wildcard *.c)
OBJ = $(SRC:.c=.o)
EXEC = $(shell basename `pwd`).drv
CC_PARAMS = -c --std=c23 -Werror -Wall -I../../lib/include -fno-stack-protector -fno-builtin -ffreestanding -fshort-wchar
LD_PARAMS = -L../../lib -lKNeOS -e DriverMain

$(EXEC): $(OBJ)
	ld -o $(EXEC).elf $^ $(LD_PARAMS)
	objcopy -O pei-x86-64 $(EXEC).elf $(EXEC)
	rm $(EXEC).elf

%.o: %.c
	gcc $(CC_PARAMS) -o $@ $<

clean:
	rm $(OBJ) $(EXEC) -f
