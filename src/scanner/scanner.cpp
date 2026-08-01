#include <libhat/scanner.hpp>

#include <libhat/defines.hpp>
#include <libhat/system.hpp>

namespace hat::detail {

    template<>
    scan_context create_context<scan_mode::Auto>(const scan_parameters& params) {
       const auto& ext = get_system().extensions;
#if defined(LIBHAT_X86) || defined(LIBHAT_X86_64)
       if (compiled_extensions.bmi || ext.bmi) {
#if defined(LIBHAT_X86_64) && defined(LIBHAT_FEATURE_AVX512)
           if ((compiled_extensions.avx512f || ext.avx512f)
               && (compiled_extensions.avx512bw || ext.avx512bw)) {
               return create_context<scan_mode::AVX512>(params);
           }
#endif
           if (compiled_extensions.avx2 || ext.avx2) {
               return create_context<scan_mode::AVX2>(params);
           }
       }
#if defined(LIBHAT_FEATURE_SSE)
       if (compiled_extensions.sse41 || ext.sse41) {
           return create_context<scan_mode::SSE>(params);
       }
#endif
#endif
#if defined(LIBHAT_ARM) || defined(LIBHAT_AARCH64)
        if (compiled_extensions.neon || ext.neon) {
            return create_context<scan_mode::Neon>(params);
        }
#endif
        // If none of the vectorized implementations are available/supported, then fallback to scanning per-byte
        return create_context<scan_mode::Single>(params);
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

    consteval auto count_matches() {
        constexpr std::array a{std::byte{1}, std::byte{2}, std::byte{3}, std::byte{4}, std::byte{1}};
        constexpr hat::fixed_signature<1> s{std::byte{1}};

        std::vector<const_scan_result> results{};
        hat::find_all_pattern(a.cbegin(), a.cend(), std::back_inserter(results), s);
        return results.size();
    }
    static_assert(count_matches() == 2);

    static_assert([] {
        constexpr std::array a{std::byte{1}, std::byte{2}, std::byte{3}, std::byte{4}, std::byte{1}};
        constexpr hat::fixed_signature<1> s{std::byte{1}};

        std::vector<const_scan_result> results{};
        hat::find_all_pattern(a.cbegin(), a.cend(), std::back_inserter(results), s);

        return results == hat::find_all_pattern(a.cbegin(), a.cend(), s);
    }());

    static_assert([] {
        constexpr std::array a{std::byte{1}, std::byte{2}, std::byte{3}, std::byte{4}, std::byte{1}};
        constexpr hat::fixed_signature<1> s{std::byte{1}};

        std::array<const_scan_result, 2> results{};
        const auto [scan_end, results_end] = hat::find_all_pattern(a.cbegin(), a.cend(), results.begin(), results.end(), s);
        return scan_end == a.cend() && results_end == std::next(results.begin(), 2);
    }());
}
