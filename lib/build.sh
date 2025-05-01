#!/bin/bash
gcc -c -o KNeOS.o KNeOS.c -I.
ar rcs libKNeOS.a KNeOS.o
