#pragma once

#include <algorithm>
#include <memory_resource>
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

    namespace detail {
        template<typename Traits, typename Alloc>
        struct owned_type {
            using type = typename Traits::template owned_t<Alloc>;
        };

        template<typename Traits>
        struct owned_type<Traits, no_allocator> {
            using type = typename Traits::owned_t;
        };
    }

    struct in_place_viewed_t{};
    struct in_place_owned_t{};

    /// Construct a cow's viewed type in place.
    static constexpr in_place_viewed_t in_place_viewed;
    /// Construct a cow's owned type in place.
    static constexpr in_place_owned_t in_place_owned;

    template<typename T>
    struct cow_traits;

    /**
     * A copy-on-write wrapper for dynamically-allocated containers. It's also useful for representing potentially owned data.
     *
     * @tparam T A type representing a non-owning view to the data.
     * @tparam Traits A traits type describing how to convert T to an owned container, among other things.
     * @tparam Allocator An allocator to use for the owned container.
     */
    template<typename T, typename Traits = cow_traits<T>, typename Allocator = typename Traits::default_allocator_t>
    requires std::is_trivially_copyable_v<T>
    struct cow {
        // Required by STL
        using allocator_type = Allocator;

        using traits_t = Traits;

        static constexpr bool uses_allocator = !std::is_same_v<typename traits_t::default_allocator_t, no_allocator>;

        static_assert(uses_allocator || std::is_same_v<allocator_type, no_allocator>, "The traits for this cow do not support allocators");
        static_assert(!uses_allocator || !std::is_same_v<allocator_type, no_allocator>, "The traits for this cow do not accept no_allocator");

        using viewed_t = T;
        using owned_t = typename detail::owned_type<traits_t, allocator_type>::type;

    private:
        static constexpr bool default_construct_alloc = !uses_allocator || std::is_default_constructible_v<allocator_type>;

        using impl_t = std::variant<
            std::conditional_t<uses_allocator, std::pair<viewed_t, allocator_type>, viewed_t>,
            owned_t>;

        constexpr const viewed_t& viewed_unchecked() const {
            if constexpr (uses_allocator) {
                return std::get<0>(this->impl).first;
            } else {
                return std::get<0>(this->impl);
            }
        }

    public:
        // Default constructor
        constexpr cow() noexcept(std::is_nothrow_default_constructible_v<impl_t>) requires(std::is_default_constructible_v<impl_t>) = default;

        // Default construct with an allocator
        constexpr explicit cow(const allocator_type& alloc)
            noexcept(std::is_nothrow_default_constructible_v<viewed_t>)
            requires(uses_allocator && std::is_default_constructible_v<viewed_t>)
            : impl(std::in_place_index<0>, std::piecewise_construct, std::tuple{}, std::forward_as_tuple(alloc)) {}

        // Copy constructor for not uses_allocator
        constexpr cow(const cow&) noexcept(std::is_nothrow_copy_constructible_v<impl_t>) requires(!uses_allocator) = default;

        // Copy constructor for uses_allocator (per the standard, we need to do these methods to construct it)
        constexpr cow(const cow& other) noexcept(std::is_nothrow_copy_constructible_v<owned_t>) requires(uses_allocator)
            : impl(other.is_viewed()
                    ? impl_t{ std::in_place_index<0>, other.viewed_unchecked(), std::allocator_traits<allocator_type>::select_on_container_copy_construction(other.get_allocator()) }
                    : other.impl) {}

        // Move constructor, behavior doesn't vary with uses_allocator
        constexpr cow(cow&&) noexcept(!std::is_nothrow_move_constructible_v<impl_t>) = default;

        // In-place constructor for viewed_t for not no_allocator, default initializes the allocator
        template<typename... Args>
        requires (uses_allocator)
        constexpr explicit cow(in_place_viewed_t, Args&&... args) noexcept(std::is_nothrow_constructible_v<viewed_t, Args&&...>) requires(default_construct_alloc)
            : impl{ std::in_place_index<0>, std::piecewise_construct, std::forward_as_tuple<Args...>(args...), std::forward_as_tuple(allocator_type{}) } {}

        // In-place constructor for viewed_t for not_allocator, takes in an allocator
        template<typename... Args>
        requires (uses_allocator)
        constexpr cow(std::allocator_arg_t, const allocator_type& alloc, in_place_viewed_t, Args&&... args) noexcept(std::is_nothrow_constructible_v<viewed_t, Args&&...>)
            : impl{ std::in_place_index<0>, std::piecewise_construct, std::forward_as_tuple<Args...>(args...), std::forward_as_tuple(alloc) } {}

        // In-place constructor for viewed_t with no_allocator
        template<typename... Args>
        requires (!uses_allocator)
        constexpr explicit cow(in_place_viewed_t, Args&&... args) noexcept(std::is_nothrow_constructible_v<viewed_t, Args&&...>)
            : impl{ std::in_place_index<0>, std::forward<Args>(args)... } {}

        // In-place constructor for owned_t, doesn't take an allocator
        template<typename... Args>
        constexpr explicit cow(in_place_owned_t, Args&&... args) noexcept(std::is_nothrow_constructible_v<owned_t, Args&&...>)
            : impl{ std::in_place_type<owned_t>, std::forward<Args>(args)... } {}

        // In-place constructor for owned_t that takes an allocator. Needs some hacks to push the allocator into the underlying owned type.
        template<typename... Args>
        constexpr explicit cow(std::allocator_arg_t, const allocator_type& alloc, in_place_owned_t, Args&&... args) noexcept(std::is_nothrow_constructible_v<owned_t, Args&&...>)
            : cow(owned_from_tuple_hack{}, std::uses_allocator_construction_args<owned_t>(alloc, std::forward<Args>(args)...)) {}

        // Construct with viewed_t, doesn't take an allocator
        constexpr explicit(false) cow(viewed_t view) noexcept requires(default_construct_alloc) : cow(in_place_viewed, view) {}

        // Construct with viewed_t and an allocator
        constexpr cow(viewed_t view, const allocator_type& alloc) noexcept requires(uses_allocator) : cow(std::allocator_arg, alloc, in_place_viewed, view) {}

        // Construct with owned_t, doesn't take an allocator
        constexpr explicit cow(owned_t owned) noexcept(std::is_nothrow_move_constructible_v<owned_t>) requires(default_construct_alloc) : cow(in_place_owned, std::move(owned)) {}

        // Construct with owned_t and an allocator
        constexpr cow(owned_t owned, const allocator_type& alloc) noexcept(std::is_nothrow_move_constructible_v<owned_t>) : cow(std::allocator_arg, alloc, in_place_owned, std::move(owned)) {}

        // Convert U to viewed_t and construct, doesn't take an allocator
        template<std::convertible_to<viewed_t> U>
        requires traits_t::template allow_viewed_conversion<U>
        constexpr explicit(false) cow(U&& val) requires(default_construct_alloc) : cow(viewed_t{std::forward<U>(val)}) {}

        // Convert U to viewed_t and construct, takes an allocator
        template<std::convertible_to<viewed_t> U>
        requires traits_t::template allow_viewed_conversion<U>
        constexpr explicit(false) cow(U&& val, const allocator_type& alloc) : cow(viewed_t{std::forward<U>(val)}, alloc) {}

        constexpr cow& operator=(const cow&) noexcept(std::is_nothrow_copy_assignable_v<impl_t>) requires(!uses_allocator) = default;
        constexpr cow& operator=(cow&&) noexcept(std::is_nothrow_move_assignable_v<impl_t>) requires(!uses_allocator) = default;

        constexpr cow& operator=(const cow& other) noexcept(std::is_nothrow_copy_assignable_v<impl_t>) requires(uses_allocator) {
            if constexpr (std::allocator_traits<allocator_type>::propagate_on_container_copy_assignment::value) {
                this->impl = other.impl;
            } else {
                if (other.is_viewed()) {
                    this->impl.template emplace<0>(other.viewed_unchecked(), this->get_allocator());
                } else {
                    this->impl.template emplace<1>(std::make_obj_using_allocator<owned_t>(this->get_allocator(), std::get<owned_t>(other.impl)));
                }
            }
            return *this;
        }

        constexpr cow& operator=(cow&& other) noexcept(std::is_nothrow_move_assignable_v<impl_t>) requires(uses_allocator) {
            if constexpr (std::allocator_traits<allocator_type>::propagate_on_container_move_assignment::value) {
                this->impl = std::move(other.impl);
            } else {
                if (other.is_viewed()) {
                    this->impl.template emplace<0>(other.viewed_unchecked(), this->get_allocator());
                } else {
                    this->impl.template emplace<1>(std::make_obj_using_allocator<owned_t>(this->get_allocator(), std::get<owned_t>(std::move(other.impl))));
                }
            }
            return *this;
        }

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

        [[nodiscard]] constexpr owned_t& owned() & noexcept(noexcept(traits_t::into_owned(std::declval<viewed_t&>(), std::declval<allocator_type>()))) requires(uses_allocator) {
            if (this->is_owned()) {
                return std::get<owned_t>(this->impl);
            }

            auto& [viewed, allocator] = std::get<0>(this->impl);
            return this->impl.template emplace<owned_t>(traits_t::into_owned(viewed, allocator));
        }

        [[nodiscard]] constexpr owned_t&& owned() && noexcept(noexcept(this->owned())) {
            return std::move(this->owned());
        }

        [[nodiscard]] constexpr allocator_type get_allocator() const noexcept(noexcept(traits_t::get_allocator(std::declval<const owned_t&>()))) requires(uses_allocator) {
            if (this->is_viewed()) {
                return std::get<0>(this->impl).second;
            }

            return traits_t::get_allocator(std::get<owned_t>(this->impl));
        }

        constexpr void ensure_owned() {
            std::ignore = this->owned();
        }

        template<typename... Args>
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

        template<typename... Args>
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

        // We figure out the args needed to construct owned_t with an allocator using uses_allocator_construction_args, which returns a tuple.
        // Since it's a tuple, we need these two hacky constructors to get those arguments into the std::variant constructor in-place.

        struct owned_from_tuple_hack {};

        template<typename... Args>
        constexpr explicit cow(owned_from_tuple_hack, std::tuple<Args...> args) : cow(std::move(args), std::make_index_sequence<sizeof...(Args)>{}) {}

        template<typename... Args, size_t... Idxs>
        constexpr explicit cow(std::tuple<Args...> args, std::index_sequence<Idxs...>) : cow(in_place_owned, std::get<Idxs>(std::move(args))...) {}
    };

    template<typename CharT, typename Traits>
    struct cow_traits<std::basic_string_view<CharT, Traits>> {
        using default_allocator_t = std::allocator<CharT>;

        using viewed_t = std::basic_string_view<CharT, Traits>;

        template<typename Alloc>
        using owned_t = std::basic_string<CharT, Traits, Alloc>;

        template<typename>
        static constexpr bool allow_viewed_conversion = true;

        template<typename Alloc>
        static constexpr viewed_t into_viewed(const owned_t<Alloc>& owned) noexcept {
            return owned;
        }

        template<typename Alloc>
        static constexpr owned_t<Alloc> into_owned(const viewed_t& viewed, Alloc alloc) {
            return std::make_obj_using_allocator<owned_t<Alloc>>(alloc, viewed);
        }

        template<typename Alloc>
        static constexpr Alloc get_allocator(const owned_t<Alloc>& owned) noexcept {
            return owned.get_allocator();
        }
    };

    template<typename CharT, typename Traits>
    cow(std::basic_string_view<CharT, Traits> str) -> cow<std::basic_string_view<CharT, Traits>>;

    template<typename CharT, typename Traits = std::char_traits<CharT>, typename Alloc = std::allocator<CharT>>
    using basic_cow_string = cow<std::basic_string_view<CharT, Traits>, cow_traits<std::basic_string_view<CharT, Traits>>, Alloc>;

    using cow_string = basic_cow_string<char>;
    using cow_u8string = basic_cow_string<char8_t>;
    using cow_u16string = basic_cow_string<char16_t>;
    using cow_u32string = basic_cow_string<char32_t>;
    using cow_wstring = basic_cow_string<wchar_t>;


    template<typename CharT, typename Traits>
    struct cow_traits<basic_cstring_view<CharT, Traits>> {
        using default_allocator_t = std::allocator<CharT>;

        using viewed_t = basic_cstring_view<CharT, Traits>;

        template<typename Alloc>
        using owned_t = std::basic_string<CharT, Traits, Alloc>;

        template<typename>
        static constexpr bool allow_viewed_conversion = true;

        template<typename Alloc>
        static constexpr viewed_t into_viewed(const owned_t<Alloc>& owned) noexcept {
            return owned;
        }

        template<typename Alloc>
        static constexpr owned_t<Alloc> into_owned(const viewed_t& viewed, Alloc alloc) {
            return std::make_obj_using_allocator<owned_t<Alloc>>(alloc, viewed);
        }

        template<typename Alloc>
        static constexpr Alloc get_allocator(const owned_t<Alloc>& owned) noexcept {
            return owned.get_allocator();
        }
    };

    template<typename CharT, typename Traits>
    cow(basic_cstring_view<CharT, Traits>) -> cow<basic_cstring_view<CharT, Traits>>;

    template<typename CharT, typename Traits = std::char_traits<CharT>, typename Alloc = std::allocator<CharT>>
    using basic_cow_cstring = cow<basic_cstring_view<CharT, Traits>, cow_traits<basic_cstring_view<CharT, Traits>>, Alloc>;

    using cow_cstring = basic_cow_cstring<char>;
    using cow_u8cstring = basic_cow_cstring<char8_t>;
    using cow_u16cstring = basic_cow_cstring<char16_t>;
    using cow_u32cstring = basic_cow_cstring<char32_t>;
    using cow_wcstring = basic_cow_cstring<wchar_t>;


    template<typename T>
    struct cow_traits<std::span<T>> {
        using default_allocator_t = std::allocator<std::remove_const_t<T>>;

        using viewed_t = std::span<T>;

        template<typename Alloc>
        using owned_t = std::vector<std::remove_const_t<T>, Alloc>;

        template<typename>
        static constexpr bool allow_viewed_conversion = true;

        template<typename Alloc>
        static constexpr viewed_t into_viewed(const owned_t<Alloc>& owned) noexcept {
            return owned;
        }

        template<typename Alloc>
        static constexpr owned_t<Alloc> into_owned(const viewed_t& viewed, Alloc alloc) {
            return std::make_obj_using_allocator<owned_t<Alloc>>(alloc, viewed.begin(), viewed.end());
        }

        template<typename Alloc>
        static constexpr Alloc get_allocator(const owned_t<Alloc>& owned) noexcept {
            return owned.get_allocator();
        }
    };

    template<typename T, typename Alloc = std::allocator<std::remove_const_t<T>>>
    using cow_span = cow<std::span<T>, cow_traits<std::span<T>>, Alloc>;

    namespace pmr {
        template<typename CharT, typename Traits = std::char_traits<CharT>>
        using basic_cow_string = cow<std::basic_string_view<CharT, Traits>, cow_traits<std::basic_string_view<CharT, Traits>>, std::pmr::polymorphic_allocator<CharT>>;

        using cow_string = basic_cow_string<char>;
        using cow_u8string = basic_cow_string<char8_t>;
        using cow_u16string = basic_cow_string<char16_t>;
        using cow_u32string = basic_cow_string<char32_t>;
        using cow_wstring = basic_cow_string<wchar_t>;


        template<typename CharT, typename Traits = std::char_traits<CharT>>
        using basic_cow_cstring = cow<basic_cstring_view<CharT, Traits>, cow_traits<basic_cstring_view<CharT, Traits>>, std::pmr::polymorphic_allocator<CharT>>;

        using cow_cstring = basic_cow_cstring<char>;
        using cow_u8cstring = basic_cow_cstring<char8_t>;
        using cow_u16cstring = basic_cow_cstring<char16_t>;
        using cow_u32cstring = basic_cow_cstring<char32_t>;
        using cow_wcstring = basic_cow_cstring<wchar_t>;


        template<typename T>
        using cow_span = cow_span<T, std::pmr::polymorphic_allocator<std::remove_const_t<T>>>;
    }
}
