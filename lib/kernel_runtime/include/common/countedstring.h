
#ifndef __COUNTED_STRING_H
#define __COUNTED_STRING_H

#include <common/types.h>

typedef struct
{
    PWCHAR wszData;
    QWORD qwLength;
} sString;

// Allocates a sString on the stack
#define _TEXT(ws) (sString) { ws, sizeof(ws) / sizeof(PWCHAR) }


sString MakeString(const PWCHAR wsz);
void DestroyString(sString *pws);

PWCHAR CopyStringAsCString(sString *pws);
sString CopyString(sString *pws);

/// @brief Appends pwsB to pwsA
/// @param pwsA The target
/// @param pwsB The string to be appended
/// @return pwsA
sString *AppendString(sString *pwsA, sString *pwsB);

/// @brief Converts a signed int into a string at a specific base
sString StringFromInt(LONGLONG ll, BYTE bBase);

/// @brief Converts an unsigned int into a string at a specific base
sString StringFromUInt(QWORD qw, BYTE bBase);

#endif // __COUNTED_STRING_H
