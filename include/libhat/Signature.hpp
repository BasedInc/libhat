#pragma once

#include <array>
#include <bit>
#include <optional>
#include <ranges>
#include <string_view>
#include <vector>

#include "CompileTime.hpp"
#include "FixedString.hpp"

namespace hat {

    /// Effectively std::optional<std::byte>, but with the added flexibility of being able to use std::bit_cast on
    /// instances of the class in constant expressions.
    struct signature_element {
        constexpr signature_element() noexcept {}
        constexpr signature_element(std::nullopt_t) noexcept {}
        constexpr signature_element(const std::byte valueIn) noexcept : val(valueIn), present(true) {}

        constexpr signature_element& operator=(std::nullopt_t) noexcept {
            return *this = signature_element{};
        }

        constexpr signature_element& operator=(const std::byte valueIn) noexcept {
            return *this = signature_element{valueIn};
        }

        constexpr void reset(std::nullopt_t) noexcept {
            *this = std::nullopt;
        }

        [[nodiscard]] constexpr bool has_value() const noexcept {
            return this->present;
        }

        [[nodiscard]] constexpr std::byte value() const noexcept {
            return this->val;
        }

        [[nodiscard]] constexpr operator bool() const noexcept {
            return this->has_value();
        }

        [[nodiscard]] constexpr std::byte operator*() const noexcept {
            return this->value();
        }
    private:
        std::byte val{};
        bool present = false;
    };

    using signature = std::vector<signature_element>;
    using signature_view = std::span<const signature_element>;

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

    template<typename Char>
    [[nodiscard]] constexpr signature string_to_signature(std::basic_string_view<Char> str) {
        const auto bytes = std::as_bytes(std::span{str});
        signature sig{};
        sig.reserve(bytes.size());
        for (std::byte byte : bytes) {
            sig.emplace_back(byte);
        }
        return sig;
    }

    template<typename Char>
    [[nodiscard]] constexpr signature string_to_signature(std::basic_string<Char> str) {
        return string_to_signature(std::basic_string_view<Char>{str});
    }

    enum class signature_parse_error {
        missing_byte,
        parse_error,
        empty_signature,
    };

    [[nodiscard]] constexpr result<signature, signature_parse_error> parse_signature(std::string_view str) {
        signature sig{};
        bool containsByte = false;
        for (const auto& word : str | std::views::split(' ')) {
            if (word.empty()) {
                continue;
            }
            if (word[0] == '?') {
                sig.emplace_back(std::nullopt);
            } else {
                const auto sv = std::string_view{word.begin(), word.end()};
                const auto parsed = parse_int<uint8_t>(sv, 16);
                if (parsed.has_value()) {
                    sig.emplace_back(static_cast<std::byte>(parsed.value()));
                } else {
                    return result_error{signature_parse_error::parse_error};
                }
                containsByte = true;
            }
        }
        if (sig.empty()) {
            return result_error{signature_parse_error::empty_signature};
        }
        if (!containsByte) {
            return result_error{signature_parse_error::missing_byte};
        }
        return sig;
    }

    /// Parses a signature string at compile time and returns the result as a fixed_signature
    template<fixed_string str>
    [[nodiscard]] consteval auto compile_signature() {
        const auto sig = parse_signature(str.c_str()).value();
        constexpr auto N = parse_signature(str.c_str()).value().size();
        fixed_signature<N> arr{};
        std::ranges::move(sig, arr.begin());
        return arr;
    }
}

namespace hat::literals::signature_literals {
    template<hat::fixed_string str>
    consteval auto operator""_sig() noexcept {
        return hat::compile_signature<str>();
    }
}
