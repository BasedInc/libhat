#pragma once

#ifndef LIBHAT_MODULE
    #include <type_traits>
#endif

#include "export.hpp"

LIBHAT_EXPORT namespace hat {

    template<typename To, typename From>
    struct constness_as : std::type_identity<std::remove_const_t<To>> {};

    template<typename To, typename From>
    struct constness_as<To, const From> : std::type_identity<const To> {};

    template<typename To, typename From>
    using constness_as_t = typename constness_as<To, From>::type;
}
