#pragma once

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

    template<typename T>
    constexpr signature object_to_signature(const T& value) {
        const auto bytes = std::bit_cast<std::array<std::byte, sizeof(T)>>(value);
        return {bytes.begin(), bytes.end()};
    }

    /// Convert raw byte storage into a signature
    constexpr signature bytes_to_signature(std::span<const std::byte> bytes) {
        return {bytes.begin(), bytes.end()};
    }

    inline signature string_to_signature(std::string_view str) {
        return bytes_to_signature({reinterpret_cast<const std::byte*>(str.data()), str.size()});
    }

    constexpr signature parse_signature(std::string_view str) {
        signature sig{};
        for (const auto& word : str | std::views::split(' ')) {
            if (word.empty()) {
                continue;
            } else if (word[0] == '?') {
                if (sig.empty()) {
                    throw std::invalid_argument("First byte cannot be a wildcard");
                }
                sig.emplace_back(std::nullopt);
            } else {
                const auto sv = std::string_view{word.begin(), word.end()};
                sig.emplace_back(static_cast<std::byte>(atoi(sv, 16) & 0xFF));
            }
        }
        return sig;
    }

    /// Parses a signature string at compile time, and provides a signature_view which exists for the program's lifetime
    template<string_literal str>
    inline signature_view compile_signature() {
        static constexpr auto compiled = ([]() consteval -> auto {
            const auto sig = parse_signature(str.c_str());
            constexpr auto N = parse_signature(str.c_str()).size();
            fixed_signature<N> arr{};
            std::ranges::move(sig, arr.begin());
            return arr;
        })();
        return compiled;
    }
}
