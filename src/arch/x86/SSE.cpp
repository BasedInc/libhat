#include <libhat/Defines.hpp>

#ifdef LIBHAT_X86

#include <libhat/Scanner.hpp>

#include <immintrin.h>

namespace hat::detail {

    template<>
    scan_result find_pattern<scan_mode::SSE>(const std::byte* begin, const std::byte* end, signature_view signature) {
        // 256 bit vector containing first signature byte repeated
        const auto firstByte = _mm_set1_epi8(static_cast<int8_t>(*signature[0]));

        std::byte byteBuffer[16]{}; // The remaining signature bytes
        std::byte maskBuffer[16]{}; // A bitmask for the signature bytes we care about
        for (size_t i = 1; i < signature.size(); i++) {
            auto e = signature[i];
            if (e.has_value()) {
                byteBuffer[i - 1] = *e;
                maskBuffer[i - 1] = std::byte{0xFFu};
            }
        }

        const auto signatureBytes = _mm_loadu_si128(reinterpret_cast<__m128i*>(&byteBuffer));
        const auto signatureMask = _mm_loadu_si128(reinterpret_cast<__m128i*>(&maskBuffer));

        auto vec = reinterpret_cast<const __m128i*>(begin);
        const auto n = static_cast<size_t>(end - signature.size() - begin) / sizeof(__m128i);
        const auto e = vec + n;

        for (; vec != e; vec++) {
            const auto cmp = _mm_cmpeq_epi8(firstByte, *vec);
            auto mask = static_cast<uint32_t>(_mm_movemask_epi8(cmp));
            while (mask) {
                const auto offset = _tzcnt_u32(mask);
                const auto i = reinterpret_cast<const std::byte*>(vec) + offset;
                const auto data = _mm_loadu_si128(reinterpret_cast<const __m128i*>(i + 1));
                const auto cmpToSig = _mm_cmpeq_epi8(signatureBytes, data);
                const auto matched = _mm_testc_si128(cmpToSig, signatureMask);
                if (matched) {
                    return i;
                }
                mask = _blsr_u32(mask);
            }
        }

        // Look in remaining bytes that couldn't be grouped into 128 bits
        begin = reinterpret_cast<const std::byte*>(vec);
        return find_pattern<scan_mode::Single>(begin, end, signature);
    }
}
#endif
