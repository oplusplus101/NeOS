echo "[3/4] Creating raw disk..."
rm -f NeOS.img
truncate -s 1G NeOS.img

echo "[4/4] Partitioning disk..."
sgdisk -o NeOS.img \
    -n 1:2048:+64M -t 1:EF00 \
    -n 2:0:+64M -t 2:204f4857-5349-4c20-4f52-443f20343036   

echo "Getting partitions"
# Get partition offsets
ESP_START=$(sgdisk -i 1 NeOS.img | grep "First sector" | awk '{print $3}')
MAIN_START=$(sgdisk -i 2 NeOS.img | grep "First sector" | awk '{print $3}')

ESP_OFFSET=$((ESP_START))
MAIN_OFFSET=$((MAIN_START))

echo "Writing ESP at offset $ESP_OFFSET..."
dd if=esp.img of=NeOS.img bs=512 seek=$ESP_OFFSET conv=notrunc status=progress

echo "Writing main FS at offset $MAIN_OFFSET..."
dd if=main_fs of=NeOS.img bs=512 seek=$MAIN_OFFSET conv=notrunc status=progress

rm main_fs esp.img -f
