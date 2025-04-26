
SRC = $(wildcard *.c)
OBJ = $(SRC:.c=.o)
EXEC = GOP-GFX.drv
CC_PARAMS = -c --std=c23 -Werror -Wall -I../../lib

$(EXEC): $(OBJ)
	ld -e DriverMain -o $(EXEC) $^

%.o: %.c
	gcc $(CC_PARAMS) -o $@ $<
