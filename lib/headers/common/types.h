
#ifndef __COMMON__TYPES_H
#define __COMMON__TYPES_H

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

typedef INT HANDLE;
typedef INT STATUS;

typedef QWORD SIZE;

#define true 1
#define false 0
#define TRUE 1
#define FALSE 0
#define NULL ((PVOID) 0)
#define MS __attribute__((ms_abi))
#define SYSV __attribute__((sysv_abi))
#define FASTCALL __attribute__((fastcall))

typedef QWORD (SYSV *ISR)(QWORD);
typedef QWORD (SYSV *ESR)(QWORD, WORD);


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

typedef struct
{
    BYTE r, g, b;
} sColour;

#define _RGB(r, g, b) ((sColour) { (r), (g), (b) })

#endif // __COMMON__TYPES_H
