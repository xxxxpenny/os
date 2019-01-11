#!/bin/bash
alias gcc-i386='docker run --rm -v $PWD:/compile -w /compile i386/gcc gcc'
alias ld-i386='docker run --rm -v $PWD:/compile -w /compile i386/gcc ld'
nasm -f elf -o lib/kernel/print.o lib/kernel/print.S
gcc-i386 -I lib/kernel/ -c -o kernel/main.o kernel/main.c
ld-i386 -Ttext 0xc0001500 -e main -o kernel.bin kernel/main.o lib/kernel/print.o
rm kernel/main.o lib/kernel/print.o
dd if=kernel.bin of=/Users/aiyi/bochs/hd60M.img bs=512 count=200 seek=9 conv=notrunc