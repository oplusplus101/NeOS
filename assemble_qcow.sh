#/bin/bash
rm NeOS.qcow2 -f || true
qemu-img create NeOS.qcow2 1G -f qcow2
sudo modprobe nbd max_part=8
sudo qemu-nbd --connect=/dev/nbd0 NeOS.qcow2

# Initialise the partition
echo "Creating partition table..."
cat $1/partitions.sfdisk | sudo sfdisk /dev/nbd0
sync

# Setup the ESP
echo "Creating the ESP filesystem..."
sudo mkfs.fat -F 32 /dev/nbd0p1
mkdir -p esp
sudo mount /dev/nbd0p1 esp
sudo cp -r efi_part/* esp
sudo cp $1/startup.nsh esp/STARTUP.NSH
sudo umount esp
sync

# Setup the main partition
echo "Creating the main filesystem..."
sudo mkfs.fat -F 32 /dev/nbd0p2
mkdir -p main
sudo mount /dev/nbd0p2 main
sudo cp -r main_part/* main 
sudo umount main
sync

# Clean up
echo "Cleaning up..."
sudo rm -rf esp
sudo rm -rf main
sudo qemu-nbd -d /dev/nbd0
sudo modprobe -r nbd
sync
