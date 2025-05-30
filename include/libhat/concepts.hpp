#pragma once

#ifndef LIBHAT_MODULE
    #include <concepts>
    #include <cstddef>
    #include <iterator>
    #include <ranges>
    #include <type_traits>
#endif

#include "export.hpp"

LIBHAT_EXPORT namespace hat {

    template<typename T>
    concept integer = std::integral<T> && !std::is_same_v<std::decay_t<T>, bool>;
}

namespace hat::detail {

    template<typename Fn, typename Ret>
    concept supplier = requires(Fn&& fn) {
        { fn() } -> std::same_as<Ret>;
    };

    template<typename Fn>
    concept function = std::is_function_v<Fn>;

    template<typename From, typename To>
    concept reinterpret_as = sizeof(To) == sizeof(From)
                             && std::is_trivially_copyable_v<From>
                             && std::is_trivially_copyable_v<To>
                             && std::is_trivially_constructible_v<To>;

    template<typename T>
    concept byte_input_iterator = std::input_iterator<T>
                            && std::forward_iterator<T>
                            && std::contiguous_iterator<T>
                            && std::same_as<std::iter_value_t<T>, std::byte>;

    template<typename T>
    concept byte_input_range = std::ranges::input_range<T>
                            && std::ranges::contiguous_range<T>
                            && std::ranges::sized_range<T>
                            && std::same_as<std::ranges::range_value_t<T>, std::byte>;

    template<typename T>
    concept char_iterator = std::is_same_v<std::iter_value_t<T>, char>;

    template<typename T>
    concept non_const = !std::is_const_v<T>;
}
