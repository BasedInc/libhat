#pragma once

// Detect CPU Architecture
#if defined(_M_X64) || defined(__amd64__) || defined(_M_IX86) || defined(__i386__)
    #define LIBHAT_X86
    #if defined(_M_X64) || defined(__amd64__)
        #define LIBHAT_X86_64
    #endif
#elif defined(_M_ARM64) || defined(__aarch64__) || defined(_M_ARM) || defined(__arm__)
    #define LIBHAT_ARM
#else
    #error Unsupported Architecture
#endif

// Detect Operating System
#if defined(_WIN32)
    #define LIBHAT_WINDOWS
#else
    #error Unsupported Operating System
#endif

// Macros wrapping intrinsics
#ifdef LIBHAT_X86
    #ifdef LIBHAT_X86_64
        #define LIBHAT_TZCNT64(num) _tzcnt_u64(num)
        #define LIBHAT_BLSR64(num) _blsr_u64(num)
    #else
        #include <bit>
        #define LIBHAT_TZCNT64(num) std::countl_zero(num)
        #define LIBHAT_BLSR64(num) num & (num - 1)
    #endif
#endif

#if __cpp_if_consteval >= 202106L
    #define LIBHAT_IF_CONSTEVAL consteval
#else
    #define LIBHAT_IF_CONSTEVAL (std::is_constant_evaluated())
#endif
