
#ifndef __NUMBERS_H
#define __NUMBERS_H

#define _BE2LEDW(n) (((n) & 0x000000FF) << 24 | ((n) & 0x0000FF00) << 8 | ((n) & 0x00FF0000) >> 8 | ((n) & 0xFF000000) >> 24)
#define _BE2LEW(n) (((n) & 0x00FF) << 8) | (((n) & 0xFF00) >> 8)
#define _BE2LE48(n) (((n) >> 40) & 0x0000000000FF) | \
                    (((n) >> 24) & 0x00000000FF00) | \
                    (((n) >> 8)  & 0x000000FF0000) | \
                    (((n) << 8)  & 0x0000FF000000) | \
                    (((n) << 24) & 0x00FF00000000) | \
                    (((n) << 40) & 0xFF0000000000)

#endif // __NUMBERS_H
