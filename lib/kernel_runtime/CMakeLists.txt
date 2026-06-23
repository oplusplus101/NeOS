
file(GLOB_RECURSE KERNEL_RUNTIME_SRC "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c")
include_directories(${KERNEL_RUNTIME} ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/arch/${TARGET_ARCH}/include ${CMAKE_SOURCE_DIR}/lib/headers)

add_library(${KERNEL_RUNTIME} OBJECT ${KERNEL_RUNTIME_SRC})

# Compiler flags
target_compile_options(${KERNEL_RUNTIME} PRIVATE $<$<COMPILE_LANGUAGE:C>:${BASE_FLAGS} -fpie>)

# Linker flags
target_link_options(${KERNEL_RUNTIME} PRIVATE -pie -nostdlib ${CMAKE_BINARY_DIR}/libARCH.sys.a)
