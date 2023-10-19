#include <libhat/Defines.hpp>

#ifdef LIBHAT_X86

#include <libhat/Scanner.hpp>

#include <immintrin.h>
#include <tuple>

namespace hat::detail {

    inline auto load_signature_512(signature_view signature) {
        std::byte byteBuffer[64]{}; // The remaining signature bytes
        uint64_t maskBuffer{}; // A bitmask for the signature bytes we care about
        for (size_t i = 1; i < signature.size(); i++) {
            auto e = signature[i];
            if (e.has_value()) {
                byteBuffer[i - 1] = *e;
                maskBuffer |= (1ull << (i - 1));
            }
        }
        return std::make_tuple(
            _mm512_loadu_si512(&byteBuffer),
            _cvtu64_mask64(maskBuffer)
        );
    }

    template<scan_alignment alignment, bool cmpeq2, bool veccmp>
    scan_result find_pattern_avx512(const std::byte* begin, const std::byte* end, signature_view signature) {
        // 512 bit vector containing first signature byte repeated
        const auto firstByte = _mm512_set1_epi8(static_cast<int8_t>(*signature[0]));

        __m512i secondByte;
        if constexpr (cmpeq2) {
            secondByte = _mm512_set1_epi8(static_cast<int8_t>(*signature[1]));
        }

        __m512i signatureBytes;
        uint64_t signatureMask;
        if constexpr (veccmp) {
            std::tie(signatureBytes, signatureMask) = load_signature_512(signature);
        }

        begin = next_boundary_align<alignment>(begin);
        if (begin >= end) {
            return {};
        }

        auto vec = reinterpret_cast<const __m512i*>(begin);
        const auto n = static_cast<size_t>(end - signature.size() - begin) / sizeof(__m512i);
        const auto e = vec + n;

        for (; vec != e; vec++) {
            auto mask = _mm512_cmpeq_epi8_mask(firstByte, _mm512_loadu_si512(vec));

            if constexpr (cmpeq2) {
                const auto mask2 = _mm512_cmpeq_epi8_mask(secondByte, _mm512_loadu_si512(vec));
                mask &= (mask2 >> 1) | (0b1ull << 63);
            }

            mask &= create_alignment_mask<uint64_t, alignment>();
            while (mask) {
                const auto offset = LIBHAT_TZCNT64(mask);
                const auto i = reinterpret_cast<const std::byte*>(vec) + offset;
                if constexpr (veccmp) {
                    const auto data = _mm512_loadu_si512(i + 1);
                    const auto invalid = _mm512_mask_cmpneq_epi8_mask(signatureMask, signatureBytes, data);
                    if (!invalid) {
                        return i;
                    }
                } else {
                    auto match = std::equal(signature.begin() + 1, signature.end(), i + 1, [](auto opt, auto byte) {
                        return !opt.has_value() || *opt == byte;
                    });
                    if (match) {
                        return i;
                    }
                }
                mask = LIBHAT_BLSR64(mask);
            }
        }

        // Look in remaining bytes that couldn't be grouped into 512 bits
        begin = reinterpret_cast<const std::byte*>(vec);
        return find_pattern<scan_mode::Single, alignment>(begin, end, signature);
    }

    template<scan_alignment alignment>
    scan_result find_pattern_avx512(const std::byte* begin, const std::byte* end, signature_view signature) {
        const bool cmpeq2 = signature.size() > 1 && signature[1].has_value();
        const bool veccmp = signature.size() <= 65;

        if (cmpeq2 && veccmp) {
            return find_pattern_avx512<alignment, true, true>(begin, end, signature);
        } else if (cmpeq2) {
            return find_pattern_avx512<alignment, true, false>(begin, end, signature);
        } else if (veccmp) {
            return find_pattern_avx512<alignment, false, true>(begin, end, signature);
        } else {
            return find_pattern_avx512<alignment, false, false>(begin, end, signature);
        }
    }

    template<>
    scan_result find_pattern<scan_mode::AVX512, scan_alignment::X1>(const std::byte* begin, const std::byte* end, signature_view signature) {
        return find_pattern_avx512<scan_alignment::X1>(begin, end, signature);
    }

    template<>
    scan_result find_pattern<scan_mode::AVX512, scan_alignment::X16>(const std::byte* begin, const std::byte* end, signature_view signature) {
        return find_pattern_avx512<scan_alignment::X16>(begin, end, signature);
    }
}
#endif
