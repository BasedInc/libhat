#include <libhat/defines.hpp>

#if defined(LIBHAT_X86) || defined(LIBHAT_X86_64)

#include <libhat/scanner.hpp>

#include "../../Utils.hpp"

#include <immintrin.h>

namespace hat::detail {

    LIBHAT_TARGET("avx")
    static void load_signature_256(const signature_view signature, __m256i& bytes, __m256i& mask) {
        std::byte byteBuffer[32]{}; // The remaining signature bytes
        std::byte maskBuffer[32]{}; // A bitmask for the signature bytes we care about
        for (std::size_t i = 0; i < signature.size(); i++) {
            byteBuffer[i] = signature[i].value();
            maskBuffer[i] = signature[i].mask();
        }
        bytes = _mm256_loadu_si256(reinterpret_cast<__m256i*>(&byteBuffer));
        mask = _mm256_loadu_si256(reinterpret_cast<__m256i*>(&maskBuffer));
    }

    template<scan_alignment alignment, bool cmpeq2, bool veccmp>
    LIBHAT_TARGET("avx,avx2,bmi")
    static const_scan_result find_pattern_avx2(const std::byte* begin, const std::byte* end, const scan_context& context) {
        const auto signature = context.signature;
        const auto cmpIndex = cmpeq2 ? *context.pairIndex : context.cmpIndex;

        // 256 bit vector containing first signature byte repeated
        const auto firstByte = _mm256_set1_epi8(static_cast<std::int8_t>(*signature[cmpIndex]));

        __m256i secondByte;
        if constexpr (cmpeq2) {
            secondByte = _mm256_set1_epi8(static_cast<std::int8_t>(*signature[cmpIndex + 1]));
        }

        __m256i signatureBytes, signatureMask;
        if constexpr (veccmp) {
            load_signature_256(signature, signatureBytes, signatureMask);
        }

        auto [pre, vec, post] = segment_scan<__m256i, 32, veccmp>(begin, end, signature.size(), cmpIndex);

        if (!pre.empty()) {
            const auto result = find_pattern_single<alignment>(pre.data(), pre.data() + pre.size(), context);
            if (result.has_result()) {
                return result;
            }
        }

        const auto vec_begin = std::to_address(vec.begin());
        const auto vec_end = std::to_address(vec.end());
        for (auto it = vec_begin; it != vec_end; it++) {
            const auto cmp = _mm256_cmpeq_epi8(firstByte, _mm256_load_si256(it));
            auto mask = static_cast<std::uint32_t>(_mm256_movemask_epi8(cmp));

            if constexpr (cmpeq2) {
                const auto cmp2 = _mm256_cmpeq_epi8(secondByte, _mm256_load_si256(it));
                auto mask2 = static_cast<std::uint32_t>(_mm256_movemask_epi8(cmp2));
                // avoid loading unaligned memory by letting a match of the first signature byte in the last
                // position imply that the second byte also matched
                mask &= (mask2 >> 1) | (0b1u << 31);
            }

            if constexpr (alignment != scan_alignment::X1) {
                mask &= std::rotl(create_alignment_mask<std::uint32_t, alignment>(), static_cast<int>(cmpIndex));
                if (!mask) continue;
            }

            while (mask) {
                const auto offset = _tzcnt_u32(mask);
                const auto i = reinterpret_cast<const std::byte*>(it) + offset - cmpIndex;
                if constexpr (veccmp) {
                    const auto data = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(i));
                    const auto neqBits = _mm256_xor_si256(data, signatureBytes);
                    const auto match = _mm256_testz_si256(neqBits, signatureMask);
                    if (match) LIBHAT_UNLIKELY {
                        return i;
                    }
                } else {
                    const auto match = std::equal(signature.begin(), signature.end(), i);
                    if (match) LIBHAT_UNLIKELY {
                        return i;
                    }
                }
                mask = _blsr_u32(mask);
            }
        }

        if (!post.empty()) {
            return find_pattern_single<alignment>(post.data(), post.data() + post.size(), context);
        }
        return {};
    }

    template<>
    scan_function_t resolve_scanner<scan_mode::AVX2>(scan_context& context) {
        context.apply_hints({.vectorSize = 32});

        const auto alignment = context.alignment;
        const auto signature = context.signature;
        const bool cmpeq2 = context.pairIndex.has_value();
        const bool veccmp = signature.size() <= 32;

        return find_specialization_switch<[]<auto... p>() consteval {
            return &find_pattern_avx2<p...>;
        }>(alignment, cmpeq2, veccmp);
    }
}
#endif
