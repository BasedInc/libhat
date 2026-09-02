#include "scanner.hpp"

#define INRANGE(x,a,b)  (x >= a && x <= b)
#define getBits( x )    (INRANGE((x&(~0x20)),'A','F') ? ((x&(~0x20)) - 'A' + 0xa) : (INRANGE(x,'0','9') ? x - '0' : 0))
#define getByte( x )    (getBits(x[0]) << 4 | getBits(x[1]))

namespace {

    // Written by learn_more
    // https://www.unknowncheats.me/forum/650040-post8.html
    struct LearnMoreScanner : Scanner {

        std::string_view Name() override {
            return "learn_more";
        }

        void Setup(const hat::cstring_view signature) override {
            pattern_ = signature.c_str();
        }

        const std::byte* Find(const std::span<const std::byte> buffer) override {
            const auto* rangeStart = std::to_address(buffer.begin());
            const auto* rangeEnd = std::to_address(buffer.end());

            using PBYTE = uint8_t*;
            using BYTE = uint8_t;
            using PWORD = uint16_t*;

            const char* pat = pattern_;
            const std::byte* firstMatch = nullptr;
            for (auto pCur = rangeStart; pCur < rangeEnd; pCur++) {
                if (!*pat) return firstMatch;
                if (*(PBYTE)pat == '\?' || *(BYTE*)pCur == getByte(pat)) {
                    if (!firstMatch) firstMatch = pCur;
                    if (!pat[2]) return firstMatch;
                    if (*(PWORD)pat == '\?\?' || *(PBYTE)pat != '\?') pat += 3;
                    else pat += 2; //one ?
                } else {
                    pat = pattern_;
                    firstMatch = 0;
                }
            }
            return NULL;
        }

    private:
        const char* pattern_{};
    };

    REGISTER_SCANNER(LearnMoreScanner);
}
