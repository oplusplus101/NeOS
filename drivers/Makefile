
SRC = $(wildcard *.c)
OBJ = $(SRC:.c=.o)
EXEC = $(shell basename `pwd`).drv
CC_PARAMS = -c --std=c23 -Werror -Wall -I../../lib/include -fno-stack-protector -fno-builtin -ffreestanding -fshort-wchar
LD_PARAMS = -L../../lib -lKNeOS

$(EXEC): $(OBJ)
	x86_64-w64-mingw32-ld -o $(EXEC) $^ $(LD_PARAMS)

%.o: %.c
	x86_64-w64-mingw32-gcc $(CC_PARAMS) -o $@ $<

clean:
	rm $(OBJ) $(EXEC) -f
