
ARCH = x86_64
TARGET = bootloader.efi
SRCS = $(wildcard *.c)
CFLAGS = -Wpedantic -Wall -Wextra --ansi -O0 -std=c99 -I../lib/headers -mno-red-zone

include uefi/Makefile
