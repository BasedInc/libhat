#include <libhat/Scanner.hpp>

#include <libhat/Defines.hpp>
#include <libhat/System.hpp>

namespace hat::detail {

    template<scan_alignment alignment>
    scan_result find_pattern(const scan_context& context) {
#if defined(LIBHAT_X86)
        const auto& ext = get_system().extensions;
        if (ext.bmi) {
#if !defined(LIBHAT_DISABLE_AVX512)
            if (ext.avx512f && ext.avx512bw) {
                return find_pattern<scan_mode::AVX512, alignment>(context);
            }
#endif
            if (ext.avx2) {
                return find_pattern<scan_mode::AVX2, alignment>(context);
            }
        }
#if !defined(LIBHAT_DISABLE_SSE)
        if (ext.sse41) {
            return find_pattern<scan_mode::SSE, alignment>(context);
        }
#endif
#endif
        // If none of the vectorized implementations are available/supported, then fallback to scanning per-byte
        return find_pattern<scan_mode::Single, alignment>(context);
    }

    template scan_result find_pattern<scan_alignment::X1>(const scan_context& context);
    template scan_result find_pattern<scan_alignment::X16>(const scan_context& context);
}
