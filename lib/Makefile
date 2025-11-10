
SRC = $(wildcard src/*.c)
OBJ = $(SRC:.c=.o)
EXEC = KNeOS.dll
LIB  = libKNeOS.a
CC_PARAMS = -c --std=c23 -Werror -Wall -Iinclude -fno-stack-protector -nostdlib -ffreestanding -fno-builtin -fshort-wchar

$(EXEC): $(OBJ)
	x86_64-w64-mingw32-gcc -fPIE -fPIC -static -nostartfiles -shared -o $@ $^ -Wl,--out-implib,$(LIB)

src/%.o: src/%.c
	x86_64-w64-mingw32-gcc $(CC_PARAMS) -o $@ $<

clean:
	rm $(OBJ) $(EXEC) $(LIB) -f
