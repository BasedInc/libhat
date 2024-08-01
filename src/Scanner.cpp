#include <libhat/Scanner.hpp>

#include <libhat/Defines.hpp>
#include <libhat/System.hpp>

namespace hat::detail {

    void scan_context::apply_hints() {}

    std::pair<scan_function_t, size_t> resolve_scanner(const scan_context& context) {
#if defined(LIBHAT_X86)
        const auto& ext = get_system().extensions;
        if (ext.bmi) {
#if !defined(LIBHAT_DISABLE_AVX512)
            if (ext.avx512f && ext.avx512bw) {
                return {get_scanner<scan_mode::AVX512>(context), 64};
            }
#endif
            if (ext.avx2) {
                return {get_scanner<scan_mode::AVX2>(context), 32};
            }
        }
#if !defined(LIBHAT_DISABLE_SSE)
        if (ext.sse41) {
            return {get_scanner<scan_mode::SSE>(context), 16};
        }
#endif
#endif
        // If none of the vectorized implementations are available/supported, then fallback to scanning per-byte
        return {get_scanner<scan_mode::Single>(context), 0};
    }
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
