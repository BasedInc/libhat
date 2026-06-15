#include <libhat/defines.hpp>

#if defined(LIBHAT_ARM) || defined(LIBHAT_AARCH64)

#include <libhat/scanner.hpp>

#include <sse2neon.h>

#ifdef _MSC_VER
    namespace hat::detail {
        inline unsigned long bsf(unsigned __int64 num) noexcept {
            unsigned long offset;
            _BitScanForward64(&offset, num);
            return offset;
        }
    }

#define LIBHAT_BSF64(num) hat::detail::bsf(num)
#else
#define LIBHAT_BSF64(num) __builtin_ctzll(num)
#endif

namespace hat::detail {

    inline void load_signature_128(const signature_view signature, uint8x16_t& bytes, uint8x16_t& mask) {
        std::byte byteBuffer[16]{}; // The remaining signature bytes
        std::byte maskBuffer[16]{}; // A bitmask for the signature bytes we care about
        for (size_t i = 0; i < signature.size(); i++) {
            byteBuffer[i] = signature[i].value();
            maskBuffer[i] = signature[i].mask();
        }
        bytes = vld1q_u8(&byteBuffer);
        mask = vld1q_u8(&maskBuffer);
    }

    template<scan_alignment alignment>
    LIBHAT_FORCEINLINE consteval uint64_t create_alignment_mask_neon() {
        uint64_t mask{};
        for (size_t i = 0; i < 16; i += alignment_stride<alignment>) {
            mask |= (static_cast<uint64_t>(0xF) << (i * 4));
        }
        return mask;
    }

    template<scan_alignment alignment, bool cmpeq2, bool veccmp>
    const_scan_result find_pattern_neon(const std::byte* begin, const std::byte* end, const scan_context& context) {
        const auto signature = context.signature;
        const auto cmpIndex = cmpeq2 ? *context.pairIndex : context.cmpIndex;

        // 128 bit vector containing first signature byte repeated
        const auto firstByte = vdupq_n_u8(static_cast<int8_t>(*signature[cmpIndex]));

        uint8x16_t secondByte;
        if constexpr (cmpeq2) {
            secondByte = vdupq_n_u8(static_cast<int8_t>(*signature[cmpIndex + 1]));
        }

        uint8x16_t signatureBytes, signatureMask;
        if constexpr (veccmp) {
            load_signature_128(signature, signatureBytes, signatureMask);
        }

        auto [pre, vec, post] = segment_scan<uint8x16_t, veccmp>(begin, end, signature.size(), cmpIndex);

        if (!pre.empty()) {
            const auto result = find_pattern_single<alignment>(pre.data(), pre.data() + pre.size(), context);
            if (result.has_result()) {
                return result;
            }
        }

        for (auto& it : vec) {
            auto cmp = vceqq_u8(firstByte, vld1q_s8(&it));

            if constexpr (cmpeq2) {
                const auto cmp2 = vceqq_u8(secondByte, vld1q_s8(reinterpret_cast<const std::byte*>(&it) + 1));
                cmp = vandq_u8(cmp, cmp2);
            }

            auto mask = std::bit_cast<uint64_t>(vshrn_n_u16(cmp, 4));
            if constexpr (alignment != scan_alignment::X1) {
                mask &= std::rotl(create_alignment_mask_neon<alignment>(), static_cast<int>(cmpIndex) * 4);
            }

            while (mask) {
                const auto offset = LIBHAT_BSF64(mask) / 4;
                const auto i = reinterpret_cast<const std::byte*>(&it) + offset - cmpIndex;
                if constexpr (veccmp) {
                    const auto data = vld1q_u8(i);
                    const auto neqBits = veorq_u8(data, signatureBytes);
                    const auto match = _mm_testz_si128(neqBits, signatureMask);
                    if (match) LIBHAT_UNLIKELY {
                        return i;
                    }
                } else {
                    const auto match = std::equal(signature.begin(), signature.end(), i);
                    if (match) LIBHAT_UNLIKELY {
                        return i;
                    }
                }
                mask &= ~(0xF * (mask & -static_cast<int64_t>(mask)));
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

        if (alignment == scan_alignment::X1) {
            if (cmpeq2 && veccmp) {
                return &find_pattern_neon<scan_alignment::X1, true, true>;
            } else if (cmpeq2) {
                return &find_pattern_neon<scan_alignment::X1, true, false>;
            } else if (veccmp) {
                return &find_pattern_neon<scan_alignment::X1, false, true>;
            } else {
                return &find_pattern_neon<scan_alignment::X1, false, false>;
            }
        } else if (alignment == scan_alignment::X16) {
            if (cmpeq2 && veccmp) {
                return &find_pattern_neon<scan_alignment::X16, true, true>;
            } else if (cmpeq2) {
                return &find_pattern_neon<scan_alignment::X16, true, false>;
            } else if (veccmp) {
                return &find_pattern_neon<scan_alignment::X16, false, true>;
            } else {
                return &find_pattern_neon<scan_alignment::X16, false, false>;
            }
        }
        LIBHAT_UNREACHABLE();
    }
}
#endif
