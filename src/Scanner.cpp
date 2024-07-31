#include <libhat/Scanner.hpp>

#include <libhat/Defines.hpp>
#include <libhat/System.hpp>

namespace hat::detail {

    void scan_context::apply_hints([[maybe_unused]] const scan_alignment alignment) {}

    template<scan_alignment alignment>
    const_scan_result find_pattern(const std::byte* begin, const std::byte* end, const scan_context& context) {
#if defined(LIBHAT_X86)
        const auto& ext = get_system().extensions;
        if (ext.bmi) {
#if !defined(LIBHAT_DISABLE_AVX512)
            if (ext.avx512f && ext.avx512bw) {
                return find_pattern<scan_mode::AVX512, alignment>(begin, end, context);
            }
#endif
            if (ext.avx2) {
                return find_pattern<scan_mode::AVX2, alignment>(begin, end, context);
            }
        }
#if !defined(LIBHAT_DISABLE_SSE)
        if (ext.sse41) {
            return find_pattern<scan_mode::SSE, alignment>(begin, end, context);
        }
#endif
#endif
        // If none of the vectorized implementations are available/supported, then fallback to scanning per-byte
        return find_pattern<scan_mode::Single, alignment>(begin, end, context);
    }

    template const_scan_result find_pattern<scan_alignment::X1>(const std::byte* begin, const std::byte* end, const scan_context& context);
    template const_scan_result find_pattern<scan_alignment::X16>(const std::byte* begin, const std::byte* end, const scan_context& context);
}

// Validate return value const-ness for the root find_pattern impl
namespace hat {
    static_assert(std::is_same_v<scan_result, decltype(find_pattern(
        std::declval<std::byte*>(),
        std::declval<std::byte*>(),
        std::declval<signature_view>()))>);

    static_assert(std::is_same_v<const_scan_result, decltype(find_pattern(
        std::declval<const std::byte*>(),
        std::declval<const std::byte*>(),
        std::declval<signature_view>()))>);
}
