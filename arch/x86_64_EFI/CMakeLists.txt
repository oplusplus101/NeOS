
# Load the headers
include_directories(${ARCH_EXE} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${ARCH_EXE} PRIVATE ${CMAKE_SOURCE_DIR}/lib/headers)

# Load the sources
file(GLOB_RECURSE ARCH_SRC FOLLOW_SYMLINKS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c ${CMAKE_CURRENT_SOURCE_DIR}/src/*.asm)
add_executable(${ARCH_EXE} ${ARCH_SRC})
set_target_properties(${ARCH_EXE} PROPERTIES SUFFIX ".sys")

target_compile_definitions(${ARCH_EXE} PRIVATE ARCH)
target_compile_options(${ARCH_EXE} PRIVATE $<$<COMPILE_LANGUAGE:C>:${BASE_FLAGS} -fpie>)

# Linker flags
target_link_options(${ARCH_EXE} PRIVATE -pie -nostdlib -Wl,--entry=ArchMain ${CMAKE_CURRENT_SOURCE_DIR}/arch.def)

# add_custom_command(TARGET ${ARCH_EXE} POST_BUILD COMMAND nm -n -a ${CMAKE_BINARY_DIR}/arch/${ARCH_EXE}.sys > ${CMAKE_BINARY_DIR}/arch.addrs)

add_custom_command(TARGET ${ARCH_EXE} POST_BUILD
    COMMAND x86_64-w64-mingw32-dlltool
            --def ${CMAKE_CURRENT_SOURCE_DIR}/arch.def
            --dllname $<TARGET_FILE:${ARCH_EXE}>
            --output-lib ${CMAKE_BINARY_DIR}/${ARCH_LIBRARY}
    COMMENT "Creating import library"
)

# Clean up
set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES "${CMAKE_BINARY_DIR}/${ARCH_LIBRARY}")
