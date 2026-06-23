# Load the headers
include_directories(${KERNEL_EXE} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${KERNEL_EXE} PRIVATE ${CMAKE_SOURCE_DIR}/lib/kernel_runtime/include)
include_directories(${KERNEL_EXE} PRIVATE ${CMAKE_SOURCE_DIR}/lib/headers)

# Load the sources
file(GLOB_RECURSE KERNEL_SRC FOLLOW_SYMLINKS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c ${CMAKE_CURRENT_SOURCE_DIR}/src/*.asm)
add_executable(${KERNEL_EXE} ${KERNEL_SRC} $<TARGET_OBJECTS:runtime>)
add_dependencies(${KERNEL_EXE} ${ARCH_EXE})
set_target_properties(${KERNEL_EXE} PROPERTIES SUFFIX ".sys")

target_compile_definitions(${KERNEL_EXE} PRIVATE KERNEL)

# Compiler flags
target_compile_options(${KERNEL_EXE} PRIVATE $<$<COMPILE_LANGUAGE:C>:${BASE_FLAGS} -fpie -DQEMU_UART>)

# Linker flags
target_link_options(${KERNEL_EXE} PRIVATE -pie -nostdlib -Wl,--entry=KernelMain ${CMAKE_CURRENT_SOURCE_DIR}/NeOS.def)
target_link_libraries(${KERNEL_EXE} PRIVATE ${CMAKE_BINARY_DIR}/libARCH.sys.a)

# add_custom_command(TARGET ${KERNEL_EXE} POST_BUILD COMMAND nm -n -a ${CMAKE_BINARY_DIR}/${KERNEL_EXE}.sys > ${CMAKE_BINARY_DIR}/kernel.addrs)

add_custom_command(TARGET ${KERNEL_EXE} POST_BUILD
    COMMAND x86_64-w64-mingw32-dlltool
            --def ${CMAKE_CURRENT_SOURCE_DIR}/NeOS.def
            --dllname $<TARGET_FILE:${KERNEL_EXE}>.sys
            --output-lib ${CMAKE_BINARY_DIR}/${KERNEL_LIBRARY}
    COMMENT "Creating import library for drivers"
)
