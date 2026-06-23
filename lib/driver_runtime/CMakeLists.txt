# src/CMakeLists.txt

# Source files
file(GLOB_RECURSE SRC_FILES "src/*.c")

# Create static library
add_library(KNeOS STATIC ${SRC_FILES})

# C standard
target_compile_features(KNeOS PRIVATE c_std_23)

# Include directories
target_include_directories(KNeOS PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(KNeOS PUBLIC ${CMAKE_SOURCE_DIR}/lib/headers)


# Compiler flags
target_compile_options(KNeOS PRIVATE
    -Werror
    -Wall
    -fno-stack-protector
    -nostdlib
    -ffreestanding
    -fno-builtin
    -fshort-wchar
    -m64
)

# Output archive name
set_target_properties(KNeOS PROPERTIES
    OUTPUT_NAME "KNeOS"
    PREFIX "lib"
)
