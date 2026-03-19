
qemu-system-x86_64 $3 -m 1G -cpu qemu64 -serial stdio \
                   -drive if=pflash,format=raw,unit=0,file=$1/OVMF_CODE.fd,readonly=on \
                   -drive if=pflash,format=raw,unit=1,file=$1/OVMF_VARS.fd,readonly=on \
                   -drive id=disk,file=$2,if=none,format=qcow2 -device ahci,id=ahci -device ide-hd,drive=disk,bus=ahci.0 \
                   -no-reboot