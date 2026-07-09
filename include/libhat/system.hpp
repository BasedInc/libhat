#pragma once

#ifndef LIBHAT_MODULE
    #include <string>
#endif

#include "defines.hpp"
#include "export.hpp"

LIBHAT_EXPORT namespace hat {
    struct system_info {
        std::size_t page_size{};

        system_info(const system_info&) = delete;
        system_info& operator=(const system_info&) = delete;
        system_info(system_info&&) = delete;
        system_info& operator=(system_info&&) = delete;
    protected:
        system_info();
    };
}

#if defined(LIBHAT_X86) || defined(LIBHAT_X86_64)
LIBHAT_EXPORT namespace hat {

    struct cpu_extensions {
        bool sse;
        bool sse2;
        bool sse3;
        bool ssse3;
        bool sse41;
        bool sse42;
        bool avx;
        bool avx2;
        bool avx512f;
        bool avx512bw;
        bool popcnt;
        bool bmi;
    };

    struct system_info_x86 : hat::system_info {
        std::string cpu_vendor{};
        std::string cpu_brand{};
        cpu_extensions extensions{};
    private:
        system_info_x86();
        friend const system_info_x86& get_system();
        static const system_info_x86 instance;
    };

    inline constexpr cpu_extensions compiled_extensions{
#if defined(__SSE__) || _M_IX86_FP >= 1
        .sse = true,
#else
        .sse = false,
#endif
#if defined(__SSE2__) || _M_IX86_FP >= 2
        .sse2 = true,
#else
        .sse2 = false,
#endif
#if defined(__SSE3__)
        .sse3 = true,
#else
        .sse3 = false,
#endif
#if defined(__SSSE3__)
        .ssse3 = true,
#else
        .ssse3 = false,
#endif
#if defined(__SSE4_1__)
        .sse41 = true,
#else
        .sse41 = false,
#endif
#if defined(__SSE4_2__)
        .sse42 = true,
#else
        .sse42 = false,
#endif
#if defined(__AVX__)
        .avx = true,
#else
        .avx = false,
#endif
#if defined(__AVX2__)
        .avx2 = true,
#else
        .avx2 = false,
#endif
#if defined(__AVX512F__)
        .avx512f = true,
#else
        .avx512f = false,
#endif
#if defined(__AVX512BW__)
        .avx512bw = true,
#else
        .avx512bw = false,
#endif
#if defined(__POPCNT__)
        .popcnt = true,
#else
        .popcnt = false,
#endif
#if defined(__BMI__)
        .bmi = true,
#else
        .bmi = false,
#endif
    };

    using system_info_impl = system_info_x86;
}
#elif defined(LIBHAT_ARM) || defined(LIBHAT_AARCH64)
LIBHAT_EXPORT namespace hat {

    struct cpu_extensions {
        bool neon;
    };

    struct system_info_arm : hat::system_info {
        cpu_extensions extensions{};
    private:
        system_info_arm();
        friend const system_info_arm& get_system();
        static const system_info_arm instance;
    };

    using system_info_impl = system_info_arm;

    inline constexpr cpu_extensions compiled_extensions{
#if defined(__ARM_NEON) || defined(LIBHAT_WINDOWS) || defined(LIBHAT_MAC)
        .neon = true,
#else
        .neon = false,
#endif
    };
}
#endif

LIBHAT_EXPORT namespace hat {

    const system_info_impl& get_system();
}
