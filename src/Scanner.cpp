#include <libhat/scanner.hpp>

#include <libhat/defines.hpp>
#include <libhat/system.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <utility>

#include "utils.hpp"

namespace hat::detail {

    static constexpr std::uint16_t NUM_PAIRS = 512;

    using pair_hint_t = std::tuple<
        const std::array<std::pair<std::byte, std::byte>, NUM_PAIRS>&,
        const std::array<std::uint16_t, NUM_PAIRS>&
    >;

    static auto get_pair_hint(const scan_hint hints) -> std::optional<pair_hint_t> {
        [[maybe_unused]] constexpr auto p = [](const std::uint8_t a, const std::uint8_t b) {
            return std::pair{std::byte{a}, std::byte{b}};
        };
        if (static_cast<bool>(hints & scan_hint::x86_64)) {
#ifdef LIBHAT_HINT_X86_64
#include "scanner/frequency/x86_64.inl"
            return std::tie(pairs_x1, scores_x1);
#endif
        }
        if (static_cast<bool>(hints & scan_hint::aarch64)) {
#ifdef LIBHAT_HINT_AARCH64
#include "scanner/frequency/aarch64.inl"
            return std::tie(pairs_x1, scores_x1);
#endif
        }
        return std::nullopt;
    }

    std::optional<std::size_t> get_optimal_pair(const scan_parameters& params) {
        const bool pair0 = static_cast<bool>(params.hints & scan_hint::pair0);

        const auto pair_hint = get_pair_hint(params.hints);
        if (pair_hint && !pair0) {
            const auto getScore = [&](const std::byte a, const std::byte b) -> std::uint16_t {
                const auto& [pairs, scores] = *pair_hint;
                const std::pair pair{a, b};
                const auto it = std::ranges::lower_bound(pairs, pair);
                const auto index = static_cast<std::uint16_t>(it - pairs.begin());
                return it != pairs.end() && *it == pair ? scores[index] : NUM_PAIRS;
            };

            std::optional<std::pair<std::size_t, std::uint16_t>> best{};
            for (auto it = params.signature.begin(); it != std::prev(params.signature.end()); it++) {
                const auto i = static_cast<std::size_t>(it - params.signature.begin());
                auto& a = *it;
                auto& b = *std::next(it);

                if (a.all() && b.all()) {
                    const auto score = getScore(a.value(), b.value());
                    if (!best || score > best->second) {
                        best.emplace(i, score);
                    }
                }
            }

            if (best) {
                return best->first;
            }
        }

        // If no "optimal" pair was found, find the first byte pair in the signature
        for (auto it = params.signature.begin(); it != std::prev(params.signature.end()); it++) {
            const auto i = static_cast<std::size_t>(it - params.signature.begin());
            auto& a = *it;
            auto& b = *std::next(it);

            if (a.all() && b.all()) {
                return i;
            }
            if (i == 0 && pair0) {
                break;
            }
        }

        return {};
    }

    std::optional<std::size_t> get_optimal_byte(const scan_parameters& params) {
        const auto signature = params.signature;

        std::array<uint8_t, 256> counts{};
        for (const auto element : signature) {
            const auto value = std::to_integer<uint8_t>(element.value());
            auto& count = counts[value];
            if (element.all() && count < 0xFF) {
                count++;
            }
        }

        std::optional<size_t> min{};
        uint8_t minCount = 0xFF;
        for (std::size_t i = 0; i < signature.size(); i++) {
            auto& element = signature[i];
            const auto count = std::to_integer<uint8_t>(element.value());
            if (!min || count < minCount) {
                min = i;
                minCount = count;
            }
        }

        return min;
    }

    template<>
    scan_context create_context<scan_mode::Auto>(const scan_parameters& params) {
//         const auto& ext = get_system().extensions;
// #if defined(LIBHAT_X86) || defined(LIBHAT_X86_64)
//         if (compiled_extensions.bmi || ext.bmi) {
// #if defined(LIBHAT_X86_64) && defined(LIBHAT_FEATURE_AVX512)
//             if ((compiled_extensions.avx512f || ext.avx512f)
//                 && (compiled_extensions.avx512bw || ext.avx512bw)) {
//                 return create_context<scan_mode::AVX512>(params);
//             }
// #endif
//             if (compiled_extensions.avx2 || ext.avx2) {
//                 return create_context<scan_mode::AVX2>(params);
//             }
//         }
// #if defined(LIBHAT_FEATURE_SSE)
//         if (compiled_extensions.sse41 || ext.sse41) {
//             return create_context<scan_mode::SSE>(params);
//         }
// #endif
// #endif
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
