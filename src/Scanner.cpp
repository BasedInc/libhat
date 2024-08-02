#include <libhat/Scanner.hpp>

#include <libhat/Defines.hpp>
#include <libhat/System.hpp>

#include "arch/x86/Frequency.hpp"

namespace hat::detail {

    void scan_context::apply_hints(const scanner_context& scanner) {
        const bool x86_64 = static_cast<bool>(this->hints & scan_hint::x86_64);
        const bool pair0 = static_cast<bool>(this->hints & scan_hint::pair0);

        if (x86_64 && !pair0 && scanner.vectorSize && this->alignment == hat::scan_alignment::X1) {
            const auto get_score = [this](const std::byte a, const std::byte b) {
                constexpr auto& pairs = hat::detail::x86_64::pairs_x1;
                const auto it = std::ranges::find(pairs, std::pair{a, b});
                return it == pairs.end() ? pairs.size() : pairs.size() - static_cast<size_t>(it - pairs.begin()) - 1;
            };

            const auto score_pair = [&](auto&& tup) {
                auto [a, b] = std::get<1>(tup);
                return std::make_tuple(std::get<0>(tup), get_score(a.value(), b.value()));
            };

            static constexpr auto is_complete_pair = [](auto&& tup) {
                auto [a, b] = std::get<1>(tup);
                return a.has_value() && b.has_value();
            };

            auto valid_pairs = this->signature
                | std::views::take(scanner.vectorSize)
                | std::views::adjacent<2>
                | std::views::enumerate
                | std::views::filter(is_complete_pair)
                | std::views::transform(score_pair);

            if (!valid_pairs.empty()) {
                this->pairIndex = std::get<0>(std::ranges::max(valid_pairs, std::ranges::less{}, [](auto&& tup) {
                    return std::get<1>(tup);
                }));
            }
        }

        // If no "optimal" pair was found, find the first byte pair in the signature
        if (!this->pairIndex.has_value()) {
            size_t i{};
            for (auto&& [a, b] : this->signature | std::views::adjacent<2>) {
                if (a.has_value() && b.has_value()) {
                    this->pairIndex = i;
                    break;
                }
                if (i == 0 && pair0) {
                    break;
                }
                i++;
            }
        }
    }

    void scan_context::auto_resolve_scanner() {
#if defined(LIBHAT_X86)
        const auto& ext = get_system().extensions;
        if (ext.bmi) {
#if !defined(LIBHAT_DISABLE_AVX512)
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
}
