module;

#ifndef LIBHAT_USE_STD_MODULE
    #include <algorithm>
    #include <array>
    #include <bit>
    #include <concepts>
    #include <cstddef>
    #include <cstdint>
    #include <cstdlib>
    #include <cstring>
    #include <execution>
    #include <functional>
    #include <iterator>
    #include <memory>
    #include <memory_resource>
    #include <new>
    #include <optional>
    #include <ranges>
    #include <span>
    #include <string>
    #include <string_view>
    #include <tuple>
    #include <type_traits>
    #include <utility>
    #include <variant>
    #include <vector>
    #if __has_include(<expected>)
        #include <expected>
    #endif
#endif

#include <version>

export module libhat;

#ifdef LIBHAT_USE_STD_MODULE
    import std.compat;
#endif

extern "C++" {
#define LIBHAT_MODULE
#include "libhat.hpp"
}
