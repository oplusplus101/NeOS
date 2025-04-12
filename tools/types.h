
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

#endif // __TYPES_H
