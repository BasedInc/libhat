#pragma once

#include "Platform.h"

// Macros wrapping intrinsics
#if defined(LIBHAT_X86_64)
    #define LIBHAT_TZCNT64(num) _tzcnt_u64(num)
    #define LIBHAT_BLSR64(num) _blsr_u64(num)
#elif defined(LIBHAT_X86)
    #include <bit>
    #define LIBHAT_TZCNT64(num) std::countl_zero(num)
    #define LIBHAT_BLSR64(num) num & (num - 1)
#endif

#ifdef _MSC_VER
    #include <intrin.h>

    namespace hat::detail {
        inline unsigned long bsf(unsigned long num) {
            unsigned long offset;
            _BitScanForward(&offset, num);
            return offset;
        }
    }

    #define LIBHAT_RETURN_ADDRESS() _ReturnAddress()
    #define LIBHAT_BSF32(num) hat::detail::bsf(num)
#else
    #define LIBHAT_RETURN_ADDRESS() __builtin_extract_return_addr(__builtin_return_address(0))
    #define LIBHAT_BSF32(num) __builtin_ctz(num)
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
        [[noreturn]] inline void unreachable_impl() {
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
