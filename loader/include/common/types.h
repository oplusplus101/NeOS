
#ifndef __TYPES_H
#define __TYPES_H

typedef char CHAR;
typedef unsigned char BYTE;
typedef BYTE *PBYTE;
typedef CHAR *PCHAR;

typedef short SHORT;
typedef unsigned short WORD;
typedef SHORT *PSHORT;
typedef WORD *PWORD;

typedef int INT;
typedef unsigned int DWORD;
typedef INT *PINT;
typedef DWORD *PDWORD;

typedef long long int LONGLONG;
typedef unsigned long long int QWORD;
typedef LONGLONG *PLONGLONG;
typedef QWORD *PQWORD;

typedef unsigned char BOOL;
typedef BOOL *PBOOL;

typedef void VOID;
typedef VOID *PVOID;

typedef unsigned short WCHAR;
typedef WCHAR *PWCHAR;

#define true 1
#define false 0
#define TRUE 1
#define FALSE 0
#define NULL ((PVOID) 0)

typedef struct
{
    QWORD qwRAX;
    QWORD qwRBX;
    QWORD qwRCX;
    QWORD qwRDX;

    QWORD qwR8;
    QWORD qwR9;
    QWORD qwR10;
    QWORD qwR11;
    QWORD qwR12;
    QWORD qwR13;
    QWORD qwR14;
    QWORD qwR15;

    QWORD qwRSI;
    QWORD qwRDI;
    QWORD qwRBP;

    QWORD qwError;

    QWORD qwRIP;
    QWORD qwCS;
    QWORD qwFlags;
    QWORD qwRSP;
    QWORD qwSS;
} __attribute__((packed)) sCPUState;

#endif // __TYPES_H
