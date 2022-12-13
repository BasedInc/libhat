#include <libhat/Defines.hpp>
#ifdef LIBHAT_X86

#include <libhat/Scanner.hpp>

#include <immintrin.h>

namespace hat::detail {

    template<>
    scan_result find_pattern<scan_mode::AVX512>(std::byte* begin, std::byte* end, signature_view signature) {
        // 512 bit vector containing first signature byte repeated
        const auto firstByte = _mm512_set1_epi8(static_cast<int8_t>(*signature[0]));

        std::byte byteBuffer[64]{}; // The remaining signature bytes
        uint64_t maskBuffer{}; // A bitmask for the signature bytes we care about
        for (size_t i = 1; i < signature.size(); i++) {
            auto e = signature[i];
            if (e.has_value()) {
                byteBuffer[i - 1] = *e;
                maskBuffer |= (1ull << (i - 1));
            }
        }

        const auto signatureBytes = _mm512_loadu_si512(&byteBuffer);
        const auto signatureMask = _cvtu64_mask64(maskBuffer);

        auto vec = reinterpret_cast<__m512i*>(begin);
        const auto n = static_cast<size_t>(end - signature.size() - begin) / sizeof(__m512i);
        const auto e = vec + n;

        for (; vec != e; vec++) {
            auto mask = _mm512_cmpeq_epi8_mask(firstByte, *vec);
            while (mask) {
                const auto offset = LIBHAT_TZCNT64(mask);
                const auto i = reinterpret_cast<std::byte*>(vec) + offset;
                const auto data = _mm512_loadu_si512(i + 1);
                const auto invalid = _mm512_mask_cmpneq_epi8_mask(signatureMask, signatureBytes, data);
                if (!invalid) {
                    return i;
                }
                mask = LIBHAT_BLSR64(mask);
            }
        }

        // Look in remaining bytes that couldn't be grouped into 512 bits
        begin = reinterpret_cast<std::byte*>(vec);
        return find_pattern<scan_mode::FastFirst>(begin, end, signature);
    }
}
#endif
