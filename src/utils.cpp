#include <libhat/scanner.hpp>

#include <array>
#include <cstddef>
#include <tuple>
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

    static auto count_bytes(const signature_view signature) {
        std::array<std::uint8_t, 256> counts{};
        for (const auto element : signature) {
            const auto value = std::to_integer<std::uint8_t>(element.value());
            auto& count = counts[value];
            if (element.all() && count < 0xFF) {
                count++;
            }
        }
        return counts;
    }

    std::optional<std::size_t> get_optimal_pair(const scan_parameters& params) {
        const auto sig = params.signature;

        // Only utilize byte pair based scanning if the signature starts with a byte pair
        if (static_cast<bool>(params.hints & scan_hint::pair0)) {
            if (sig.size() >= 2 && sig[0].all() && sig[1].all()) {
                return 0;
            }
            return {};
        }

        if (const auto pair_hint = get_pair_hint(params.hints)) {
            const auto getScore = [&](const std::byte a, const std::byte b) -> std::uint16_t {
                const auto& [pairs, scores] = *pair_hint;
                const std::pair pair{a, b};
                const auto it = std::ranges::lower_bound(pairs, pair);
                const auto index = static_cast<std::uint16_t>(it - pairs.begin());
                return it != pairs.end() && *it == pair ? scores[index] : NUM_PAIRS;
            };

            std::optional<std::pair<std::size_t, std::uint16_t>> best{};
            for (auto it = sig.begin(); it != std::prev(sig.end()); it++) {
                auto a = *it;
                auto b = *std::next(it);
                if (!a.all() || !b.all()) {
                    continue;
                }

                const auto score = getScore(a.value(), b.value());
                if (!best || score > best->second) {
                    best.emplace(std::distance(sig.begin(), it), score);
                }
            }

            if (best) {
                return best->first;
            }
        }

        // If no "optimal" pair was found based on hints, find the best one based on individual byte occurrences
        const auto counts = count_bytes(sig);
        std::optional<std::pair<std::size_t, std::uint16_t>> best{};
        for (auto it = sig.begin(); it != std::prev(sig.end()); it++) {
            auto a = *it;
            auto b = *std::next(it);
            if (!a.all() || !b.all()) {
                continue;
            }

            const auto score = static_cast<std::uint16_t>(counts[std::to_integer<std::uint8_t>(*a)]
                + counts[std::to_integer<std::uint8_t>(*b)]);
            if (!best || score < best->second) {
                best.emplace(std::distance(sig.begin(), it), score);
                if (score == 2) {
                    break; // minimum value
                }
            }
        }

        if (best) {
            return best->first;
        }
        return {};
    }

    std::optional<std::size_t> get_optimal_byte(const scan_parameters& params) {
        const auto signature = params.signature;
        const auto counts = count_bytes(signature);

        auto bytes = signature | std::views::filter(&signature_element::all);
        const auto min = std::ranges::min_element(bytes, std::less{},
            [&](const auto element) { return counts[std::to_integer<std::uint8_t>(*element)]; }).base();

        if (min != signature.end()) {
            return std::distance(signature.begin(), min);
        }
        return {};
    }
}
