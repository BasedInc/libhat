#pragma once

#define INRANGE(x,a,b)  (x >= a && x <= b)
#define getBits( x )    (INRANGE((x&(~0x20)),'A','F') ? ((x&(~0x20)) - 'A' + 0xa) : (INRANGE(x,'0','9') ? x - '0' : 0))
#define getByte( x )    (getBits(x[0]) << 4 | getBits(x[1]))

// https://www.unknowncheats.me/forum/650040-post8.html
namespace UC2 {
    inline const std::byte* findPattern(const std::byte* rangeStart, const std::byte* rangeEnd, const char* pattern) {
        using PBYTE = uint8_t*;
        using BYTE = uint8_t;
        using PWORD = uint16_t*;

        const char* pat = pattern;
        const std::byte* firstMatch = nullptr;
        for (auto pCur = rangeStart; pCur < rangeEnd; pCur++) {
            if (!*pat) return firstMatch;
            if (*(PBYTE)pat == '\?' || *(BYTE*)pCur == getByte(pat)) {
                if (!firstMatch) firstMatch = pCur;
                if (!pat[2]) return firstMatch;
                if (*(PWORD)pat == '\?\?' || *(PBYTE)pat != '\?') pat += 3;
                else pat += 2; //one ?
            } else {
                pat = pattern;
                firstMatch = 0;
            }
        }
        return NULL;
    }
}

#undef INRANGE
#undef getBits
#undef getByte
