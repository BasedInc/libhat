#pragma once
#include <span>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include "cstring_view.hpp"

namespace hat {
    struct in_place_viewed_t{};
    struct in_place_owned_t{};

    /// Construct a cow's viewed type in place.
    static constexpr in_place_viewed_t in_place_viewed;
    /// Construct a cow's owned type in place.
    static constexpr in_place_owned_t in_place_owned;

    template <typename T>
    struct cow_traits;

    /**
     * A copy-on-write wrapper for dynamically-allocated containers. It's also useful for representing potentially owned data.
     *
     * @tparam T A type representing a non-owning view to the data.
     * @tparam Traits A traits type describing how to convert T to an owned container, among other things.
     */
    template <typename T, typename Traits = cow_traits<T>>
    requires std::is_trivially_copyable_v<T>
    struct cow {
        using traits_t = Traits;
        using viewed_t = T;
        using owned_t = typename traits_t::owned_t;

    private:
        using impl_t = std::variant<viewed_t, owned_t>;

    public:
        constexpr cow() noexcept(std::is_nothrow_default_constructible_v<impl_t>) requires(std::is_default_constructible_v<impl_t>) = default;

        constexpr explicit(false) cow(viewed_t view) noexcept : impl{ view } {}
        constexpr explicit cow(owned_t owned) noexcept : impl{ std::move(owned) } {}

        template <typename... Args>
        constexpr explicit cow(in_place_viewed_t, Args&&... args) noexcept(std::is_nothrow_constructible_v<viewed_t, Args&&...>)
            : impl{ std::in_place_type<viewed_t>, std::forward<Args>(args)... } {}

        template <typename... Args>
        constexpr explicit cow(in_place_owned_t, Args&&... args) noexcept(std::is_nothrow_constructible_v<owned_t, Args&&...>)
            : impl{ std::in_place_type<owned_t>, std::forward<Args>(args)... } {}

        template <std::convertible_to<viewed_t> U>
        requires traits_t::template allow_viewed_conversion<U>
        constexpr explicit(false) cow(U&& val) : cow(viewed_t{std::forward<U>(val)}) {}

        [[nodiscard]] constexpr bool is_viewed() const noexcept {
            return std::holds_alternative<viewed_t>(this->impl);
        }

        [[nodiscard]] constexpr bool is_owned() const noexcept {
            return std::holds_alternative<owned_t>(this->impl);
        }

        [[nodiscard]] constexpr viewed_t viewed() const noexcept(noexcept(traits_t::into_viewed(std::declval<const owned_t&>()))) {
            if (this->is_viewed()) {
                return std::get<viewed_t>(this->impl);
            }

            return traits_t::into_viewed(std::get<owned_t>(this->impl));
        }

        [[nodiscard]] constexpr owned_t& owned() & noexcept(noexcept(traits_t::into_owned(std::declval<viewed_t&>()))) {
            if (this->is_owned()) {
                return std::get<owned_t>(this->impl);
            }

            return this->impl.template emplace<owned_t>(traits_t::into_owned(std::get<viewed_t>(this->impl)));
        }

        [[nodiscard]] constexpr owned_t&& owned() && noexcept(noexcept(traits_t::into_owned(std::declval<viewed_t&>()))) {
            return std::move(this->owned());
        }

        constexpr void ensure_owned() {
            std::ignore = this->owned();
        }

        template <typename... Args>
        constexpr const viewed_t& emplace_viewed(Args&&... args) noexcept(noexcept(this->impl.template emplace<viewed_t>(std::forward<Args>(args)...))) {
            return this->impl.template emplace<viewed_t>(std::forward<Args>(args)...);
        }

        template <typename... Args>
        constexpr owned_t& emplace_owned(Args&&... args) noexcept(noexcept(this->impl.template emplace<owned_t>(std::forward<Args>(args)...))) {
            return this->impl.template emplace<owned_t>(std::forward<Args>(args)...);
        }

        // ReSharper disable once CppNonExplicitConversionOperator
        constexpr explicit(false) operator viewed_t() const { // NOLINT(*-explicit-constructor)
            return this->viewed();
        }

    private:
        impl_t impl;
    };

    template <typename CharT, typename Traits>
    struct cow_traits<std::basic_string_view<CharT, Traits>> {
        using viewed_t = std::basic_string_view<CharT, Traits>;
        using owned_t = std::basic_string<CharT, Traits>;

        template <typename>
        static constexpr bool allow_viewed_conversion = true;

        static constexpr viewed_t into_viewed(const owned_t& owned) noexcept {
            return owned;
        }

        static constexpr owned_t into_owned(const viewed_t& viewed) {
            return owned_t{ viewed };
        }
    };

    template <typename CharT, typename Traits>
    cow(std::basic_string_view<CharT, Traits> str) -> cow<std::basic_string_view<CharT, Traits>>;

    template <typename CharT, typename Traits = std::char_traits<CharT>>
    using basic_cow_string = cow<std::basic_string_view<CharT, Traits>>;

    using cow_string = basic_cow_string<char>;
    using cow_u8string = basic_cow_string<char8_t>;
    using cow_u16string = basic_cow_string<char16_t>;
    using cow_u32string = basic_cow_string<char32_t>;
    using cow_wstring = basic_cow_string<wchar_t>;


    template <typename CharT, typename Traits>
    struct cow_traits<basic_cstring_view<CharT, Traits>> {
        using viewed_t = basic_cstring_view<CharT, Traits>;
        using owned_t = std::basic_string<CharT, Traits>;

        template <typename>
        static constexpr bool allow_viewed_conversion = true;

        static constexpr viewed_t into_viewed(const owned_t& owned) noexcept {
            return owned;
        }

        static constexpr owned_t into_owned(const viewed_t& viewed) {
            return owned_t{ viewed };
        }
    };

    template <typename CharT, typename Traits>
    cow(basic_cstring_view<CharT, Traits>) -> cow<basic_cstring_view<CharT, Traits>>;

    template <typename CharT, typename Traits = std::char_traits<CharT>>
    using basic_cow_cstring = cow<basic_cstring_view<CharT, Traits>>;

    using cow_cstring = basic_cow_cstring<char>;
    using cow_u8cstring = basic_cow_cstring<char8_t>;
    using cow_u16cstring = basic_cow_cstring<char16_t>;
    using cow_u32cstring = basic_cow_cstring<char32_t>;
    using cow_wcstring = basic_cow_cstring<wchar_t>;


    template <typename T>
    struct cow_traits<std::span<T>> {
        using viewed_t = std::span<T>;
        using owned_t = std::vector<std::remove_const_t<T>>;

        template <typename>
        static constexpr bool allow_viewed_conversion = true;

        static constexpr viewed_t into_viewed(const owned_t& owned) noexcept {
            return owned;
        }

        static constexpr owned_t into_owned(const viewed_t& viewed) {
            return owned_t{ viewed.begin(), viewed.end() };
        }
    };

    template <typename T>
    using cow_vector = cow<std::span<T>>;
}
