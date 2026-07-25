#include <libhat/defines.hpp>
#if defined(LIBHAT_X86) || defined(LIBHAT_X86_64)

#include <libhat/system.hpp>

#include <array>
#include <bit>
#include <bitset>
#include <cstdint>
#include <vector>
#include <cstring>

#include <immintrin.h>

namespace {
    struct info_t {
        int eax;
        int ebx;
        int ecx;
        int edx;
    };
}

#ifdef LIBHAT_WINDOWS
    #include <intrin.h>

    static info_t cpuid_impl(const int id) {
        std::array<int, 4> info{};
        __cpuid(info.data(), id);
        return std::bit_cast<info_t>(info);
    }
    static info_t cpuidex_impl(const int id, const int subId) {
        std::array<int, 4> info{};
        __cpuidex(info.data(), id, subId);
        return std::bit_cast<info_t>(info);
    }
#endif

#ifdef LIBHAT_UNIX
    #include <cpuid.h>

    static info_t cpuid_impl(const int id) {
        info_t info{};
        __cpuid(id, info.eax, info.ebx, info.ecx, info.edx);
        return info;
    }
    static info_t cpuidex_impl(const int id, const int subId) {
        info_t info{};
        __cpuid_count(id, subId, info.eax, info.ebx, info.ecx, info.edx);
        return info;
    }
#endif

LIBHAT_TARGET("xsave")
static auto xgetbv_impl(const unsigned int a) {
    auto value = _xgetbv(a);
    return static_cast<std::make_unsigned_t<decltype(value)>>(value);
}

#ifndef _XCR_XFEATURE_ENABLED_MASK
    #define _XCR_XFEATURE_ENABLED_MASK 0
#endif

namespace hat {

    static constexpr int CPU_BASIC_INFO = 0;
    static constexpr int CPU_EXTENDED_INFO = static_cast<int>(0x80000000);
    static constexpr int CPU_BRAND_STRING = static_cast<int>(0x80000004);

    system_info_x86::system_info_x86() {
        // Gather basic info
        std::array<info_t, 8> data{};
        const auto info = ::cpuid_impl(CPU_BASIC_INFO);
        const int nIds = std::min(info.eax, static_cast<int>(data.size()) - 1);
        for (int i = CPU_BASIC_INFO; i <= nIds; i++) {
            data[static_cast<size_t>(i)] = ::cpuidex_impl(i, 0);
        }

        char vendor[0xC + 1]{};
        std::memcpy(vendor + sizeof(int) * 0, &info.ebx, sizeof(int));
        std::memcpy(vendor + sizeof(int) * 1, &info.edx, sizeof(int));
        std::memcpy(vendor + sizeof(int) * 2, &info.ecx, sizeof(int));

        // Read relevant info
        const std::bitset<32> f_1_ECX_{static_cast<std::uint32_t>(data[1].ecx)};
        const std::bitset<32> f_1_EDX_{static_cast<std::uint32_t>(data[1].edx)};
        const std::bitset<32> f_7_EBX_{static_cast<std::uint32_t>(data[7].ebx)};

        // Gather extended info
        std::array<info_t, 5> extData{};
        const auto extInfo = ::cpuid_impl(CPU_EXTENDED_INFO);
        const int nExtIds = std::min(extInfo.eax, static_cast<int>(extData.size()) + CPU_EXTENDED_INFO - 1);
        for (int i = CPU_EXTENDED_INFO; i <= nExtIds; i++) {
            extData[static_cast<size_t>(i - CPU_EXTENDED_INFO)] = ::cpuidex_impl(i, 0);
        }

        // Read extended info
        char brand[0x40 + 1]{};
        if (nExtIds >= CPU_BRAND_STRING) {
            std::memcpy(brand + sizeof(info_t) * 0, &extData[2], sizeof(info_t));
            std::memcpy(brand + sizeof(info_t) * 1, &extData[3], sizeof(info_t));
            std::memcpy(brand + sizeof(info_t) * 2, &extData[4], sizeof(info_t));
        }

        // Check OS capabilities
        bool avxsupport = false;
        bool avx512support = false;
        const bool xsave = f_1_ECX_[26];
        const bool osxsave = f_1_ECX_[27];
        if (xsave && osxsave) {
            // https://cdrdv2-public.intel.com/671190/253668-sdm-vol-3a.pdf (Page 2-20)
            const std::bitset<64> xcr = xgetbv_impl(_XCR_XFEATURE_ENABLED_MASK);
            avxsupport = xcr[1] && xcr[2]; // xmm and ymm
            avx512support = avxsupport && xcr[5] && xcr[6] && xcr[7]; // opmask and zmm
        }

        this->cpu_vendor = vendor;
        this->cpu_brand = brand;
        this->extensions = {
            .sse      = f_1_EDX_[25],
            .sse2     = f_1_EDX_[26],
            .sse3     = f_1_ECX_[0],
            .ssse3    = f_1_ECX_[9],
            .sse41    = f_1_ECX_[19],
            .sse42    = f_1_ECX_[20],
            .avx      = f_1_ECX_[28] && avxsupport,
            .avx2     = f_7_EBX_[5] && avxsupport,
            .avx512f  = f_7_EBX_[16] && avx512support,
            .avx512bw = f_7_EBX_[30] && avx512support,
            .popcnt   = f_1_ECX_[23],
            .bmi      = f_7_EBX_[3],
        };
    }
}
#endif
