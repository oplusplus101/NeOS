# The Plan

## Create a basic 64-bit OS with the following features
- UEFI bootloader
- A kernel that handles: users, the filesystem, memory, and loading drivers
- A GUI like DWM or Windows XP
- An NTFS filesystem, and/or a custom filesystem
- Use the PE32+ format for executables
- A bash-like shell and/or an MS-DOS-like shell.
- A BASIC-like scripting language


## The boot process
- The bootloader will load the loader located at: \NEOSLDR.SYS in the ESP
- The loader will load the config file at: C:\NeOS\NeOS.cfg
- The loader will then load the kernel at: C:\NeOS\NeOS.sys and execute it
- The kernel will setup syscalls and processes then load the filesystem driver, whose location is specified in the config file.
- The kernel will then load C:\NeOS\Drivers.cfg and load each enabled driver listed.
- The kernel will then load the each subsystem at C:\NeOS\Subsystems
- The kernel will finally run the autoexec file specified in the config file.


## Drivers
### Driver structure
- A driver is a kernel extension loaded at address 0xFFFFF82000000000 or higher
- Drivers must have an entry functin (DriverEntry) with the following signature:
STATUS DriverEntry(sDriverObject *)

- If a driver returns 0, it will be kept in memory,
- If a driver returns anything else, a crash-log will be created and a kernel panic will be invoked if needed.

### The Drivers.cfg file
- The label is the driver's file name
- Enable

## Subsystem
- Applications will run under a so-called subsystem, which is basically just a driver that interfaces between kernel functions and a more user-friendly API (e.g. POSIX, WIN-32). This will also allow for running Windows and Linux apps side-by-side.
- The subsystem will be a kernel-level process (basically a kernel inside a kernel)

## Paths
- The seperator can either be '/' or '\'
- The current directory is '.', and the higher directory is '..'
- Storage devices use the following format: '(A-Z):'
- The main drive will default to 'C:'
- Letter-case is ignored


## Filesystem structure and operation
- The system directory will be C:\NeOS
- There will be a directory for temp files in C:\Temp which will be cleared daily
- There will be "virtual files", i.e. files that are only visible to a specific process and the kernel.
  These virtual files can be persistent (they will stay when the process terminates) or non-persistent (deleted once the process terminates)

## Coding Style
### Prefixes
- b for BYTE
- bl for BOOL
- w for WORD
- dw for DWORD
- qw for QWORD
- s for a non-terminated string (e.g. A FAT-32 filename)
- sz for a zero-terminated string
- arr for array
- lst for list
- i for INT
- l for LONG
- ll for LONGLONG
- nX for a count of X element(s)
### Other
- Opening curly brackets are always on a new line instead of following the statement.
- Macro-functions are always prefixed with _
- Globals are prefixed with g_
- Struct typedefs are prefixed with s and struct names are prefixed with _tag

## Configuration
- The default file-format will be an INI-like format ending in either: .ini or .cfg
- Other configuration can be done using the Registry, which will be similar to the Windows registry
