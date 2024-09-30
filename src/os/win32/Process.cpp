#include <libhat/Defines.hpp>
#ifdef LIBHAT_WINDOWS

#include <libhat/Process.hpp>

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <bit>
#include <string>
#include <span>

namespace hat::process {

    static bool isValidModule(const void* mod, const std::optional<size_t> size) {
        if (size && *size < sizeof(IMAGE_DOS_HEADER)) {
            return false;
        }

        const auto dosHeader = static_cast<const IMAGE_DOS_HEADER*>(mod);
        if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE) {
            return false;
        }

        if (size && *size < dosHeader->e_lfanew + sizeof(IMAGE_NT_HEADERS)) {
            return false;
        }

        const auto ntHeaders = reinterpret_cast<const IMAGE_NT_HEADERS*>(static_cast<const std::byte*>(mod) + dosHeader->e_lfanew);
        if (ntHeaders->Signature != IMAGE_NT_SIGNATURE) {
            return false;
        }

        return true;
    }

    static const IMAGE_NT_HEADERS& getNTHeaders(const module_t mod) {
        const auto* scanBytes = reinterpret_cast<const std::byte*>(mod);
        const auto* dosHeader = reinterpret_cast<const IMAGE_DOS_HEADER*>(mod);
        return *reinterpret_cast<const IMAGE_NT_HEADERS*>(scanBytes + dosHeader->e_lfanew);
    }

    module_t get_process_module() {
        return module_t{reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr))};
    }

    std::optional<module_t> get_module(const std::string_view name) {
        const int size = MultiByteToWideChar(CP_UTF8, 0, name.data(), static_cast<int>(name.size()), nullptr, 0);
        if (!size) {
            return {};
        }

        std::wstring str;
        str.resize(size);

        MultiByteToWideChar(CP_UTF8, 0, name.data(), static_cast<int>(name.size()), str.data(), size);

        if (const auto module = GetModuleHandleW(str.c_str()); module) {
            return module_t{std::bit_cast<uintptr_t>(module)};
        }
        return {};
    }

    std::optional<module_t> module_at(void* address, const std::optional<size_t> size) {
        if (isValidModule(address, size)) {
            return module_t{std::bit_cast<uintptr_t>(address)};
        }
        return {};
    }

    std::span<std::byte> get_module_data(const module_t mod) {
        auto* const scanBytes = reinterpret_cast<std::byte*>(mod);
        const size_t sizeOfImage = getNTHeaders(mod).OptionalHeader.SizeOfImage;
        return {scanBytes, sizeOfImage};
    }

    std::span<std::byte> get_section_data(const module_t mod, const std::string_view name) {
        auto* bytes = reinterpret_cast<std::byte*>(mod);
        const auto& ntHeaders = getNTHeaders(mod);

        const auto maxChars = std::min<size_t>(name.size(), 8);

        const auto* sectionHeader = IMAGE_FIRST_SECTION(&ntHeaders);
        for (DWORD i = 0; i < ntHeaders.OptionalHeader.NumberOfRvaAndSizes; i++, sectionHeader++) {
            if (strncmp(name.data(), reinterpret_cast<const char*>(sectionHeader->Name), maxChars) == 0) {
                return {
                    bytes + sectionHeader->VirtualAddress,
                    static_cast<size_t>(sectionHeader->Misc.VirtualSize)
                };
            }
        }
        return {};
    }
}
#endif
