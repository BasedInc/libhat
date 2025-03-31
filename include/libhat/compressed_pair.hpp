#pragma once

#include "defines.hpp"
#include "type_traits.hpp"

#include <tuple>

namespace hat::detail {

    template<typename T1, typename T2>
    struct compressed_pair {
        using first_type = T1;
        using second_type = T2;

    private:
        static constexpr bool explicit_default_ctor = !hat::is_implicitly_default_constructible_v<first_type>
            || !hat::is_implicitly_default_constructible_v<second_type>;

        static constexpr bool explicit_copy_init = !std::is_convertible_v<const first_type&, first_type>
            || !std::is_convertible_v<const second_type&, second_type>;

        template<typename U1, typename U2>
        static constexpr bool explicit_forward_init = !std::is_convertible_v<U1, first_type>
            || !std::is_convertible_v<U2, second_type>;

    public:
        LIBHAT_NO_UNIQUE_ADDRESS first_type first;
        LIBHAT_NO_UNIQUE_ADDRESS second_type second;

        constexpr explicit(explicit_default_ctor) compressed_pair()
            noexcept(std::is_nothrow_default_constructible_v<first_type> && std::is_nothrow_default_constructible_v<second_type>)
            requires(std::is_default_constructible_v<first_type> && std::is_default_constructible_v<second_type>) = default;

        constexpr explicit(explicit_copy_init) compressed_pair(const first_type& first, const second_type& second)
            requires(std::is_copy_constructible_v<first_type> && std::is_copy_constructible_v<second_type>)
            : first(first), second(second) {}

        template<typename U1, typename U2>
        constexpr explicit(explicit_forward_init<U1, U2>) compressed_pair(U1&& first, U2&& second)
            requires(std::is_constructible_v<first_type, U1> && std::is_constructible_v<second_type, U2>)
            : first(std::forward<U1>(first)), second(std::forward<U2>(second)) {}

        template<typename... Args1, typename... Args2>
        constexpr compressed_pair(std::piecewise_construct_t, std::tuple<Args1...> firstArgs, std::tuple<Args2...> secondArgs)
            : compressed_pair(firstArgs, secondArgs, std::index_sequence_for<Args1...>{}, std::index_sequence_for<Args2...>{}) {}

    private:
        template<typename Tuple1, typename Tuple2, size_t... Indices1, size_t... Indices2>
        constexpr compressed_pair(Tuple1& first, Tuple2& second, std::index_sequence<Indices1...>, std::index_sequence<Indices2...>)
            : first(std::get<Indices1>(std::move(first))...), second(std::get<Indices2>(std::move(second))...) {}
    };
}
