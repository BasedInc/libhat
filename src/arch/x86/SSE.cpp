#include <libhat/Defines.hpp>

#if (defined(LIBHAT_X86) || defined(LIBHAT_X86_64)) && !defined(LIBHAT_DISABLE_SSE)

#include <libhat/Scanner.hpp>

#include <immintrin.h>

namespace hat::detail {

    inline void load_signature_128(const signature_view signature, __m128i& bytes, __m128i& mask) {
        std::byte byteBuffer[16]{}; // The remaining signature bytes
        std::byte maskBuffer[16]{}; // A bitmask for the signature bytes we care about
        for (size_t i = 0; i < signature.size(); i++) {
            auto e = signature[i];
            if (e.has_value()) {
                byteBuffer[i] = *e;
                maskBuffer[i] = std::byte{0xFFu};
            }
        }
        bytes = _mm_loadu_si128(reinterpret_cast<__m128i*>(&byteBuffer));
        mask = _mm_loadu_si128(reinterpret_cast<__m128i*>(&maskBuffer));
    }

    template<scan_alignment alignment, bool cmpeq2, bool veccmp>
    const_scan_result find_pattern_sse(const std::byte* begin, const std::byte* end, const scan_context& context) {
        const auto signature = context.signature;
        const auto cmpIndex = cmpeq2 ? *context.pairIndex : 0;
        LIBHAT_ASSUME(cmpIndex < 16);

        // 128 bit vector containing first signature byte repeated
        const auto firstByte = _mm_set1_epi8(static_cast<int8_t>(*signature[cmpIndex]));

        __m128i secondByte;
        if constexpr (cmpeq2) {
            secondByte = _mm_set1_epi8(static_cast<int8_t>(*signature[cmpIndex + 1]));
        }

        __m128i signatureBytes, signatureMask;
        if constexpr (veccmp) {
            load_signature_128(signature, signatureBytes, signatureMask);
        }

        auto [pre, vec, post] = segment_scan<__m128i, veccmp>(begin, end, signature.size(), cmpIndex);

        if (!pre.empty()) {
            const auto result = find_pattern_single<alignment>(pre.data(), pre.data() + pre.size(), context);
            if (result.has_result()) {
                return result;
            }
        }

        for (auto& it : vec) {
            const auto cmp = _mm_cmpeq_epi8(firstByte, _mm_loadu_si128(&it));
            auto mask = static_cast<uint16_t>(_mm_movemask_epi8(cmp));

            if constexpr (alignment != scan_alignment::X1) {
                mask &= create_alignment_mask<uint16_t, alignment>();
                if (!mask) continue;
            } else if constexpr (cmpeq2) {
                const auto cmp2 = _mm_cmpeq_epi8(secondByte, _mm_loadu_si128(&it));
                auto mask2 = static_cast<uint16_t>(_mm_movemask_epi8(cmp2));
                mask &= (mask2 >> 1) | (0b1u << 15);
            }

            while (mask) {
                const auto offset = LIBHAT_BSF32(mask);
                const auto i = reinterpret_cast<const std::byte*>(&it) + offset - cmpIndex;
                if constexpr (veccmp) {
                    const auto data = _mm_loadu_si128(reinterpret_cast<const __m128i*>(i));
                    const auto cmpToSig = _mm_cmpeq_epi8(signatureBytes, data);
                    const auto matched = _mm_testc_si128(cmpToSig, signatureMask);
                    if (matched) LIBHAT_UNLIKELY {
                        return i;
                    }
                } else {
                    auto match = std::equal(signature.begin(), signature.end(), i, [](auto opt, auto byte) {
                        return !opt.has_value() || *opt == byte;
                    });
                    if (match) LIBHAT_UNLIKELY {
                        return i;
                    }
                }
                mask &= (mask - 1);
            }
        }

        if (!post.empty()) {
            return find_pattern_single<alignment>(post.data(), post.data() + post.size(), context);
        }
        return {};
    }

    template<>
    scan_function_t resolve_scanner<scan_mode::SSE>(scan_context& context) {
        context.apply_hints({.vectorSize = 16});

        const auto alignment = context.alignment;
        const auto signature = context.signature;
        const bool veccmp = signature.size() <= 16;

        if (alignment == scan_alignment::X1) {
            const bool cmpeq2 = context.pairIndex.has_value();
            if (cmpeq2 && veccmp) {
                return &find_pattern_sse<scan_alignment::X1, true, true>;
            } else if (cmpeq2) {
                return &find_pattern_sse<scan_alignment::X1, true, false>;
            } else if (veccmp) {
                return &find_pattern_sse<scan_alignment::X1, false, true>;
            } else {
                return &find_pattern_sse<scan_alignment::X1, false, false>;
            }
        } else if (alignment == scan_alignment::X16) {
            if (veccmp) {
                return &find_pattern_sse<scan_alignment::X16, false, true>;
            } else {
                return &find_pattern_sse<scan_alignment::X16, false, false>;
            }
        }
        LIBHAT_UNREACHABLE();
    }
}
#endif
