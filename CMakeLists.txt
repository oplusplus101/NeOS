
# Setup
cmake_minimum_required(VERSION 3.10)
project(NeOS)
enable_language(C ASM_NASM)

# Params
set(LOADER_EXE      NEOSLDR.sys)
set(KERNEL_EXE      NeOS.sys)
set(BOOTLOADER_PATH ${CMAKE_SOURCE_DIR}/bootloader/bootloader.efi)
set(IMAGE_PATH      ${CMAKE_BINARY_DIR}/NeOS.qcow2)
set(TERMINAL_EXE    st)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_FLAGS "-m64 -O0 -g -nostdlib -ffreestanding -fno-builtin -fstack-protector-all -fno-exceptions -mno-stack-arg-probe -mno-red-zone -Wall -Wpedantic -Werror -fshort-wchar -no-pie")

# Load the headers
include_directories(${LOADER_EXE} PRIVATE ${CMAKE_SOURCE_DIR}/loader/include)
include_directories(${KERNEL_EXE} PRIVATE ${CMAKE_SOURCE_DIR}/kernel/include)

# Load the sources
file(GLOB_RECURSE LOADER_SRC ${CMAKE_SOURCE_DIR}/loader/src/*.c ${CMAKE_SOURCE_DIR}/loader/src/*.asm)
file(GLOB_RECURSE KERNEL_SRC FOLLOW_SYMLINKS ${CMAKE_SOURCE_DIR}/kernel/src/*.c ${CMAKE_SOURCE_DIR}/kernel/src/*.asm)
add_executable(${LOADER_EXE} ${LOADER_SRC})
add_executable(${KERNEL_EXE} ${KERNEL_SRC})

# Linker stuff
set_target_properties(${LOADER_EXE} PROPERTIES LINK_FLAGS "-T ${CMAKE_SOURCE_DIR}/loader/linker.ld")
set_target_properties(${KERNEL_EXE} PROPERTIES LINK_FLAGS "-T ${CMAKE_SOURCE_DIR}/kernel/linker.ld")

# Post build
add_custom_command(
    TARGET ${LOADER_EXE} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} --output-format=pei-x86-64 $<TARGET_FILE:${LOADER_EXE}> $<TARGET_FILE:${LOADER_EXE}>
    COMMENT "Converting ${LOADER_EXE} to PE-32+"
)

add_custom_command(
    TARGET ${KERNEL_EXE} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} --output-format=pei-x86-64 $<TARGET_FILE:${KERNEL_EXE}> $<TARGET_FILE:${KERNEL_EXE}>
    COMMENT "Converting ${KERNEL_EXE} to PE-32+"
)

# Creates partitions: build/efi_part and build/main_part and image build/NeOS.sys
add_custom_target(
    image
    
    # Setup the EFI System Partition (ESP)
    COMMAND mkdir -p ${CMAKE_BINARY_DIR}/efi_part/EFI/BOOT
    COMMAND cp ${CMAKE_BINARY_DIR}/${LOADER_EXE} ${CMAKE_BINARY_DIR}/efi_part/NEOSLDR.SYS
    COMMAND cp ${BOOTLOADER_PATH} ${CMAKE_BINARY_DIR}/efi_part/EFI/BOOT/BOOTX64.EFI

    # Setup the main partition
    COMMAND mkdir -p ${CMAKE_BINARY_DIR}/main_part
    COMMAND cp -r ${CMAKE_SOURCE_DIR}/main_template/* ${CMAKE_BINARY_DIR}/main_part
    COMMAND cp ${CMAKE_BINARY_DIR}/${KERNEL_EXE} ${CMAKE_BINARY_DIR}/main_part/NeOS/NeOS.sys

    # Build the drivers
    COMMAND cd ${CMAKE_SOURCE_DIR}/drivers && ./build.sh
    COMMAND cp ${CMAKE_SOURCE_DIR}/drivers/*/*.drv ${CMAKE_BINARY_DIR}/main_part/NeOS/Drivers

    # Copy the libraries
    COMMAND cp ${CMAKE_SOURCE_DIR}/lib/*.dll ${CMAKE_BINARY_DIR}/main_part/NeOS/Libraries
    
    # Assemble the image
    COMMAND ${TERMINAL_EXE} ${CMAKE_SOURCE_DIR}/assemble_qcow.sh ${CMAKE_SOURCE_DIR}
    
    COMMENT "Creating image..."
    DEPENDS ${KERNEL_EXE} ${LOADER_EXE}
)

# For some reason, this breaks in vscode
add_custom_target(run COMMAND ${TERMINAL_EXE} ${CMAKE_SOURCE_DIR}/run.sh ${CMAKE_SOURCE_DIR}/OVMF ${IMAGE_PATH})
add_custom_target(gdb COMMAND ${TERMINAL_EXE} ${CMAKE_SOURCE_DIR}/run.sh ${CMAKE_SOURCE_DIR}/OVMF ${IMAGE_PATH} "-gdb tcp::9000 -S")
add_custom_target(qemu-dbg COMMAND ${TERMINAL_EXE} ${CMAKE_SOURCE_DIR}/run.sh ${CMAKE_SOURCE_DIR}/OVMF ${IMAGE_PATH} "-d int,cpu_reset,guest_errors -no-reboot -no-shutdown -D qemu.log")

# Clean up
set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES "${IMAGE_PATH};${CMAKE_BINARY_DIR}/efi_part;${CMAKE_BINARY_DIR}/main_part")
