#pragma once

#include <string>

namespace hat {

    typedef struct system_info_x86 {
        std::string cpu_vendor;
        std::string cpu_brand;
        struct {
            bool sse;
            bool sse2;
            bool sse3;
            bool ssse3;
            bool sse41;
            bool sse42;
            bool avx;
            bool avx2;
            bool avx512;
            bool popcnt;
            bool bmi1;
        } extensions;

        system_info_x86(const system_info_x86&) = delete;
        system_info_x86& operator=(const system_info_x86&) = delete;
    private:
        system_info_x86();
        friend const system_info_x86& get_system();
        static const system_info_x86 instance;
    } system_info;
}
