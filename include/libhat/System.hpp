#pragma once

#include <libhat/Defines.hpp>
#include <string>

namespace hat {
    struct system_info {
        size_t page_size{};

        system_info(const system_info&) = delete;
        system_info& operator=(const system_info&) = delete;
        system_info(system_info&&) = delete;
        system_info& operator=(system_info&&) = delete;
    protected:
        system_info();
    };
}

#if defined(LIBHAT_X86) || defined(LIBHAT_X86_64)
namespace hat {

    struct system_info_x86 : hat::system_info {
        std::string cpu_vendor{};
        std::string cpu_brand{};
        struct {
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
        } extensions{};
    private:
        system_info_x86();
        friend const system_info_x86& get_system();
        static const system_info_x86 instance;
    };

    using system_info_impl = system_info_x86;
}
#elif defined(LIBHAT_ARM) || defined(LIBHAT_AARCH64)
namespace hat {

    struct system_info_arm : hat::system_info {
    private:
        system_info_arm() = default;
        friend const system_info_arm& get_system();
        static const system_info_arm instance;
    };

    using system_info_impl = system_info_arm;
}
#endif

namespace hat {

    const system_info_impl& get_system();
}
