#include <libhat/Defines.hpp>

#if defined(LIBHAT_X86) && !defined(LIBHAT_DISABLE_SSE)

#include <libhat/Scanner.hpp>

#include <immintrin.h>
#include <tuple>

namespace hat::detail {

    inline auto load_signature_128(signature_view signature) {
        std::byte byteBuffer[16]{}; // The remaining signature bytes
        std::byte maskBuffer[16]{}; // A bitmask for the signature bytes we care about
        for (size_t i = 1; i < signature.size(); i++) {
            auto e = signature[i];
            if (e.has_value()) {
                byteBuffer[i - 1] = *e;
                maskBuffer[i - 1] = std::byte{0xFFu};
            }
        }
        return std::make_tuple(
            _mm_loadu_si128(reinterpret_cast<__m128i*>(&byteBuffer)),
            _mm_loadu_si128(reinterpret_cast<__m128i*>(&maskBuffer))
        );
    }

    template<scan_alignment alignment, bool cmpeq2, bool veccmp>
    const_scan_result find_pattern_sse(const std::byte* begin, const std::byte* end, const scan_context& context) {
        const auto signature = context.signature;

        // 256 bit vector containing first signature byte repeated
        const auto firstByte = _mm_set1_epi8(static_cast<int8_t>(*signature[0]));

        __m128i secondByte;
        if constexpr (cmpeq2) {
            secondByte = _mm_set1_epi8(static_cast<int8_t>(*signature[1]));
        }

        __m128i signatureBytes, signatureMask;
        if constexpr (veccmp) {
            std::tie(signatureBytes, signatureMask) = load_signature_128(signature);
        }

        begin = next_boundary_align<alignment>(begin);
        if (begin >= end) {
            return {};
        }

        auto vec = reinterpret_cast<const __m128i*>(begin);
        const auto n = static_cast<size_t>(end - signature.size() - begin) / sizeof(__m128i);
        const auto e = vec + n;

        for (; vec != e; vec++) {
            const auto cmp = _mm_cmpeq_epi8(firstByte, _mm_loadu_si128(vec));
            auto mask = static_cast<uint16_t>(_mm_movemask_epi8(cmp));

            if constexpr (alignment != scan_alignment::X1) {
                mask &= create_alignment_mask<uint16_t, alignment>();
                if (!mask) continue;
            } else if constexpr (cmpeq2) {
                const auto cmp2 = _mm_cmpeq_epi8(secondByte, _mm_loadu_si128(vec));
                auto mask2 = static_cast<uint16_t>(_mm_movemask_epi8(cmp2));
                mask &= (mask2 >> 1) | (0b1u << 15);
            }

            while (mask) {
                const auto offset = LIBHAT_BSF32(mask);
                const auto i = reinterpret_cast<const std::byte*>(vec) + offset;
                if constexpr (veccmp) {
                    const auto data = _mm_loadu_si128(reinterpret_cast<const __m128i*>(i + 1));
                    const auto cmpToSig = _mm_cmpeq_epi8(signatureBytes, data);
                    const auto matched = _mm_testc_si128(cmpToSig, signatureMask);
                    if (matched) {
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
                mask &= (mask - 1);
            }
        }

        // Look in remaining bytes that couldn't be grouped into 128 bits
        begin = reinterpret_cast<const std::byte*>(vec);
        return find_pattern_single<alignment>(begin, end, context);
    }

    template<>
    scan_function_t get_scanner<scan_mode::SSE>(const scan_context& context) {
        const auto alignment = context.alignment;
        const auto signature = context.signature;
        const bool veccmp = signature.size() <= 17;

        if (alignment == scan_alignment::X1) {
            const bool cmpeq2 = signature.size() > 1 && signature[1].has_value();
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
        std::unreachable();
    }
}
#endif
