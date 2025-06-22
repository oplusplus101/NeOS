#!/bin/bash
for name in */; do
    cd $name
    make -f ../Makefile
    if [ $? -ne 0 ]; then
        echo -e "\033[0;31mFailed to build $name\033[0m"
        cd ..
        exit 1
    fi
    cd ..
done
