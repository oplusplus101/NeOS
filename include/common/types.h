
#ifndef __TYPES_H
#define __TYPES_H

typedef char CHAR;
typedef unsigned char BYTE;

typedef short SHORT;
typedef unsigned short WORD;

typedef int INT;
typedef unsigned int DWORD;

typedef long long int LONG64;
typedef unsigned long long int QWORD;

typedef QWORD SIZE_T;

typedef unsigned char BOOL;

typedef void VOID;
typedef VOID *PVOID;

#define true 1
#define false 0
#define TRUE 1
#define FALSE 0
#define NULL ((PVOID) 0)

#endif // __TYPES_H
