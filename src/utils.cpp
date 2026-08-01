#include <libhat/scanner.hpp>

#include <array>
#include <cstddef>
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
}
