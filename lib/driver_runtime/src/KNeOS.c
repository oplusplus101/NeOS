
#define DLL_BUILD
#include <KNeOS.h>

// Extra functions
LIB_FUNC void KNeoPrintHex(QWORD n, BYTE nDigits, BOOL bUppercase)
{
    nDigits = nDigits > 16 ? 16 : nDigits;
    PCHAR sDigits = bUppercase ? "0123456789ABCDEF" : "0123456789abcdef";
    CHAR sNumber[16];

    for (INT i = 0; i < nDigits; i++)
    {
        sNumber[i] = sDigits[n % 16];
        n /= 16;
    }
    
    for (INT i = nDigits - 1; i >= 0; i--)
        KNeoPrintChar(sNumber[i]);
}

LIB_FUNC void KNeoPrintDec(QWORD n)
{
    if (n == 0)
    {
        KNeoPrintChar('0');
        return;
    }

    CHAR sNumber[20];
    INT i = 0;
    while (n)
    {
        sNumber[i++] = (n % 10) + '0';
        n /= 10;
    }
    
    for (INT j = i - 1; j >= 0; j--)
        KNeoPrintChar(sNumber[j]);
}

LIB_FUNC void KNeoPrintFormat(const PWCHAR wszFormat, ...)
{
    __builtin_va_list args;
    __builtin_va_start(args, wszFormat);
    for (QWORD i = 0; wszFormat[i] != 0; i++)
    {
        if (wszFormat[i] == L'%')
        {
            i++; // Skip the %
            switch (wszFormat[i])
            {
            case L'd':
            case L'i':
                INT n = __builtin_va_arg(args, INT);
                if (n < 0)
                {
                    n *= -1;
                    KNeoPrintChar('-');
                }
                KNeoPrintDec(n);
                break;
            case L'u':
                KNeoPrintDec(__builtin_va_arg(args, QWORD));
                break;
            case L'p':
                KNeoPrintHex(__builtin_va_arg(args, QWORD), 16, true);
                break;
                case L's':
                const PCHAR sz = __builtin_va_arg(args, const PCHAR);
                if (sz == NULL)
                    KNeoPrintString("(null)");
                else
                    KNeoPrintString(sz);
                break;
            case L'w':
                const PWCHAR wsz = __builtin_va_arg(args, const PWCHAR);
                if (wsz == NULL)
                    KNeoPrintString("(null)");
                else
                    for (QWORD j = 0; wsz[j] != 0; j++)
                        KNeoPrintChar(wsz[j]);
                break;
            
            case L'c':
                KNeoPrintChar((char) __builtin_va_arg(args, DWORD));
                break;
            case L'%':
                KNeoPrintChar('%');
                break;
            // case L'G': // GUID
            //     BYTE *pGUID = __builtin_va_arg(args, BYTE *);
                // PrintFormat("%08X-%04X-%04X-%04X-%12X", *((DWORD *) &pGUID[0]),
                //                                         *((WORD *)  &pGUID[4]),
                //                                         *((WORD *)  &pGUID[6]),
                //                                         _BE2LEW(*((WORD *)  &pGUID[8])),
                //                                         _BE2LE48(*((QWORD *) &pGUID[10])));
                break;
            default:
    
                if (wszFormat[i] >= '0' && wszFormat[i] <= '9' && wszFormat[i + 1] >= '0' && wszFormat[i + 1] <= '9')
                {
                    BYTE qwDigits = (wszFormat[i] - '0') * 10 + (wszFormat[i + 1] - '0');
                    i += 2;
                    if (wszFormat[i] == 'X')
                        KNeoPrintHex(__builtin_va_arg(args, QWORD), qwDigits, true);
                    else if (wszFormat[i] == 'x')
                        KNeoPrintHex(__builtin_va_arg(args, QWORD), qwDigits, false);
                    else if (wszFormat[i] == 's')
                        for (BYTE j = 0; j < qwDigits; j++)
                            KNeoPrintChar(__builtin_va_arg(args, const PCHAR)[j]);
                }
            }
            
        }
        else
            KNeoPrintChar(wszFormat[i]);
    }
    __builtin_va_end(args);
}


LIB_FUNC void DllMainCRTStartup()
{
    
}
