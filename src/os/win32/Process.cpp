#include <libhat/Defines.hpp>
#ifdef LIBHAT_WINDOWS

#include <libhat/Process.hpp>

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN

#include <Windows.h>
#include <string>
#include <span>

namespace hat::process {

    namespace {
        PIMAGE_NT_HEADERS GetNTHeaders(module_t mod) {
            auto* const scanBytes = reinterpret_cast<std::byte*>(mod);
            auto* const dosHeader = reinterpret_cast<PIMAGE_DOS_HEADER>(mod);
            if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE)
                return nullptr;

            auto* const ntHeaders = reinterpret_cast<PIMAGE_NT_HEADERS>(scanBytes + dosHeader->e_lfanew);
            if (ntHeaders->Signature != IMAGE_NT_SIGNATURE)
                return nullptr;

            return ntHeaders;
        }
    }

    module_t get_process_module() {
        return module_t{reinterpret_cast<uintptr_t>(GetModuleHandleA(nullptr))};
    }

    module_t get_module(const std::string& name) {
        return module_at(reinterpret_cast<uintptr_t>(GetModuleHandleA(name.c_str())));
    }

    std::span<std::byte> get_module_data(module_t mod) {
        auto* const scanBytes = reinterpret_cast<std::byte*>(mod);
        auto* const ntHeaders = GetNTHeaders(mod);
        if (!ntHeaders)
            return {};

        const size_t sizeOfImage = ntHeaders->OptionalHeader.SizeOfImage;
        return {scanBytes, sizeOfImage};
    }

    std::span<std::byte> get_section_data(module_t mod, std::string_view name) {
        auto* const scanBytes = reinterpret_cast<std::byte*>(mod);
        auto* const ntHeaders = GetNTHeaders(mod);
        if (!ntHeaders)
            return {};

        const auto maxChars = std::min<size_t>(name.size(), 8);

        const auto* sectionHeader = IMAGE_FIRST_SECTION(ntHeaders);
        for (int i = 0; i < ntHeaders->FileHeader.NumberOfSections; i++, sectionHeader++) {
            if (strncmp(name.data(), reinterpret_cast<const char*>(sectionHeader->Name), maxChars) == 0) {
                return {
                    scanBytes + sectionHeader->VirtualAddress,
                    static_cast<size_t>(sectionHeader->Misc.VirtualSize)
                };
            }
        }
        return {};
    }
}
#endif
