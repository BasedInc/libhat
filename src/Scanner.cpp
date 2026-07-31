#include <libhat/scanner.hpp>

#include <libhat/defines.hpp>
#include <libhat/system.hpp>

#ifdef LIBHAT_HINT_X86_64
#include "arch/x86/Frequency.hpp"
#endif

#ifdef LIBHAT_HINT_AARCH64
#include "arch/arm/Frequency.hpp"
#endif

#include "Utils.hpp"

namespace {

    struct anchor_matcher {

        anchor_matcher(const hat::signature_view& signature, const std::span<std::size_t>& anchors, const std::size_t literals) :
            signature_(signature),
            anchors_(anchors),
            literals_(literals) {}

        [[nodiscard]] std::pair<hat::signature_element, std::size_t> top() const {
            return anchor(0);
        }

        [[nodiscard]] bool matches(const std::byte* buffer) const {
            for (size_t i = 1; i < literals_; i++) {
                const auto [element, offset] = anchor(i);
                if (buffer[offset] != element) {
                    std::swap(anchors_[i], anchors_[i - 1]);
                    return false;
                }
            }

            for (size_t i = literals_; i < anchors_.size(); i++) {
                const auto [element, offset] = anchor(i);
                if (buffer[offset] != element) {
                    return false;
                }
            }

            return true;
        }

        [[nodiscard]] std::size_t literals() const {
            return literals_;
        }

    private:
        [[nodiscard]] std::pair<hat::signature_element, std::size_t> anchor(const size_t index) const {
            const auto offset = anchors_[index];
            return {signature_[offset], offset};
        }

        hat::signature_view    signature_;
        std::span<std::size_t> anchors_;
        std::size_t            literals_;
    };

    struct signature_info {
        std::optional<std::size_t> optimalByteIndex{}; // The index of the least common fully masked element
        std::size_t                anyMaskCount{};     // The number of elements with a non-zero mask
    };

    struct std_context {
        mutable std::vector<std::size_t> anchors{};
        std::size_t literals{};

        explicit std_context(const hat::detail::scan_parameters& params) {
            this->anchors.resize(static_cast<size_t>(
                std::ranges::count_if(params.signature, &hat::signature_element::any)));
            auto it = this->anchors.begin();

            // Add fully masked bytes
            for (size_t i{}; auto e : params.signature) {
                if (e.all()) {
                    *it++ = i;
                }
                i++;
            }
            this->literals = static_cast<std::size_t>(std::distance(this->anchors.begin(), it));

            // Add partially masked bytes
            for (size_t i{}; auto e : params.signature) {
                if (!e.all() && !e.none()) {
                    *it++ = i;
                }
                i++;
            }
        }
    };

    anchor_matcher create_anchor_matcher(const hat::detail::scan_context& context) {
        const auto& std = context.get<std_context>();
        return {context.signature(), std.anchors, std.literals};
    }
}

namespace hat::detail {

    static constexpr std::uint16_t NUM_PAIRS = 512;

    using pair_hint_t = std::tuple<
        const std::array<std::pair<std::byte, std::byte>, NUM_PAIRS>&,
        const std::array<std::uint16_t, NUM_PAIRS>&
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

    template<scan_alignment alignment>
    static const_scan_result find_pattern_single(const std::byte* begin, const std::byte* end, const scan_context& context) {
        static constexpr auto stride = alignment_stride<alignment>;

        const auto scanBegin = align_up<stride>(begin);
        const auto scanEnd = align_up<stride>(end - context.signature().size() + 1);
        if (scanBegin >= scanEnd) {
            return nullptr;
        }

        const auto matcher = create_anchor_matcher(context);
        for (auto i = scanBegin; i != scanEnd; i += stride) {
            if (matcher.literals() > 0) {
                const auto [anchor, offset] = matcher.top();
                while (i != scanEnd) {
                    if (i[offset] == anchor) {
                        break;
                    }
                    i += stride;
                }
                if (i == scanEnd) LIBHAT_UNLIKELY break;
            }
            if (matcher.matches(i)) {
                return i;
            }
        }

        return nullptr;
    }

    template<>
    const_scan_result find_pattern_single<scan_alignment::X1>(const std::byte* begin, const std::byte* end, const scan_context& context) {
        const auto signature = context.signature();
        const auto scanEnd = end - signature.size() + 1;

        const auto matcher = create_anchor_matcher(context);
        for (auto i = begin; i != scanEnd; i++) {
            // This check should get hoisted out by the optimizer
            if (matcher.literals() > 0) {
                const auto [anchor, offset] = matcher.top();

                // Use std::find to efficiently find the first byte
                #ifndef _MSC_VER
                    i = static_cast<const std::byte*>(
                        std::memchr(i + offset, static_cast<unsigned char>(anchor.value()), static_cast<std::size_t>(scanEnd - i)));
                    if (!i) LIBHAT_UNLIKELY break;
                    i -= offset;
                #elif __cpp_lib_execution >= 201902L
                    i = std::find(std::execution::unseq, i + offset, scanEnd + offset, anchor.value()) - offset;
                    if (i == scanEnd) LIBHAT_UNLIKELY break;
                #else
                    i = std::find(i + offset, scanEnd + offset, anchor.value()) - offset;
                    if (i == scanEnd) LIBHAT_UNLIKELY break;
                #endif
            }
            if (matcher.matches(i)) {
                return i;
            }
        }
        return nullptr;
    }

    template<>
    scan_context create_context<scan_mode::Single>(const scan_parameters& params) {
        switch (params.alignment) {
            case scan_alignment::X1: return {params.signature, &find_pattern_single<scan_alignment::X1>, std::type_identity<std_context>{}, params};
            case scan_alignment::X4: return {params.signature, &find_pattern_single<scan_alignment::X4>, std::type_identity<std_context>{}, params};
            case scan_alignment::X16: return {params.signature, &find_pattern_single<scan_alignment::X16>, std::type_identity<std_context>{}, params};
        }
        LIBHAT_UNREACHABLE();
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
