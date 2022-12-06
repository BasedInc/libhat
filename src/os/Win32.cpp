#include <libhat/Defines.hpp>
#ifdef LIBHAT_WINDOWS

#include <libhat/MemoryProtector.hpp>
#include <libhat/Process.hpp>

#include <Windows.h>
#include <string>
#include <span>

namespace hat {
    DWORD ToWinProt(protection flags) {
        const bool r = bool(flags & protection::Read);
        const bool w = bool(flags & protection::Write);
        const bool x = bool(flags & protection::Execute);

        if (x && w) return PAGE_EXECUTE_READWRITE;
        if (x && r) return PAGE_EXECUTE_READ;
        if (x)      return PAGE_EXECUTE;
        if (w)      return PAGE_READWRITE;
        if (r)      return PAGE_READONLY;
        return PAGE_NOACCESS;
    }

    memory_protector::memory_protector(uintptr_t address, size_t size, protection flags) : address(address), size(size) {
        VirtualProtect(reinterpret_cast<LPVOID>(this->address), this->size, ToWinProt(flags), reinterpret_cast<PDWORD>(&this->oldProtection));
    }

    memory_protector::~memory_protector() {
        DWORD temp;
        VirtualProtect(reinterpret_cast<LPVOID>(this->address), this->size, this->oldProtection, &temp);
    }
}

namespace hat::process {

    namespace {
        PIMAGE_NT_HEADERS GetNTHeaders(module_t module) {
            auto* const scanBytes = reinterpret_cast<std::byte*>(module);
            auto* const dosHeader = reinterpret_cast<PIMAGE_DOS_HEADER>(module);
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

    std::span<std::byte> get_module_data(module_t module) {
        auto* const scanBytes = reinterpret_cast<std::byte*>(module);
        auto* const ntHeaders = GetNTHeaders(module);
        if (!ntHeaders)
            return {};

        const size_t sizeOfImage = ntHeaders->OptionalHeader.SizeOfImage;
        return {scanBytes, sizeOfImage};
    }

    std::span<std::byte> get_section_data(module_t module, std::string_view name) {
        auto* const scanBytes = reinterpret_cast<std::byte*>(module);
        auto* const ntHeaders = GetNTHeaders(module);
        if (!ntHeaders)
            return {};

        const auto* sectionHeader = IMAGE_FIRST_SECTION(ntHeaders);
        for (int i = 0; i < ntHeaders->FileHeader.NumberOfSections; i++, sectionHeader++) {
            if (strncmp(name.data(), reinterpret_cast<const char*>(sectionHeader->Name), 8) == 0) {
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
