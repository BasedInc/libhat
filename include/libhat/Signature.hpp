#pragma once

#include <array>
#include <bit>
#include <optional>
#include <ranges>
#include <string_view>
#include <vector>

#include "CompileTime.hpp"

namespace hat {

    using signature_element = std::optional<std::byte>;
    using signature         = std::vector<signature_element>;
    using signature_view    = std::span<const signature_element>;

    template<size_t N>
    using fixed_signature = std::array<signature_element, N>;

    /// Convert raw byte storage into a signature
    [[nodiscard]] constexpr signature bytes_to_signature(std::span<const std::byte> bytes) {
        return {bytes.begin(), bytes.end()};
    }

    template<typename T>
    [[nodiscard]] constexpr signature object_to_signature(const T& value) {
        using bytes = std::array<std::byte, sizeof(T)>;
        return bytes_to_signature(std::bit_cast<bytes>(value));
    }

    [[nodiscard]] constexpr signature string_to_signature(std::string_view str) {
        signature sig{};
        sig.reserve(str.size());
        for (char ch : str) {
            sig.emplace_back(static_cast<std::byte>(ch));
        }
        return sig;
    }

    enum class signature_parse_error {
        illegal_wildcard,
        parse_error,
        empty_signature,
    };

    [[nodiscard]] constexpr result<signature, signature_parse_error> parse_signature(std::string_view str) {
        signature sig{};
        for (const auto& word : str | std::views::split(' ')) {
            if (word.empty()) {
                continue;
            } else if (word[0] == '?') {
                if (sig.empty()) {
                    return result_error{signature_parse_error::illegal_wildcard};
                }
                sig.emplace_back(std::nullopt);
            } else {
                const auto sv = std::string_view{word.begin(), word.end()};
                auto parsed = parse_int<uint8_t>(sv, 16);
                if (parsed.has_value()) {
                    sig.emplace_back(static_cast<std::byte>(parsed.value()));
                } else {
                    return result_error{signature_parse_error::parse_error};
                }
            }
        }
        if (sig.empty()) {
            return result_error{signature_parse_error::empty_signature};
        }
        return sig;
    }

    /// Parses a signature string at compile time and returns the result as a fixed_signature
    template<string_literal str>
    [[nodiscard]] consteval auto compile_signature() {
        const auto sig = parse_signature(str.c_str()).value();
        constexpr auto N = parse_signature(str.c_str()).value().size();
        fixed_signature<N> arr{};
        std::ranges::move(sig, arr.begin());
        return arr;
    }
}

#ifdef LIBHAT_ENABLE_SIGNATURE_LITERALS
namespace hat::literals::signature_literals {
    template<typename CharT, CharT... Chars>
    consteval auto operator""_sig() noexcept {
        return hat::compile_signature<(CharT[sizeof...(Chars)]){Chars...}>();
    }
}
#endif
