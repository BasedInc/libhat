#pragma once

#include "scan_context.hpp"
#include "../utils.hpp"

namespace hat::detail {

    struct simd_context {
        std::size_t cmpIndex{};
    };

    template<size_t width, auto impl>
    scan_context create_simd_scanner(const scan_parameters& params) {
        const bool veccmp = params.signature.size() <= width;
        auto cmpIndex = get_optimal_pair(params);
        const bool cmpeq2 = cmpIndex.has_value();
        if (!cmpIndex) cmpIndex = get_optimal_byte(params);

        if (!cmpIndex) LIBHAT_UNLIKELY {
            // Highly unlikely case (no elements with a 0xFF mask), but it is supported by the 'Single' implementation.
            return create_context<hat::detail::scan_mode::Single>(params);
        }

        const auto resolve = [&]<scan_alignment A>(std::integral_constant<scan_alignment, A>) {
            if (cmpeq2 && veccmp) return impl.template operator()<A, true, true>();
            if (cmpeq2) return impl.template operator()<A, true, false>();
            if (veccmp) return impl.template operator()<A, false, true>();
            return impl.template operator()<A, false, false>();
        };

        const auto create = [&]<scan_alignment A>(std::integral_constant<scan_alignment, A> a) {
            if constexpr (to_stride(A) >= width) {
                return create_context<scan_mode::Single>(params);
            } else {
                return scan_context{params.signature, resolve(a), std::type_identity<simd_context>{}, *cmpIndex};
            }
        };

        switch (params.alignment) {
            using enum scan_alignment;
            case X1: return create(std::integral_constant<scan_alignment, X1>{});
            case X4: return create(std::integral_constant<scan_alignment, X4>{});
            case X16: return create(std::integral_constant<scan_alignment, X16>{});
        }
        LIBHAT_UNREACHABLE();
    }

    template<typename Vector, std::size_t alignment, bool veccmp>
    LIBHAT_FORCEINLINE auto segment_scan(
        const std::byte* begin,
        const std::byte* end,
        const std::size_t signatureSize,
        const std::size_t cmpOffset
    ) -> std::tuple<std::span<const std::byte>, std::span<const Vector>, std::span<const std::byte>> {
        // Alignment may not match due to function-targeted architecture flags
        // The size should though...
        static_assert(sizeof(Vector) == alignment);

        auto validateRange = [signatureSize](const std::byte* b, const std::byte* e) -> std::span<const std::byte> {
            if (b <= e && static_cast<std::size_t>(e - b) >= signatureSize) {
                return {b, e};
            }
            return {};
        };

        const auto preBegin = begin;
        const auto vecBegin = reinterpret_cast<const Vector*>(align_up<alignment>(preBegin + cmpOffset));
        if (reinterpret_cast<const std::byte*>(vecBegin) > end) LIBHAT_UNLIKELY {
            return {validateRange(begin, end), {}, {}};
        }

        const std::size_t vecAvailable = static_cast<std::size_t>(end - reinterpret_cast<const std::byte*>(vecBegin));
        const std::size_t requiredAfter = veccmp ? sizeof(Vector) : signatureSize;
        const auto vecEnd = vecBegin + (vecAvailable >= requiredAfter ? (vecAvailable - requiredAfter) / sizeof(Vector) : 0);

        // If the scan can't be vectorized, just do the single byte scanner "pre" part
        if (vecBegin == vecEnd) LIBHAT_UNLIKELY {
            return {validateRange(begin, end), {}, {}};
        }

        const auto preEnd = reinterpret_cast<const std::byte*>(vecBegin) - cmpOffset + signatureSize;
        const auto postBegin = reinterpret_cast<const std::byte*>(vecEnd) - cmpOffset;
        const auto postEnd = end;

        return {
            validateRange(preBegin, preEnd),
            std::span{vecBegin, vecEnd},
            validateRange(postBegin, postEnd)
        };
    }
}
