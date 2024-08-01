#include <libhat/Defines.hpp>

#if defined(LIBHAT_X86) && !defined(LIBHAT_DISABLE_AVX512)

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
    const_scan_result find_pattern_avx512(const std::byte* begin, const std::byte* end, const scan_context& context) {
        const auto signature = context.signature;
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
        if (begin >= end) LIBHAT_UNLIKELY {
            return {};
        }

        auto vec = reinterpret_cast<const __m512i*>(begin);
        const auto n = static_cast<size_t>(end - signature.size() - begin) / sizeof(__m512i);
        const auto e = vec + n;

        for (; vec != e; vec++) {
            auto mask = _mm512_cmpeq_epi8_mask(firstByte, _mm512_loadu_si512(vec));

            if constexpr (alignment != scan_alignment::X1) {
                mask &= create_alignment_mask<uint64_t, alignment>();
                if (!mask) continue;
            } else if constexpr (cmpeq2) {
                const auto mask2 = _mm512_cmpeq_epi8_mask(secondByte, _mm512_loadu_si512(vec));
                mask &= (mask2 >> 1) | (0b1ull << 63);
            }

            while (mask) {
                const auto offset = LIBHAT_TZCNT64(mask);
                const auto i = reinterpret_cast<const std::byte*>(vec) + offset;
                if constexpr (veccmp) {
                    const auto data = _mm512_loadu_si512(i + 1);
                    const auto invalid = _mm512_mask_cmpneq_epi8_mask(signatureMask, signatureBytes, data);
                    if (!invalid) LIBHAT_UNLIKELY {
                        return i;
                    }
                } else {
                    auto match = std::equal(signature.begin() + 1, signature.end(), i + 1, [](auto opt, auto byte) {
                        return !opt.has_value() || *opt == byte;
                    });
                    if (match) LIBHAT_UNLIKELY {
                        return i;
                    }
                }
                mask = LIBHAT_BLSR64(mask);
            }
        }

        // Look in remaining bytes that couldn't be grouped into 512 bits
        begin = reinterpret_cast<const std::byte*>(vec);
        return find_pattern_single<alignment>(begin, end, context);
    }

    template<>
    scan_function_t get_scanner<scan_mode::AVX512>(const scan_context& context) {
        const auto alignment = context.alignment;
        const auto signature = context.signature;
        const bool veccmp = signature.size() <= 65;

        if (alignment == scan_alignment::X1) {
            const bool cmpeq2 = signature.size() > 1 && signature[1].has_value();
            if (cmpeq2 && veccmp) {
                return &find_pattern_avx512<scan_alignment::X1, true, true>;
            } else if (cmpeq2) {
                return &find_pattern_avx512<scan_alignment::X1, true, false>;
            } else if (veccmp) {
                return &find_pattern_avx512<scan_alignment::X1, false, true>;
            } else {
                return &find_pattern_avx512<scan_alignment::X1, false, false>;
            }
        } else if (alignment == scan_alignment::X16) {
            if (veccmp) {
                return &find_pattern_avx512<scan_alignment::X16, false, true>;
            } else {
                return &find_pattern_avx512<scan_alignment::X16, false, false>;
            }
        }
        LIBHAT_UNREACHABLE();
    }
}
#endif
