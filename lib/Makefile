
SRC = $(wildcard src/*.c)
OBJ = $(SRC:.c=.o)
EXEC = libKNeOS.a
CC_PARAMS = -c --std=c23 -Werror -Wall -Iinclude -fno-stack-protector -nostdlib -ffreestanding -fno-builtin

$(EXEC): $(OBJ)
	ar rcs $@ $^

src/%.o: src/%.c
	gcc $(CC_PARAMS) -o $@ $<

clean:
	rm $(OBJ) $(EXEC) -f
