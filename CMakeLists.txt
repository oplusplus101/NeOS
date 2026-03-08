
# Setup
cmake_minimum_required(VERSION 3.10)
project(NeOS)
enable_language(C ASM_NASM)

# Params
set(LOADER_EXE      NEOSLDR)
set(KERNEL_EXE      NeOS)
set(KERNEL_LIBRARY  libNeOS.sys.a)
set(BOOTLOADER_PATH ${CMAKE_SOURCE_DIR}/bootloader/bootloader.efi)
set(IMAGE_PATH      ${CMAKE_BINARY_DIR}/NeOS.qcow2)
set(BASE_FLAGS      -m64 -O0 -nostdlib -fno-builtin -fno-omit-frame-pointer -fno-exceptions -mno-stack-arg-probe -mno-red-zone -Wall -Wpedantic -Werror -fshort-wchar -Wl,--no-relax)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Load the headers
include_directories(${LOADER_EXE} PRIVATE ${CMAKE_SOURCE_DIR}/loader/include)
include_directories(${KERNEL_EXE} PRIVATE ${CMAKE_SOURCE_DIR}/kernel/include)
include_directories(${LOADER_EXE} PRIVATE ${CMAKE_SOURCE_DIR}/lib/kernel_runtime/include)
include_directories(${KERNEL_EXE} PRIVATE ${CMAKE_SOURCE_DIR}/lib/kernel_runtime/include)

# Load the sources
file(GLOB_RECURSE LOADER_SRC ${CMAKE_SOURCE_DIR}/loader/src/*.c ${CMAKE_SOURCE_DIR}/loader/src/*.asm)
file(GLOB_RECURSE KERNEL_SRC FOLLOW_SYMLINKS ${CMAKE_SOURCE_DIR}/kernel/src/*.c ${CMAKE_SOURCE_DIR}/kernel/src/*.asm)
add_executable(${LOADER_EXE} ${LOADER_SRC} $<TARGET_OBJECTS:runtime>)
add_executable(${KERNEL_EXE} ${KERNEL_SRC} $<TARGET_OBJECTS:runtime>)
set_target_properties(${LOADER_EXE} PROPERTIES SUFFIX ".sys")
set_target_properties(${KERNEL_EXE} PROPERTIES SUFFIX ".sys")

target_compile_definitions(${LOADER_EXE} PRIVATE LOADER)
target_compile_definitions(${KERNEL_EXE} PRIVATE KERNEL)

file(GLOB_RECURSE KERNEL_RUNTIME_SRC "${CMAKE_SOURCE_DIR}/lib/kernel_runtime/src/*.c")

add_library(runtime OBJECT ${KERNEL_RUNTIME_SRC})

# Compiler flags
target_compile_options(${LOADER_EXE} PRIVATE $<$<COMPILE_LANGUAGE:C>:${BASE_FLAGS} -fno-pie -finstrument-functions -DFUNCTION_INSTRUMENTATIONS>)
target_compile_options(${KERNEL_EXE} PRIVATE $<$<COMPILE_LANGUAGE:C>:${BASE_FLAGS} -fno-pie -finstrument-functions -DFUNCTION_INSTRUMENTATIONS>)
target_compile_options(runtime       PRIVATE $<$<COMPILE_LANGUAGE:C>:${BASE_FLAGS} -fno-pie -finstrument-functions -DFUNCTION_INSTRUMENTATIONS>)

# Linker flags
target_link_options(${LOADER_EXE} PRIVATE -T ${CMAKE_SOURCE_DIR}/loader/linker.ld -no-pie -nostdlib -Wl,--image-base,0x100000)
target_link_options(${KERNEL_EXE} PRIVATE -T ${CMAKE_SOURCE_DIR}/kernel/linker.ld -no-pie -nostdlib -Wl,--image-base,0x200000 ${CMAKE_SOURCE_DIR}/kernel/NeOS.def)
target_link_options(runtime PRIVATE -no-pie -nostdlib)

add_custom_command(TARGET ${KERNEL_EXE} POST_BUILD COMMAND nm -n -a ${CMAKE_BINARY_DIR}/${KERNEL_EXE}.sys > ${CMAKE_BINARY_DIR}/kernel.addrs)
add_custom_command(TARGET ${LOADER_EXE} POST_BUILD COMMAND nm -n -a ${CMAKE_BINARY_DIR}/${LOADER_EXE}.sys > ${CMAKE_BINARY_DIR}/loader.addrs)

add_custom_command(TARGET ${KERNEL_EXE} POST_BUILD
    COMMAND x86_64-w64-mingw32-dlltool
            --def ${CMAKE_SOURCE_DIR}/kernel/NeOS.def
            --dllname $<TARGET_FILE:${KERNEL_EXE}>.sys
            --output-lib ${CMAKE_BINARY_DIR}/${KERNEL_LIBRARY}
    COMMENT "Creating import library for drivers"
)

# Creates partitions: build/efi_part and build/main_part and image build/NeOS.sys
add_custom_target(
    image
    
    # Setup the EFI System Partition (ESP)
    COMMAND mkdir -p ${CMAKE_BINARY_DIR}/efi_part/EFI/BOOT
    COMMAND cp ${CMAKE_BINARY_DIR}/${LOADER_EXE}.sys ${CMAKE_BINARY_DIR}/efi_part/NEOSLDR.SYS
    COMMAND cp ${BOOTLOADER_PATH} ${CMAKE_BINARY_DIR}/efi_part/EFI/BOOT/BOOTX64.EFI

    # Setup the main partition
    COMMAND mkdir -p ${CMAKE_BINARY_DIR}/main_part
    COMMAND cp -r ${CMAKE_SOURCE_DIR}/main_template/* ${CMAKE_BINARY_DIR}/main_part
    COMMAND cp ${CMAKE_BINARY_DIR}/${KERNEL_EXE}.sys ${CMAKE_BINARY_DIR}/main_part/NeOS/NeOS.sys

    # Build the drivers
    COMMAND cd ${CMAKE_SOURCE_DIR}/drivers && ./build.sh
    COMMAND cp ${CMAKE_SOURCE_DIR}/drivers/*/*.drv ${CMAKE_BINARY_DIR}/main_part/NeOS/Drivers

    # Copy the libraries
    COMMAND cp ${CMAKE_SOURCE_DIR}/lib/*/*.dll ${CMAKE_BINARY_DIR}/main_part/NeOS/Libraries || true
    
    # Assemble the image
    COMMAND ${TERMINAL_EXE} ${CMAKE_SOURCE_DIR}/assemble_qcow.sh ${CMAKE_SOURCE_DIR}
    
    COMMENT "Creating image..."
    DEPENDS ${KERNEL_EXE} ${LOADER_EXE}
)

add_custom_target(run COMMAND ${CMAKE_SOURCE_DIR}/run.sh ${CMAKE_SOURCE_DIR}/OVMF ${IMAGE_PATH})
add_custom_target(gdb COMMAND ${CMAKE_SOURCE_DIR}/run.sh ${CMAKE_SOURCE_DIR}/OVMF ${IMAGE_PATH} "-gdb tcp::9000 -S")
add_custom_target(qemu-dbg COMMAND ${CMAKE_SOURCE_DIR}/run.sh ${CMAKE_SOURCE_DIR}/OVMF ${IMAGE_PATH} "-d int,cpu_reset,guest_errors,nochain -no-reboot -no-shutdown -D qemu.log -no-hpet")

# Clean up
set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES "${IMAGE_PATH};${CMAKE_BINARY_DIR}/efi_part;${CMAKE_BINARY_DIR}/main_part;${CMAKE_BINARY_DIR}/${KERNEL_LIBRARY}")
