#include <libhat/Defines.hpp>

#ifdef LIBHAT_X86

#include <libhat/Scanner.hpp>

#include <immintrin.h>
#include <tuple>

namespace hat::detail {

    inline auto load_signature_256(signature_view signature) {
        std::byte byteBuffer[32]{}; // The remaining signature bytes
        std::byte maskBuffer[32]{}; // A bitmask for the signature bytes we care about
        for (size_t i = 1; i < signature.size(); i++) {
            auto e = signature[i];
            if (e.has_value()) {
                byteBuffer[i - 1] = *e;
                maskBuffer[i - 1] = std::byte{0xFFu};
            }
        }
        return std::make_tuple(
            _mm256_loadu_si256(reinterpret_cast<__m256i*>(&byteBuffer)),
            _mm256_loadu_si256(reinterpret_cast<__m256i*>(&maskBuffer))
        );
    }

    template<scan_alignment alignment, bool cmpeq2, bool veccmp>
    scan_result find_pattern_avx2(const scan_context& context) {
        auto [begin, end, signature, hints] = context;

        // 256 bit vector containing first signature byte repeated
        const auto firstByte = _mm256_set1_epi8(static_cast<int8_t>(*signature[0]));

        __m256i secondByte;
        if constexpr (cmpeq2) {
            secondByte = _mm256_set1_epi8(static_cast<int8_t>(*signature[1]));
        }

        __m256i signatureBytes, signatureMask;
        if constexpr (veccmp) {
            std::tie(signatureBytes, signatureMask) = load_signature_256(signature);
        }

        begin = next_boundary_align<alignment>(begin);
        if (begin >= end) {
            return {};
        }

        auto vec = reinterpret_cast<const __m256i*>(begin);
        const auto n = static_cast<size_t>(end - signature.size() - begin) / sizeof(__m256i);
        const auto e = vec + n;

        for (; vec != e; vec++) {
            const auto cmp = _mm256_cmpeq_epi8(firstByte, _mm256_loadu_si256(vec));
            auto mask = static_cast<uint32_t>(_mm256_movemask_epi8(cmp));

            if constexpr (alignment != scan_alignment::X1) {
                mask &= create_alignment_mask<uint32_t, alignment>();
                if (!mask) continue;
            } else if constexpr (cmpeq2) {
                const auto cmp2 = _mm256_cmpeq_epi8(secondByte, _mm256_loadu_si256(vec));
                auto mask2 = static_cast<uint32_t>(_mm256_movemask_epi8(cmp2));
                // avoid loading unaligned memory by letting a match of the first signature byte in the last
                // position imply that the second byte also matched
                mask &= (mask2 >> 1) | (0b1u << 31);
            }

            while (mask) {
                const auto offset = _tzcnt_u32(mask);
                const auto i = reinterpret_cast<const std::byte*>(vec) + offset;
                if constexpr (veccmp) {
                    const auto data = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(i + 1));
                    const auto cmpToSig = _mm256_cmpeq_epi8(signatureBytes, data);
                    const auto matched = _mm256_testc_si256(cmpToSig, signatureMask);
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
                mask = _blsr_u32(mask);
            }
        }

        // Look in remaining bytes that couldn't be grouped into 256 bits
        begin = reinterpret_cast<const std::byte*>(vec);
        return find_pattern<scan_mode::Single, alignment>({begin, end, signature, hints});
    }

    template<scan_alignment alignment>
    scan_result find_pattern_avx2(const scan_context& context) {
        auto& signature = context.signature;
        const bool cmpeq2 = alignment == scan_alignment::X1 && signature.size() > 1 && signature[1].has_value();
        const bool veccmp = signature.size() <= 33;

        if (cmpeq2 && veccmp) {
            return find_pattern_avx2<alignment, true, true>(context);
        } else if (cmpeq2) {
            return find_pattern_avx2<alignment, true, false>(context);
        } else if (veccmp) {
            return find_pattern_avx2<alignment, false, true>(context);
        } else {
            return find_pattern_avx2<alignment, false, false>(context);
        }
    }

    template<>
    scan_result find_pattern<scan_mode::AVX2, scan_alignment::X1>(const scan_context& context) {
        return find_pattern_avx2<scan_alignment::X1>(context);
    }

    template<>
    scan_result find_pattern<scan_mode::AVX2, scan_alignment::X16>(const scan_context& context) {
        return find_pattern_avx2<scan_alignment::X16>(context);
    }
}
#endif
