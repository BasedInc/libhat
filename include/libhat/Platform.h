#pragma once

// Detect CPU Architecture
#if defined(_M_X64) || defined(__amd64__)
    #define LIBHAT_X86_64
#elif defined(_M_IX86) || defined(__i386__)
    #define LIBHAT_X86
#elif defined(_M_ARM64) || defined(__aarch64__)
    #define LIBHAT_AARCH64
#elif defined(_M_ARM) || defined(__arm__)
    #define LIBHAT_ARM
#else
    #error Unsupported Architecture
#endif

// Detect Operating System
#if defined(_WIN32)
    #define LIBHAT_WINDOWS
#elif defined(linux) || defined(__linux__) || defined(__linux)
    #define LIBHAT_UNIX
    #define LIBHAT_LINUX
#elif defined(__APPLE__) && defined(__MACH__)
    #define LIBHAT_UNIX
    #define LIBHAT_MAC
#else
    #error Unsupported Operating System
#endif
