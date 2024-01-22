#pragma once

#include <type_traits>

namespace hat {

    template<typename To, typename From>
    struct constness_as : std::type_identity<std::remove_const_t<To>> {};

    template<typename To, typename From>
    struct constness_as<To, const From> : std::type_identity<const To> {};

    template<typename To, typename From>
    using constness_as_t = typename constness_as<To, From>::type;
}
