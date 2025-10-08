#pragma once

#ifndef LIBHAT_MODULE
    #include <algorithm>
    #include <array>
    #include <bit>
    #include <optional>
    #include <ranges>
    #include <string_view>
    #include <vector>
#endif

#include "defines.hpp"
#include "export.hpp"
#include "strconv.hpp"
#include "fixed_string.hpp"

LIBHAT_EXPORT namespace hat {

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

        constexpr void reset() noexcept {
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
    template<size_t N>
    [[nodiscard]] constexpr auto bytes_to_signature(std::span<const std::byte, N> bytes) {
        if constexpr (N == std::dynamic_extent) {
            return signature{bytes.begin(), bytes.end()};
        } else {
            fixed_signature<N> result;
            std::ranges::copy(bytes, result.begin());
            return result;
        }
    }

    template<typename T>
    [[nodiscard]] constexpr auto object_to_signature(const T& value) {
        constexpr size_t N = sizeof(T);
        using view = std::span<const std::byte, N>;
        if LIBHAT_IF_CONSTEVAL {
            return bytes_to_signature(view{std::bit_cast<std::array<std::byte, N>>(value)});
        } else {
            return bytes_to_signature(view{reinterpret_cast<const std::byte*>(&value), N});
        }
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

    [[nodiscard]] LIBHAT_CONSTEXPR_RESULT result<size_t, signature_parse_error> parse_signature_to(std::output_iterator<signature_element> auto out, std::string_view str) {
        size_t written = 0;
        bool containsByte = false;
        for (const auto& word : str | std::views::split(' ')) {
            if (word.empty()) {
                continue;
            }
            if (word[0] == '?') {
                *out++ = signature_element{std::nullopt};
                written++;
            } else {
                const auto sv = std::string_view{word.begin(), word.end()};
                const auto parsed = parse_int<uint8_t>(sv, 16);
                if (parsed.has_value()) {
                    *out++ = signature_element{static_cast<std::byte>(parsed.value())};
                    written++;
                } else {
                    return result_error{signature_parse_error::parse_error};
                }
                containsByte = true;
            }
        }
        if (written == 0) {
            return result_error{signature_parse_error::empty_signature};
        }
        if (!containsByte) {
            return result_error{signature_parse_error::missing_byte};
        }
        return written;
    }

    [[nodiscard]] LIBHAT_CONSTEXPR_RESULT result<signature, signature_parse_error> parse_signature(std::string_view str) {
        signature sig{};
        auto result = parse_signature_to(std::back_inserter(sig), str);

        if (result.has_value()) {
            return sig;
        }

        return result_error{result.error()};
    }
}

namespace hat::detail {

    template<size_t N>
    [[nodiscard]] consteval std::pair<std::array<signature_element, N>, size_t> compile_signature_max_size(std::string_view str) {
        std::array<signature_element, N> array;
        auto size = parse_signature_to(array.begin(), str);
        return {array, size.value()};
    }
}

LIBHAT_EXPORT namespace hat {

    /// Parses a signature string at compile time and returns the result as a fixed_signature
    template<fixed_string str>
    [[nodiscard]] consteval auto compile_signature() {
#if __cpp_constexpr >= 202211L
        static
#endif
        constexpr auto pair = detail::compile_signature_max_size<str.size() / 2 + 1>(str.to_view());
        fixed_signature<pair.second> arr{};
        std::move(pair.first.begin(), pair.first.begin() + pair.second, arr.begin());
        return arr;
    }

    [[nodiscard]] constexpr std::string to_string(const signature_view signature) {
        constexpr std::string_view hex{"0123456789ABCDEF"};
        std::string ret;
        ret.reserve(signature.size() * 3);
        for (auto& element : signature) {
            if (element.has_value()) {
                ret += {
                    hex[static_cast<size_t>(element.value() >> 4) & 0xFu],
                    hex[static_cast<size_t>(element.value() >> 0) & 0xFu],
                    ' '
                };
            } else {
                ret += "? ";
            }
        }
        ret.pop_back();
        return ret;
    }
}

LIBHAT_EXPORT namespace hat::inline literals::inline signature_literals {

    template<hat::fixed_string str>
    consteval auto operator""_sig() noexcept {
        return hat::compile_signature<str>();
    }

#if __cpp_constexpr >= 202211L
    template<hat::fixed_string str>
    constexpr auto operator""_sigv() noexcept {
        static constexpr auto sig = hat::compile_signature<str>();
        return hat::signature_view{sig};
    }
#endif
}
