#pragma once

#include <type_traits>

namespace hat::detail {

    template<typename T>
    void implicitly_default_construct(const T&);
}

namespace hat {

    template<typename T>
    struct is_implicitly_default_constructible : std::false_type {};

    template<typename T> requires requires { hat::detail::implicitly_default_construct<T>({}); }
    struct is_implicitly_default_constructible<T> : std::true_type {};

    template<typename T>
    constexpr bool is_implicitly_default_constructible_v = is_implicitly_default_constructible<T>::value;

    template<typename To, typename From>
    struct constness_as : std::type_identity<std::remove_const_t<To>> {};

    template<typename To, typename From>
    struct constness_as<To, const From> : std::type_identity<const To> {};

    template<typename To, typename From>
    using constness_as_t = typename constness_as<To, From>::type;
}
