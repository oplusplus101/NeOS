set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

set(CMAKE_C_COMPILER   x86_64-w64-mingw32-gcc)
set(CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++)
set(CMAKE_C_LINK_EXECUTABLE x86_64-w64-mingw32-ld)
set(CMAKE_ASM_NASM_LINK_EXECUTABLE x86_64-w64-mingw32-ld)
set(CMAKE_ASM_NASM_COMPILER nasm)
set(CMAKE_RC_COMPILER  x86_64-w64-mingw32-windres)

set(CMAKE_C_FLAGS_INIT "${CMAKE_C_FLAGS_INIT} -ffreestanding -fno-exceptions -fno-unwind-tables -fno-asynchronous-unwind-tables -fno-stack-protector")
set(CMAKE_EXE_LINKER_FLAGS_INIT "${CMAKE_EXE_LINKER_FLAGS_INIT} -nostdlib -nodefaultlibs -Wl,--no-seh")

# Stop from trying to run test executables
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

