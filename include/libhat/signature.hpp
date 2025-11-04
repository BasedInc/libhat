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
        constexpr signature_element() noexcept = default;
        constexpr signature_element(std::nullopt_t) noexcept {}
        constexpr signature_element(const std::byte value) noexcept : value_{value}, mask_{0xFF} {}
        constexpr signature_element(const std::byte value, const std::byte mask) noexcept : value_{value & mask}, mask_{mask} {}

        constexpr signature_element& operator=(std::nullopt_t) noexcept {
            return *this = signature_element{};
        }

        constexpr signature_element& operator=(const std::byte valueIn) noexcept {
            return *this = signature_element{valueIn};
        }

        constexpr void reset() noexcept {
            *this = std::nullopt;
        }

        [[nodiscard]] constexpr std::byte value() const noexcept {
            return this->value_;
        }

        [[nodiscard]] constexpr std::byte mask() const noexcept {
            return this->mask_;
        }

        [[nodiscard]] constexpr std::byte operator*() const noexcept {
            return this->value();
        }

        [[nodiscard]] constexpr bool all() const noexcept {
            return this->mask_ == std::byte{0xFF};
        }

        [[nodiscard]] constexpr bool any() const noexcept {
            return this->mask_ != std::byte{0x00};
        }

        [[nodiscard]] constexpr bool none() const noexcept {
            return this->mask_ == std::byte{0x00};
        }

        [[nodiscard]] constexpr bool has(const uint8_t digit) const noexcept {
            const auto m = std::to_integer<uint8_t>(this->mask_);
            return (m & (1u << digit)) != 0;
        }

        [[nodiscard]] constexpr bool at(const uint8_t digit) const noexcept {
            const auto v = std::to_integer<uint8_t>(this->value_);
            return (v & (1u << digit)) != 0;
        }

        [[nodiscard]] constexpr std::strong_ordering operator<=>(const signature_element& other) const noexcept = default;

        [[nodiscard]] constexpr bool operator==(const std::byte byte) const noexcept {
            return (byte & this->mask_) == this->value_;
        }

    private:
        std::byte value_{};
        std::byte mask_{};
    };

    using signature = std::vector<signature_element>;
    using signature_view = std::span<const signature_element>;

    template<size_t N>
    using fixed_signature = std::array<signature_element, N>;

    enum class signature_error {
        missing_masked_byte,
        element_parse_error,
        empty_signature,
        expected_wildcard,
        invalid_token_length,
        illegal_first_byte,
    };

    /// Convert raw byte storage into a signature
    template<size_t N> requires (N != std::dynamic_extent && N > 0)
    [[nodiscard]] constexpr auto bytes_to_signature(std::span<const std::byte, N> bytes) {
        fixed_signature<N> result;
        std::ranges::copy(bytes, result.begin());
        return result;
    }

    /// Convert raw byte storage into a signature
    [[nodiscard]] LIBHAT_CONSTEXPR_RESULT result<signature, signature_error> bytes_to_signature(std::span<const std::byte> bytes) {
        if (bytes.empty()) {
            return result_error{signature_error::empty_signature};
        }
        return signature{bytes.begin(), bytes.end()};
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
    [[nodiscard]] LIBHAT_CONSTEXPR_RESULT result<signature, signature_error> string_to_signature(std::basic_string_view<Char> str) {
        if (str.empty()) {
            return result_error{signature_error::empty_signature};
        }

        signature result;
        result.resize(str.size() * sizeof(Char));

        auto it = result.begin();
        for (Char ch : str) {
            const auto bytes = std::bit_cast<std::array<std::byte, sizeof(Char)>>(ch);
            std::ranges::copy(bytes, it);
            it += bytes.size();
        }

        return result;
    }

    template<typename Char>
    [[nodiscard]] LIBHAT_CONSTEXPR_RESULT result<signature, signature_error> string_to_signature(std::basic_string<Char> str) {
        return string_to_signature(std::basic_string_view<Char>{str});
    }

    namespace detail {

        LIBHAT_CONSTEXPR_RESULT std::optional<signature_element> parse_signature_element(const std::string_view str, const uint8_t base) {
            uint8_t value{};
            uint8_t mask{};
            for (auto& ch : str) {
                value *= base;
                mask *= base;
                if (ch != '?') {
                    auto digit = hat::parse_int<uint8_t>(&ch, &ch + 1, base);
                    if (!digit.has_value()) [[unlikely]] {
                        return std::nullopt;
                    }
                    value += digit.value();
                    mask += static_cast<uint8_t>(base - 1);
                }
            }

            return signature_element{std::byte{value}, std::byte{mask}};
        }
    }

    [[nodiscard]] LIBHAT_CONSTEXPR_RESULT result<size_t, signature_error> parse_signature_to(std::output_iterator<signature_element> auto out, const std::string_view str) {
        size_t written = 0;
        bool containsByte = false;

        for (auto&& sub : str | std::views::split(' ')) {
            const std::string_view word{sub.begin(), sub.end()};
            switch (word.size()) {
                case 0: {
                    continue;
                }
                case 1: {
                    if (word.front() != '?') {
                        return result_error{signature_error::expected_wildcard};
                    }
                    *out++ = signature_element{std::nullopt};
                    written++;
                    break;
                }
                case 2:
                case 8: {
                    const uint8_t base = word.size() == 2 ? 16 : 2;
                    auto element = detail::parse_signature_element(word, base);
                    if (element) {
                        *out++ = *element;
                        written++;

                        if (!containsByte && element->any()) {
                            if (!element->all()) {
                                return result_error{signature_error::illegal_first_byte};
                            }
                            containsByte = true;
                        }
                    } else {
                        return result_error{signature_error::element_parse_error};
                    }
                    break;
                }
                default: {
                    return result_error{signature_error::invalid_token_length};
                }
            }
        }
        if (written == 0) {
            return result_error{signature_error::empty_signature};
        }
        if (!containsByte) {
            return result_error{signature_error::missing_masked_byte};
        }
        return written;
    }

    [[nodiscard]] LIBHAT_CONSTEXPR_RESULT result<signature, signature_error> parse_signature(std::string_view str) {
        signature sig{};
        auto result = parse_signature_to(std::back_inserter(sig), str);

        if (result.has_value()) {
            return sig;
        }

        return result_error{result.error()};
    }
}

namespace hat::detail {

#ifdef LIBHAT_HAS_CONSTEXPR_RESULT
    template<size_t N>
    [[nodiscard]] consteval std::pair<std::array<signature_element, N>, size_t> compile_signature_max_size(std::string_view str) {
        std::array<signature_element, N> array;
        auto size = parse_signature_to(array.begin(), str);
        return {array, size.value()};
    }
#endif
}

LIBHAT_EXPORT namespace hat {

#ifdef LIBHAT_HAS_CONSTEXPR_RESULT
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
#endif

    [[nodiscard]] constexpr std::string to_string(const signature_view signature) {
        constexpr std::string_view hex{"0123456789ABCDEF"};
        std::string ret;
        ret.reserve(signature.size() * 3);
        for (auto& element : signature) {
            const bool a = (element.mask() & std::byte{0xF0}) == std::byte{0xF0};
            const bool b = (element.mask() & std::byte{0x0F}) == std::byte{0x0F};
            if (a || b) {
                ret += {
                    a ? hex[static_cast<size_t>(element.value() >> 4) & 0xFu] : '?',
                    b ? hex[static_cast<size_t>(element.value() >> 0) & 0xFu] : '?',
                    ' '
                };
            } else if (element.none()) {
                ret += "? ";
            } else {
                ret += {
                    element.has(7) ? (element.at(7) ? '1' : '0') : '?',
                    element.has(6) ? (element.at(6) ? '1' : '0') : '?',
                    element.has(5) ? (element.at(5) ? '1' : '0') : '?',
                    element.has(4) ? (element.at(4) ? '1' : '0') : '?',
                    element.has(3) ? (element.at(3) ? '1' : '0') : '?',
                    element.has(2) ? (element.at(2) ? '1' : '0') : '?',
                    element.has(1) ? (element.at(1) ? '1' : '0') : '?',
                    element.has(0) ? (element.at(0) ? '1' : '0') : '?',
                    ' '
                };
            }
        }
        ret.pop_back();
        return ret;
    }
}

LIBHAT_EXPORT namespace hat::inline literals::inline signature_literals {

#ifdef LIBHAT_HAS_CONSTEXPR_RESULT
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

#endif
}
