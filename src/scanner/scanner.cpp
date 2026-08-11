#include <libhat/scanner.hpp>

#include <libhat/defines.hpp>
#include <libhat/system.hpp>

#define TEST_EXT(f)                          \
    [](const auto& ext) -> bool {            \
        if constexpr (requires { ext.f; }) { \
            return ext.f;                    \
        }                                    \
        return false;                        \
    }

#define TRY_SCAN_MODE(mode)                                     \
    do {                                                        \
        if constexpr (::has_impl<scan_mode::mode>::value)     \
            if (is_supported(scan_mode::mode))                  \
                return create_context<scan_mode::mode>(params); \
    } while (false)

namespace {

    template<hat::detail::scan_mode>
    struct has_impl : std::false_type {};

    template<>
    struct has_impl<hat::detail::scan_mode::Auto> : std::true_type {};

    template<>
    struct has_impl<hat::detail::scan_mode::Search> : std::true_type {};

    template<>
    struct has_impl<hat::detail::scan_mode::Single> : std::true_type {};

#if (defined(LIBHAT_X86) || defined(LIBHAT_X86_64)) && defined(LIBHAT_FEATURE_SSE)
    template<>
    struct has_impl<hat::detail::scan_mode::SSE> : std::true_type {};
#endif

#if defined(LIBHAT_X86) || defined(LIBHAT_X86_64)
    template<>
    struct has_impl<hat::detail::scan_mode::AVX2> : std::true_type {};
#endif

#if defined(LIBHAT_X86_64) && defined(LIBHAT_FEATURE_AVX512)
    template<>
    struct has_impl<hat::detail::scan_mode::AVX512> : std::true_type {};
#endif

#if defined(LIBHAT_ARM) || defined(LIBHAT_AARCH64)
    template<>
    struct has_impl<hat::detail::scan_mode::Neon> : std::true_type {};
#endif

    template<hat::detail::scan_mode Mode>
    bool all_of(std::predicate<const hat::cpu_extensions&> auto&&... tests) {
        if constexpr (has_impl<Mode>::value) {
            const auto& ext = hat::get_system().extensions;
            return ((tests(hat::compiled_extensions) || tests(ext)) && ...);
        }
        return false;
    }
}

namespace hat::detail {

    bool is_supported(const scan_mode mode) {
        switch (mode) {
            case scan_mode::Auto:
            case scan_mode::Search:
            case scan_mode::Single:
                return true;
            case scan_mode::SSE:
                return all_of<scan_mode::SSE>(TEST_EXT(sse41));
            case scan_mode::AVX2:
                return all_of<scan_mode::AVX2>(TEST_EXT(bmi), TEST_EXT(avx2));
            case scan_mode::AVX512:
                return all_of<scan_mode::AVX512>(TEST_EXT(bmi), TEST_EXT(avx512f), TEST_EXT(avx512bw));
            case scan_mode::Neon:
                return all_of<scan_mode::Neon>(TEST_EXT(neon));
        }
        LIBHAT_UNREACHABLE();
    }

    template<>
    scan_context create_context<scan_mode::Auto>(const scan_parameters& params) {
        TRY_SCAN_MODE(AVX512);
        TRY_SCAN_MODE(AVX2);
        TRY_SCAN_MODE(SSE);
        TRY_SCAN_MODE(Neon);

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
