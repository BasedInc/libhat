#pragma once

#include "platform.h"

#ifdef _MSC_VER
    #define LIBHAT_RETURN_ADDRESS() _ReturnAddress()
#else
    #define LIBHAT_RETURN_ADDRESS() __builtin_extract_return_addr(__builtin_return_address(0))
#endif

#if __cpp_if_consteval >= 202106L
    #define LIBHAT_IF_CONSTEVAL consteval
#else
    #include <type_traits>
    #define LIBHAT_IF_CONSTEVAL (std::is_constant_evaluated())
#endif

#if __has_cpp_attribute(likely)
    #define LIBHAT_LIKELY [[likely]]
#else
    #define LIBHAT_LIKELY
#endif

#if __has_cpp_attribute(unlikely)
    #define LIBHAT_UNLIKELY [[unlikely]]
#else
    #define LIBHAT_UNLIKELY
#endif

#if __cpp_lib_unreachable >= 202202L
    #include <utility>
    #define LIBHAT_UNREACHABLE() std::unreachable()
#elif defined(__GNUC__) || defined(__clang__)
    #define LIBHAT_UNREACHABLE() __builtin_unreachable()
#elif defined(_MSC_VER)
    #define LIBHAT_UNREACHABLE() __assume(false)
#else
    #include <cstdlib>
    namespace hat::detail {
        [[noreturn]] inline void unreachable_impl() noexcept {
            std::abort();
        }
    }
    #define LIBHAT_UNREACHABLE() hat::detail::unreachable_impl()
#endif

#if __has_cpp_attribute(assume)
    #define LIBHAT_ASSUME(...) [[assume(__VA_ARGS__)]]
#elif defined(__clang__)
    #define LIBHAT_ASSUME(...) __builtin_assume(__VA_ARGS__)
#elif defined(_MSC_VER)
    #define LIBHAT_ASSUME(...) __assume(__VA_ARGS__)
#else
    #define LIBHAT_ASSUME(...)        \
        do {                          \
            if (!(__VA_ARGS__)) {     \
                LIBHAT_UNREACHABLE(); \
            }                         \
        } while (0)
#endif

#if defined(__GNUC__) || defined(__clang__)
    #define LIBHAT_FORCEINLINE inline __attribute__((always_inline))
#elif defined(_MSC_VER)
    #define LIBHAT_FORCEINLINE __forceinline
#else
    #define LIBHAT_FORCEINLINE inline
#endif

#if __has_cpp_attribute(no_unique_address)
    #define LIBHAT_NO_UNIQUE_ADDRESS [[no_unique_address]]
#elif __has_cpp_attribute(msvc::no_unique_address)
    #define LIBHAT_NO_UNIQUE_ADDRESS [[msvc::no_unique_address]]
#else
    #define LIBHAT_NO_UNIQUE_ADDRESS
#endif