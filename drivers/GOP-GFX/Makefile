
SRC = $(wildcard *.c)
OBJ = $(SRC:.c=.o)
EXEC = GOP-GFX.drv
CC_PARAMS = -c --std=c23 -Werror -Wall -I../../lib
LD_PARAMS = -L../../lib -lKNeOS -e DriverMain

$(EXEC): $(OBJ)
	ld -o $(EXEC).elf $^ $(LD_PARAMS)
	objcopy -O pei-x86-64 $(EXEC).elf $(EXEC)
	rm $(EXEC).elf

%.o: %.c
	gcc $(CC_PARAMS) -o $@ $<

clean:
	rm $(OBJ) $(EXEC) -f
