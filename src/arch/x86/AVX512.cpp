#include <libhat/defines.hpp>

#if defined(LIBHAT_X86_64) && !defined(LIBHAT_DISABLE_AVX512)

#include <libhat/scanner.hpp>

#include <immintrin.h>

namespace hat::detail {

    inline void load_signature_512(const signature_view signature, __m512i& bytes, __m512i& mask) {
        std::byte byteBuffer[64]{}; // The remaining signature bytes
        std::byte maskBuffer[64]{}; // A bitmask for the signature bytes we care about
        for (size_t i = 0; i < signature.size(); i++) {
            byteBuffer[i] = signature[i].value();
            maskBuffer[i] = signature[i].mask();
        }
        bytes = _mm512_loadu_si512(&byteBuffer);
        mask = _mm512_loadu_si512(&maskBuffer);
    }

    template<scan_alignment alignment, bool cmpeq2, bool veccmp>
    const_scan_result find_pattern_avx512(const std::byte* begin, const std::byte* end, const scan_context& context) {
        const auto signature = context.signature;
        const auto cmpIndex = cmpeq2 ? *context.pairIndex : 0;
        LIBHAT_ASSUME(cmpIndex < 64);

        // 512 bit vector containing first signature byte repeated
        const auto firstByte = _mm512_set1_epi8(static_cast<int8_t>(*signature[cmpIndex]));

        __m512i secondByte;
        if constexpr (cmpeq2) {
            secondByte = _mm512_set1_epi8(static_cast<int8_t>(*signature[cmpIndex + 1]));
        }

        __m512i signatureBytes;
        __m512i signatureMask;
        if constexpr (veccmp) {
            load_signature_512(signature, signatureBytes, signatureMask);
        }

        auto [pre, vec, post] = segment_scan<__m512i, veccmp>(begin, end, signature.size(), cmpIndex);

        if (!pre.empty()) {
            const auto result = find_pattern_single<alignment>(pre.data(), pre.data() + pre.size(), context);
            if (result.has_result()) {
                return result;
            }
        }

        for (auto& it : vec) {
            auto mask = _mm512_cmpeq_epi8_mask(firstByte, _mm512_loadu_si512(&it));

            if constexpr (alignment != scan_alignment::X1) {
                mask &= create_alignment_mask<uint64_t, alignment>();
                if (!mask) continue;
            } else if constexpr (cmpeq2) {
                const auto mask2 = _mm512_cmpeq_epi8_mask(secondByte, _mm512_loadu_si512(&it));
                mask &= (mask2 >> 1) | (0b1ull << 63);
            }

            while (mask) {
                const auto offset = _tzcnt_u64(mask);
                const auto i = reinterpret_cast<const std::byte*>(&it) + offset - cmpIndex;
                if constexpr (veccmp) {
                    const auto data = _mm512_loadu_si512(i);
                    const auto neqBits = _mm512_xor_si512(data, signatureBytes);
                    const auto invalid = _mm512_test_epi64_mask(neqBits, signatureMask);
                    if (!invalid) LIBHAT_UNLIKELY {
                        return i;
                    }
                } else {
                    const auto match = std::equal(signature.begin(), signature.end(), i);
                    if (match) LIBHAT_UNLIKELY {
                        return i;
                    }
                }
                mask = _blsr_u64(mask);
            }
        }

        if (!post.empty()) {
            return find_pattern_single<alignment>(post.data(), post.data() + post.size(), context);
        }
        return {};
    }

    template<>
    scan_function_t resolve_scanner<scan_mode::AVX512>(scan_context& context) {
        context.apply_hints({.vectorSize = 64});

        const auto alignment = context.alignment;
        const auto signature = context.signature;
        const bool veccmp = signature.size() <= 64;

        if (alignment == scan_alignment::X1) {
            const bool cmpeq2 = context.pairIndex.has_value();
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
