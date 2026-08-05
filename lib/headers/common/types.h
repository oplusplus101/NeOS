
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

typedef SIZE PHYSICAL_ADDRESS;
typedef SIZE VIRTUAL_ADDRESS;

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
    BYTE r, g, b;
} sColour;

#define _RGB(r, g, b) ((sColour) { (r), (g), (b) })


#ifdef ARCH_EXE
#define ARCH_FUNC
#else
#define ARCH_FUNC __declspec(dllimport)
#endif

#endif // __COMMON__TYPES_H
