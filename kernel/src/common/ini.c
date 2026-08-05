
#include <common/ini.h>
#include <common/string.h>

sList ParseINIFile(PCHAR szContents)
{
    sList lstEntries = CreateEmptyList(sizeof(sINIEntry));

    WORD wLabelBufferOffset = 0, wNameBufferOffset = 0, wValueBufferOffset = 0;
    CHAR szLabelBuffer[256];
    sINIEntry sEntry;
    for (QWORD i = 0; szContents[i]; )
    {
        switch (szContents[i])
        {
        case ' ': case '\n': case '\r': case '\t':
            i++;
            break;
        case ';': case '#':
            while (szContents[++i] != '\n');
            i++; // Skip the line feed character
            break;

        case '[':
            i++;
            wLabelBufferOffset = 0;
            while (szContents[i] != ']')
                szLabelBuffer[wLabelBufferOffset++] = szContents[i++];
            i++; // Skip ]
            szLabelBuffer[wLabelBufferOffset] = 0;
            StripString(szLabelBuffer);
            break;
        default:
            wNameBufferOffset = wValueBufferOffset = 0;
            // Get the name
            while (szContents[i] != '=')
                sEntry.szName[wNameBufferOffset++] = szContents[i++];
            sEntry.szName[wNameBufferOffset] = 0;
            StripString(sEntry.szName);

            i++; // Skip =

            // Get the value
            while (szContents[i] != '\n')
                sEntry.szValue[wValueBufferOffset++] = szContents[i++];
            sEntry.szValue[wValueBufferOffset] = 0;
            StripString(sEntry.szValue);

            strcpy(sEntry.szLabel, szLabelBuffer);
            AddListElement(&lstEntries, &sEntry);
            break;
        }
    }

    return lstEntries;
}
