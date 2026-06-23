#!/bin/bash
set -euo pipefail

ESP_IMG=esp.img
ESP_SIZE=64M

rm -f $ESP_IMG

# Create FAT image
truncate -s $ESP_SIZE $ESP_IMG
mkfs.fat -F 32 -n EFI $ESP_IMG

# Copy files without mounting
mcopy -i $ESP_IMG -s efi_part/* ::
mcopy -i $ESP_IMG "$1/startup.nsh" ::STARTUP.NSH
