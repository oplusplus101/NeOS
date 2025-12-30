
SRC = $(wildcard *.c)
OBJ = $(SRC:.c=.o)
EXEC = $(shell basename `pwd`).drv

CFLAGS = -c --std=c2x -Werror -Wall -I../../lib/driver/include -fPIE -fno-stack-protector -fno-builtin -ffreestanding -fshort-wchar -mno-red-zone

LDFLAGS = -L../../lib/driver -lKNeOS -nostdlib -pie -Wl,--dynamicbase -Wl,--enable-reloc-section -Wl,--image-base,0 -Wl,-e,DriverMain

$(EXEC): $(OBJ)
	x86_64-w64-mingw32-gcc -o $(EXEC) $^ $(LDFLAGS)

%.o: %.c
	x86_64-w64-mingw32-gcc $(CFLAGS) -o $@ $<

clean:
	rm $(OBJ) $(EXEC) -f
