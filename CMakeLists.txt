
cmake_minimum_required(VERSION 3.20)
project(NeOS C ASM_NASM)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(ARCH_EXE      ARCH)
set(ARCH_LIBRARY  libARCH.sys.a)
set(KERNEL_EXE NeOS)
set(KERNEL_LIBRARY libNeOS.sys.a)
set(KERNEL_RUNTIME runtime)
set(BASE_FLAGS -m64 -O0 -mno-avx -nostdlib -fno-builtin -fno-omit-frame-pointer -fno-exceptions -mno-stack-arg-probe -mno-red-zone -Wall -Wpedantic -Werror -fshort-wchar -Wl,--no-relax)

set(TARGET_ARCH "x86_64_EFI" CACHE STRING "Target architecture/platform")

set(SUPPORTED_ARCHS x86_64_EFI)

list(FIND SUPPORTED_ARCHS ${TARGET_ARCH} ARCH_INDEX)

if(ARCH_INDEX EQUAL -1)
    message(FATAL_ERROR "Unsupported TARGET_ARCH=${TARGET_ARCH}\n" "Supported: ${SUPPORTED_ARCHS}")
endif()

message(STATUS "Building for architecture: ${TARGET_ARCH}")
add_subdirectory(arch/${TARGET_ARCH})

add_subdirectory(lib/kernel_runtime)
add_subdirectory(lib/driver_runtime)

add_subdirectory(kernel)
add_subdirectory(drivers)

# Build the image
add_custom_target(image ALL

    # --- Prepare EFI partition files ---
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/efi_part/EFI/BOOT

    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/bootloader/bootloader.efi ${CMAKE_BINARY_DIR}/efi_part/EFI/BOOT/BOOTX64.EFI

    # --- Prepare main filesystem tree ---
    COMMAND ${CMAKE_COMMAND} -E make_directory
        ${CMAKE_BINARY_DIR}/main_part

    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/main_template ${CMAKE_BINARY_DIR}/main_part

    COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_BINARY_DIR}/arch/${TARGET_ARCH}/${ARCH_EXE}.sys
        ${CMAKE_BINARY_DIR}/main_part/NeOS/${ARCH_EXE}.sys

    COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_BINARY_DIR}/kernel/${KERNEL_EXE}.sys
        ${CMAKE_BINARY_DIR}/main_part/NeOS/${KERNEL_EXE}.sys

    # Drivers (no build step anymore)
    COMMAND ${CMAKE_COMMAND} -E make_directory
        ${CMAKE_BINARY_DIR}/main_part/NeOS/Drivers

    COMMAND bash -c "cp ${CMAKE_SOURCE_DIR}/drivers/**/*.drv ${CMAKE_BINARY_DIR}/main_part/NeOS/Drivers"

    # Libraries
    # COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/main_part/NeOS/Libraries

    # COMMAND bash -c "find ${CMAKE_SOURCE_DIR}/lib -name '*.dll' | sort | xargs -I{} cp {} ${CMAKE_BINARY_DIR}/main_part/NeOS/Libraries/ || true"

    # --- Build final image ---
    COMMAND bash ${CMAKE_SOURCE_DIR}/scripts/create_esp.sh ${CMAKE_SOURCE_DIR}
    COMMAND bash ${CMAKE_SOURCE_DIR}/scripts/create_main_part.sh
    COMMAND bash ${CMAKE_SOURCE_DIR}/scripts/create_image.sh

    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}

    COMMENT "Creating NeOS disk image..."
    DEPENDS ${KERNEL_EXE} ${ARCH_EXE} ${ALL_DRIVER_TARGETS}
)

add_custom_target(qcow2
    COMMAND qemu-img convert -f raw -O qcow2 NeOS.img NeOS.qcow2
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    DEPENDS image
)

add_custom_target(run
    COMMAND bash ../run.sh ../OVMF NeOS.img
    DEPENDS image
)

add_custom_target(gdb
    COMMAND bash ../run.sh ../OVMF NeOS.img "-gdb tcp::9000 -S"
    DEPENDS image
)
