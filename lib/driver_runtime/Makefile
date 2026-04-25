
SRC = $(wildcard src/*.c)
OBJ = $(SRC:.c=.o)
LIB  = libKNeOS.a
CC_PARAMS = -c --std=c2x -Werror -Wall -Iinclude -fno-stack-protector -nostdlib -ffreestanding -fno-builtin -fshort-wchar -m64

$(LIB): $(OBJ)
	x86_64-w64-mingw32-ar rcs $(LIB) $(OBJ)

src/%.o: src/%.c
	x86_64-w64-mingw32-gcc $(CC_PARAMS) -o $@ $<

clean:
	rm $(OBJ) $(LIB) -f
