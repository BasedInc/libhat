#pragma once

#include "concepts.hpp"

namespace hat {

    template<hat::integer T>
    struct div_t {
        T quot;
        T rem;
    };

    /// Generic version of <cstdlib>'s div, with the added benefit of returning a type that isn't reliant
    /// on the implementation defined ordering of the "quot" and "rem" members in div_t.
    template<hat::integer T>
    constexpr div_t<T> div(const T numerator, const T denominator) {
        return {
            static_cast<T>(numerator / denominator),
            static_cast<T>(numerator % denominator)
        };
    }
}
