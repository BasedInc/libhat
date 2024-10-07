#include <libhat/Scanner.hpp>

#include <libhat/Defines.hpp>
#include <libhat/System.hpp>

#ifdef LIBHAT_HINT_X86_64
#include "arch/x86/Frequency.hpp"
#endif

namespace hat::detail {

    void scan_context::apply_hints(const scanner_context& scanner) {
        const bool pair0 = static_cast<bool>(this->hints & scan_hint::pair0);

#ifdef LIBHAT_HINT_X86_64
        const bool x86_64 = static_cast<bool>(this->hints & scan_hint::x86_64);
        if (x86_64 && !pair0 && scanner.vectorSize && this->alignment == hat::scan_alignment::X1) {
            static constexpr auto getScore = [](const std::byte a, const std::byte b) {
                constexpr auto& pairs = hat::detail::x86_64::pairs_x1;
                const auto it = std::ranges::find(pairs, std::pair{a, b});
                return static_cast<size_t>(it - pairs.begin());
            };

            std::optional<std::pair<size_t, size_t>> bestPair{};
            for (auto it = this->signature.begin(); it != std::prev(this->signature.end()); it++) {
                const auto i = static_cast<size_t>(it - this->signature.begin());
                auto& a = *it;
                auto& b = *std::next(it);

                if (a.has_value() && b.has_value()) {
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
#endif

        // If no "optimal" pair was found, find the first byte pair in the signature
        if (!this->pairIndex.has_value() && this->alignment == hat::scan_alignment::X1) {
            for (auto it = this->signature.begin(); it != std::prev(this->signature.end()); it++) {
                const auto i = static_cast<size_t>(it - this->signature.begin());
                auto& a = *it;
                auto& b = *std::next(it);

                if (a.has_value() && b.has_value()) {
                    this->pairIndex = i;
                    break;
                }
                if (i == 0 && pair0) {
                    break;
                }
            }
        }
    }

    void scan_context::auto_resolve_scanner() {
#if defined(LIBHAT_X86) || defined(LIBHAT_X86_64)
        const auto& ext = get_system().extensions;
        if (ext.bmi) {
#if defined(LIBHAT_X86_64) && !defined(LIBHAT_DISABLE_AVX512)
            if (ext.avx512f && ext.avx512bw) {
                this->scanner = resolve_scanner<scan_mode::AVX512>(*this);
                return;
            }
#endif
            if (ext.avx2) {
                this->scanner = resolve_scanner<scan_mode::AVX2>(*this);
                return;
            }
        }
#if !defined(LIBHAT_DISABLE_SSE)
        if (ext.sse41) {
            this->scanner = resolve_scanner<scan_mode::SSE>(*this);
            return;
        }
#endif
#endif
        // If none of the vectorized implementations are available/supported, then fallback to scanning per-byte
        this->scanner = resolve_scanner<scan_mode::Single>(*this);
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
