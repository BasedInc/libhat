#pragma once
#include <span>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include "cstring_view.hpp"

namespace hat {
    /// Tag type that tells `cow` that allocator support is not needed.
    struct no_allocator {
        no_allocator() = delete;
    };

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
    template <typename T, typename Traits = cow_traits<T>, typename Allocator = typename Traits::default_allocator_t>
    requires std::is_trivially_copyable_v<T>
    struct cow {
        using traits_t = Traits;
        using viewed_t = T;
        using owned_t = typename traits_t::template owned_t<Allocator>;

        static constexpr bool uses_allocator = !std::is_same_v<typename Traits::default_allocator_t, no_allocator>;

        static_assert(uses_allocator || std::is_same_v<Allocator, no_allocator>, "The traits for this cow do not support allocators");

    private:
        using impl_t = std::variant<
            std::conditional_t<uses_allocator, std::pair<viewed_t, Allocator>, viewed_t>,
            owned_t>;

        const viewed_t& viewed_unchecked() const {
            if constexpr (uses_allocator) {
                return std::get<0>(this->impl).first;
            } else {
                return std::get<0>(this->impl);
            }
        }

    public:
        constexpr cow() noexcept(std::is_nothrow_default_constructible_v<impl_t>) requires(std::is_default_constructible_v<impl_t>) = default;

        template <typename... Args>
        requires (uses_allocator)
        constexpr explicit cow(in_place_viewed_t, Args&&... args) noexcept(std::is_nothrow_constructible_v<viewed_t, Args&&...>)
            : impl{ std::in_place_index<0>, std::piecewise_construct, std::forward_as_tuple<Args...>(args...), std::forward_as_tuple(Allocator{}) } {}

        template <typename... Args>
        requires (!uses_allocator)
        constexpr explicit cow(in_place_viewed_t, Args&&... args) noexcept(std::is_nothrow_constructible_v<viewed_t, Args&&...>)
            : impl{ std::in_place_index<0>, std::forward<Args>(args)... } {}

        template <typename... Args>
        constexpr explicit cow(in_place_owned_t, Args&&... args) noexcept(std::is_nothrow_constructible_v<owned_t, Args&&...>)
            : impl{ std::in_place_type<owned_t>, std::forward<Args>(args)... } {}

        constexpr explicit(false) cow(viewed_t view) noexcept : cow(in_place_viewed, view) {}
        constexpr explicit cow(owned_t owned) noexcept(std::is_nothrow_move_constructible_v<owned_t>) : cow(in_place_owned, std::move(owned)) {}

        template <std::convertible_to<viewed_t> U>
        requires traits_t::template allow_viewed_conversion<U>
        constexpr explicit(false) cow(U&& val) : cow(viewed_t{std::forward<U>(val)}) {}

        [[nodiscard]] constexpr bool is_viewed() const noexcept {
            return this->impl.index() == 0;
        }

        [[nodiscard]] constexpr bool is_owned() const noexcept {
            return std::holds_alternative<owned_t>(this->impl);
        }

        [[nodiscard]] constexpr viewed_t viewed() const noexcept(noexcept(traits_t::into_viewed(std::declval<const owned_t&>()))) {
            if (this->is_viewed()) {
                return this->viewed_unchecked();
            }

            return traits_t::into_viewed(std::get<owned_t>(this->impl));
        }

        [[nodiscard]] constexpr owned_t& owned() & noexcept(noexcept(traits_t::into_owned(std::declval<viewed_t&>()))) requires(!uses_allocator) {
            if (this->is_owned()) {
                return std::get<owned_t>(this->impl);
            }

            return this->impl.template emplace<owned_t>(traits_t::into_owned(this->viewed_unchecked()));
        }

        [[nodiscard]] constexpr owned_t& owned() & noexcept(noexcept(traits_t::into_owned(std::declval<viewed_t&>(), std::declval<Allocator>()))) requires(uses_allocator) {
            if (this->is_owned()) {
                return std::get<owned_t>(this->impl);
            }

            auto& [viewed, allocator] = std::get<0>(this->impl);
            return this->impl.template emplace<owned_t>(traits_t::into_owned(viewed, allocator));
        }

        [[nodiscard]] constexpr owned_t&& owned() && noexcept(noexcept(this->owned())) {
            return std::move(this->owned());
        }

        [[nodiscard]] constexpr Allocator get_allocator() const noexcept(noexcept(traits_t::get_allocator(std::declval<const owned_t&>()))) requires(uses_allocator) {
            if (this->is_viewed()) {
                return std::get<0>(this->impl).second;
            }

            return traits_t::get_allocator(std::get<owned_t>(this->impl));
        }

        constexpr void ensure_owned() {
            std::ignore = this->owned();
        }

        template <typename... Args>
        constexpr const viewed_t& emplace_viewed(Args&&... args) noexcept(std::is_nothrow_constructible_v<viewed_t, Args&&...>) {
            if constexpr (uses_allocator) {
                // TODO: Since the allocator won't be in use anymore, it might make sense to allow to change the allocator
                // If we are holding an owned value, we need to take the allocator back out.
                return this->impl.template emplace<0>(
                    std::piecewise_construct,
                    std::forward_as_tuple<Args...>(args...),
                    std::forward_as_tuple(this->get_allocator())).first;
            } else {
                return this->impl.template emplace<0>(std::forward<Args>(args)...);
            }
        }

        template <typename... Args>
        constexpr owned_t& emplace_owned(Args&&... args) noexcept(noexcept(this->impl.template emplace<owned_t>(std::forward<Args>(args)...))) {
            // TODO: Maybe allow to change the allocator here?
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
        using default_allocator_t = std::allocator<CharT>;

        using viewed_t = std::basic_string_view<CharT, Traits>;

        template <typename Alloc>
        using owned_t = std::basic_string<CharT, Traits, Alloc>;

        template <typename>
        static constexpr bool allow_viewed_conversion = true;

        template <typename Alloc>
        static constexpr viewed_t into_viewed(const owned_t<Alloc>& owned) noexcept {
            return owned;
        }

        template <typename Alloc>
        static constexpr owned_t<Alloc> into_owned(const viewed_t& viewed, Alloc alloc) {
            return std::make_obj_using_allocator<owned_t<Alloc>>(alloc, viewed);
        }

        template <typename Alloc>
        static constexpr Alloc get_allocator(const owned_t<Alloc>& owned) noexcept {
            return owned.get_allocator();
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
        using default_allocator_t = std::allocator<CharT>;

        using viewed_t = basic_cstring_view<CharT, Traits>;

        template <typename Alloc>
        using owned_t = std::basic_string<CharT, Traits, Alloc>;

        template <typename>
        static constexpr bool allow_viewed_conversion = true;

        template <typename Alloc>
        static constexpr viewed_t into_viewed(const owned_t<Alloc>& owned) noexcept {
            return owned;
        }

        template <typename Alloc>
        static constexpr owned_t<Alloc> into_owned(const viewed_t& viewed, Alloc alloc) {
            return std::make_obj_using_allocator<owned_t<Alloc>>(alloc, viewed);
        }

        template <typename Alloc>
        static constexpr Alloc get_allocator(const owned_t<Alloc>& owned) noexcept {
            return owned.get_allocator();
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
        using default_allocator_t = std::allocator<T>;

        using viewed_t = std::span<T>;

        template <typename Alloc>
        using owned_t = std::vector<std::remove_const_t<T>, Alloc>;

        template <typename>
        static constexpr bool allow_viewed_conversion = true;

        template <typename Alloc>
        static constexpr viewed_t into_viewed(const owned_t<Alloc>& owned) noexcept {
            return owned;
        }

        template <typename Alloc>
        static constexpr owned_t<Alloc> into_owned(const viewed_t& viewed, Alloc alloc) {
            return std::make_obj_using_allocator<owned_t<Alloc>>(alloc, viewed.begin(), viewed.end());
        }

        template <typename Alloc>
        static constexpr Alloc get_allocator(const owned_t<Alloc>& owned) noexcept {
            return owned.get_allocator();
        }
    };

    template <typename T>
    using cow_vector = cow<std::span<T>>;
}
