add-symbol-file build/arch/x86_64_EFI/ARCH.sys 0xFFFFF81000000000
add-symbol-file build/kernel/NeOS.sys          0xFFFFF80000000000
target remote localhost:9000
b KException0
b KException1
b KException2
b KException3
b KException4
b KException5
b KException6
b KException7
b KException8
b KException10
b KException11
b KException12
b KException13
b KException14
skip function Log
skip function PrintFormat
#skip function KHeapAlloc
#skip function KHeapFree
skip function KHeapReAlloc
#skip function HeapAlloc
#skip function HeapFree
skip function HeapReAlloc
skip function memset
skip function memcpy
skip function memcmp
skip function ZeroMemory
set disassembly-flavor intel
