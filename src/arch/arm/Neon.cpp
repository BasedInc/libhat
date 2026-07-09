#include <libhat/defines.hpp>

#if defined(LIBHAT_ARM) || defined(LIBHAT_AARCH64)

#include <libhat/scanner.hpp>

#include "../../Utils.hpp"

#include <arm_neon.h>

#ifdef _MSC_VER
    #include <intrin.h>

    static unsigned long bsf64(const unsigned __int64 num) noexcept {
        unsigned long offset;
        _BitScanForward64(&offset, num);
        return offset;
    }

    #define LIBHAT_BSF64(num) bsf64(num)
#else
    #define LIBHAT_BSF64(num) __builtin_ctzll(num)
#endif

#ifdef LIBHAT_AARCH64
    #define LIBHAT_TEST_ZERO(x) (vmaxvq_u32(vreinterpretq_u32_u8(x)) == 0)
#else
    #define LIBHAT_TEST_ZERO(x) (!(vgetq_lane_u64(vreinterpretq_u64_u8(x), 0) | vgetq_lane_u64(vreinterpretq_u64_u8(x), 1)))
#endif

namespace hat::detail {

    static void load_signature_128(const signature_view signature, uint8x16_t& bytes, uint8x16_t& mask) {
        std::uint8_t byteBuffer[16]{}; // The remaining signature bytes
        std::uint8_t maskBuffer[16]{}; // A bitmask for the signature bytes we care about
        for (std::size_t i = 0; i < signature.size(); i++) {
            byteBuffer[i] = std::to_integer<std::uint8_t>(signature[i].value());
            maskBuffer[i] = std::to_integer<std::uint8_t>(signature[i].mask());
        }
        bytes = vld1q_u8(static_cast<const std::uint8_t*>(byteBuffer));
        mask = vld1q_u8(static_cast<const std::uint8_t*>(maskBuffer));
    }

    template<scan_alignment alignment>
    LIBHAT_FORCEINLINE consteval std::uint64_t create_alignment_mask_neon() {
        std::uint64_t mask{};
        for (std::size_t i = 0; i < 16; i += alignment_stride<alignment>) {
            mask |= (static_cast<std::uint64_t>(0xF) << (i * 4));
        }
        return mask;
    }

    template<scan_alignment alignment, bool cmpeq2, bool veccmp>
    static const_scan_result find_pattern_neon(const std::byte* begin, const std::byte* end, const scan_context& context) {
        const auto signature = context.signature;
        const auto cmpIndex = cmpeq2 ? *context.pairIndex : context.cmpIndex;

        // 128 bit vector containing first signature byte repeated
        const auto firstByte = vdupq_n_u8(static_cast<std::uint8_t>(*signature[cmpIndex]));

        uint8x16_t secondByte;
        if constexpr (cmpeq2) {
            secondByte = vdupq_n_u8(static_cast<std::uint8_t>(*signature[cmpIndex + 1]));
        }

        uint8x16_t signatureBytes, signatureMask;
        if constexpr (veccmp) {
            load_signature_128(signature, signatureBytes, signatureMask);
        }

        auto [pre, vec, post] = segment_scan<uint8x16_t, 16, veccmp>(begin, end, signature.size(), cmpIndex);

        if (!pre.empty()) {
            const auto result = find_pattern_single<alignment>(pre.data(), pre.data() + pre.size(), context);
            if (result.has_result()) {
                return result;
            }
        }

        const auto vec_begin = std::to_address(vec.begin());
        const auto vec_end = std::to_address(vec.end());
        for (auto it = vec_begin; it != vec_end; it++) {
            auto cmp = vceqq_u8(firstByte, vld1q_u8(reinterpret_cast<const std::uint8_t*>(it)));

            if constexpr (cmpeq2) {
                const auto cmp2 = vceqq_u8(secondByte, vld1q_u8(reinterpret_cast<const std::uint8_t*>(it) + 1));
                cmp = vandq_u8(cmp, cmp2);
            }

            auto mask = vget_lane_u64(vreinterpret_u64_u8(vshrn_n_u16(vreinterpretq_u16_u8(cmp), 4)),  0);
            if constexpr (alignment != scan_alignment::X1) {
                mask &= std::rotl(create_alignment_mask_neon<alignment>(), static_cast<int>(cmpIndex) * 4);
            }

            while (mask) {
                const auto offset = LIBHAT_BSF64(mask);
                const auto i = reinterpret_cast<const std::byte*>(it) + (offset >> 2) - cmpIndex;
                if constexpr (veccmp) {
                    const auto data = vld1q_u8(reinterpret_cast<const std::uint8_t*>(i));
                    const auto neqBits = veorq_u8(data, signatureBytes);
                    const auto match = vandq_u8(neqBits, signatureMask);
                    if (LIBHAT_TEST_ZERO(match)) LIBHAT_UNLIKELY {
                        return i;
                    }
                } else {
                    const auto match = std::equal(signature.begin(), signature.end(), i);
                    if (match) LIBHAT_UNLIKELY {
                        return i;
                    }
                }
                // thanks msvc?
                // mask &= ~(0xF * (mask & (~mask + 1)));
                mask ^= (std::uint64_t{0xF} << offset);
            }
        }

        if (!post.empty()) {
            return find_pattern_single<alignment>(post.data(), post.data() + post.size(), context);
        }
        return {};
    }

    template<>
    scan_function_t resolve_scanner<scan_mode::Neon>(scan_context& context) {
        context.apply_hints({.vectorSize = 16});

        const auto alignment = context.alignment;
        const auto signature = context.signature;
        const bool cmpeq2 = context.pairIndex.has_value();
        const bool veccmp = signature.size() <= 16;

        return find_specialization_switch<[]<auto... p>() consteval {
            return &find_pattern_neon<p...>;
        }>(alignment, cmpeq2, veccmp);
    }
}
#endif
