#!/bin/bash
for name in */; do
    cd $name
    make EXEC="`basename $name`.drv" -f ../Makefile clean
    if [ $? -ne 0 ]; then
        echo -e "\033[0;31mFailed to build $name\033[0m"
        cd ..
        exit 1
    fi
    cd ..
done
