#include <libhat/scanner.hpp>

#include <libhat/defines.hpp>
#include <libhat/system.hpp>

#ifdef LIBHAT_HINT_X86_64
#include "arch/x86/Frequency.hpp"
#endif

#ifdef LIBHAT_HINT_AARCH64
#include "arch/arm/Frequency.hpp"
#endif

namespace hat::detail {

    static constexpr uint16_t NUM_PAIRS = 512;

    using pair_hint_t = std::tuple<
        const std::array<std::pair<std::byte, std::byte>, NUM_PAIRS>&,
        const std::array<uint16_t, NUM_PAIRS>&
    >;

    static constexpr auto get_pair_hint(const scan_hint hints) -> std::optional<pair_hint_t> {
#ifdef LIBHAT_HINT_X86_64
        if (static_cast<bool>(hints & scan_hint::x86_64)) {
            return std::tie(hat::detail::x86_64::pairs_x1, hat::detail::x86_64::scores_x1);
        }
#endif
#ifdef LIBHAT_HINT_AARCH64
        if (static_cast<bool>(hints & scan_hint::aarch64)) {
            return std::tie(hat::detail::aarch64::pairs_x1, hat::detail::aarch64::scores_x1);
        }
#endif
        return std::nullopt;
    }

    void scan_context::apply_hints(const scanner_context& scanner) {
        const bool pair0 = static_cast<bool>(this->hints & scan_hint::pair0);

        const auto pair_hint = get_pair_hint(this->hints);
        if (pair_hint && !pair0 && scanner.vectorSize) {
            const auto getScore = [&](const std::byte a, const std::byte b) -> uint16_t {
                const auto& [pairs, scores] = *pair_hint;
                const std::pair pair{a, b};
                const auto it = std::ranges::lower_bound(pairs, pair);
                const auto index = static_cast<uint16_t>(it - pairs.begin());
                return it != pairs.end() && *it == pair ? scores[index] : NUM_PAIRS;
            };

            std::optional<std::pair<size_t, uint16_t>> bestPair{};
            for (auto it = this->signature.begin(); it != std::prev(this->signature.end()); it++) {
                const auto i = static_cast<size_t>(it - this->signature.begin());
                auto& a = *it;
                auto& b = *std::next(it);

                if (a.all() && b.all()) {
                    const auto score = getScore(a.value(), b.value());
                    if (!bestPair || score > bestPair->second) {
                        bestPair.emplace(i, score);
                    }
                }
            }

            if (bestPair) {
                this->pairIndex = bestPair->first;
            }
        }

        // If no "optimal" pair was found, find the first byte pair in the signature
        if (!this->pairIndex.has_value()) {
            for (auto it = this->signature.begin(); it != std::prev(this->signature.end()); it++) {
                const auto i = static_cast<size_t>(it - this->signature.begin());
                auto& a = *it;
                auto& b = *std::next(it);

                if (a.all() && b.all()) {
                    this->pairIndex = i;
                    break;
                }
                if (i == 0 && pair0) {
                    break;
                }
            }
        }
    }

    template<>
    scan_function_t resolve_scanner<scan_mode::Auto>(scan_context& context) {
        const auto& ext = get_system().extensions;
#if defined(LIBHAT_X86) || defined(LIBHAT_X86_64)
        if (compiled_extensions.bmi || ext.bmi) {
#if defined(LIBHAT_X86_64) && !defined(LIBHAT_DISABLE_AVX512)
            if ((compiled_extensions.avx512f || ext.avx512f)
                && (compiled_extensions.avx512bw || ext.avx512bw)) {
                return resolve_scanner<scan_mode::AVX512>(context);
            }
#endif
            if (compiled_extensions.avx2 || ext.avx2) {
                return resolve_scanner<scan_mode::AVX2>(context);
            }
        }
#if !defined(LIBHAT_DISABLE_SSE)
        if (compiled_extensions.sse41 || ext.sse41) {
            return resolve_scanner<scan_mode::SSE>(context);
        }
#endif
#endif
#if defined(LIBHAT_ARM) || defined(LIBHAT_AARCH64)
        if (compiled_extensions.neon || ext.neon) {
            return resolve_scanner<scan_mode::Neon>(context);
        }
#endif
        // If none of the vectorized implementations are available/supported, then fallback to scanning per-byte
        return resolve_scanner<scan_mode::Single>(context);
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
