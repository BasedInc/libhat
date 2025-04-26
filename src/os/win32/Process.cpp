#include <libhat/defines.hpp>
#ifdef LIBHAT_WINDOWS

#include <libhat/process.hpp>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
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

        if (size && *size < static_cast<size_t>(dosHeader->e_lfanew) + sizeof(IMAGE_NT_HEADERS)) {
            return false;
        }

        const auto ntHeaders = reinterpret_cast<const IMAGE_NT_HEADERS*>(static_cast<const std::byte*>(mod) + dosHeader->e_lfanew);
        if (ntHeaders->Signature != IMAGE_NT_SIGNATURE) {
            return false;
        }

        return true;
    }

    static const IMAGE_NT_HEADERS& getNTHeaders(const hat::process::module mod) {
        const auto* scanBytes = reinterpret_cast<const std::byte*>(mod.address());
        const auto* dosHeader = reinterpret_cast<const IMAGE_DOS_HEADER*>(mod.address());
        return *reinterpret_cast<const IMAGE_NT_HEADERS*>(scanBytes + dosHeader->e_lfanew);
    }

    static bool regionHasFlags(const std::span<const std::byte> region, const DWORD flags) {
        for (auto* addr = region.data(); addr < region.data() + region.size();) {
            MEMORY_BASIC_INFORMATION mbi{};
            if (!VirtualQuery(addr, &mbi, sizeof(mbi))) {
                return false;
            }
            if (mbi.State != MEM_COMMIT) {
                return false;
            }
            if (!(mbi.Protect & flags)) {
                return false;
            }
            addr = static_cast<const std::byte*>(mbi.BaseAddress) + mbi.RegionSize;
        }
        return true;
    }

    bool is_readable(const std::span<const std::byte> region) {
        constexpr DWORD flags = PAGE_EXECUTE_READ
            | PAGE_EXECUTE_READWRITE
            | PAGE_EXECUTE_WRITECOPY
            | PAGE_READONLY
            | PAGE_READWRITE
            | PAGE_WRITECOPY;
        return regionHasFlags(region, flags);
    }

    bool is_writable(const std::span<const std::byte> region) {
        constexpr DWORD flags = PAGE_EXECUTE_READWRITE
            | PAGE_EXECUTE_WRITECOPY
            | PAGE_READWRITE
            | PAGE_WRITECOPY;
        return regionHasFlags(region, flags);
    }

    bool is_executable(const std::span<const std::byte> region) {
        constexpr DWORD flags = PAGE_EXECUTE
            | PAGE_EXECUTE_READ
            | PAGE_EXECUTE_READWRITE
            | PAGE_EXECUTE_WRITECOPY;
        return regionHasFlags(region, flags);
    }

    hat::process::module get_process_module() {
        return module{reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr))};
    }

    std::optional<hat::process::module> get_module(const std::string_view name) {
        const int size = MultiByteToWideChar(CP_UTF8, 0, name.data(), static_cast<int>(name.size()), nullptr, 0);
        if (!size) {
            return {};
        }

        std::wstring str;
        str.resize(static_cast<size_t>(size));

        MultiByteToWideChar(CP_UTF8, 0, name.data(), static_cast<int>(name.size()), str.data(), size);

        if (const auto handle = GetModuleHandleW(str.c_str()); handle) {
            return hat::process::module{std::bit_cast<uintptr_t>(handle)};
        }
        return {};
    }

    std::optional<hat::process::module> module_at(void* address, const std::optional<size_t> size) {
        if (isValidModule(address, size)) {
            return hat::process::module{std::bit_cast<uintptr_t>(address)};
        }
        return {};
    }

    std::span<std::byte> hat::process::module::get_module_data() const {
        auto* const scanBytes = reinterpret_cast<std::byte*>(this->address());
        const size_t sizeOfImage = getNTHeaders(*this).OptionalHeader.SizeOfImage;
        return {scanBytes, sizeOfImage};
    }

    std::span<std::byte> hat::process::module::get_section_data(const std::string_view name) const {
        auto* bytes = reinterpret_cast<std::byte*>(this->address());
        const auto& ntHeaders = getNTHeaders(*this);

        const auto* sectionHeader = IMAGE_FIRST_SECTION(&ntHeaders);
        for (WORD i = 0; i < ntHeaders.FileHeader.NumberOfSections; i++, sectionHeader++) {
            const std::string_view sectionName{
                reinterpret_cast<const char*>(sectionHeader->Name),
                strnlen_s(reinterpret_cast<const char*>(sectionHeader->Name), IMAGE_SIZEOF_SHORT_NAME)
            };
            if (sectionName == name) {
                return {
                    bytes + sectionHeader->VirtualAddress,
                    static_cast<size_t>(sectionHeader->Misc.VirtualSize)
                };
            }
        }
        return {};
    }

    void hat::process::module::for_each_segment(const std::function<bool(std::span<std::byte>, hat::protection)>& callback) const {
        const auto& ntHeaders = getNTHeaders(*this);

        const auto* sectionHeader = IMAGE_FIRST_SECTION(&ntHeaders);
        for (WORD i = 0; i < ntHeaders.FileHeader.NumberOfSections; i++, sectionHeader++) {
            const std::span data{
                reinterpret_cast<std::byte*>(this->address() + sectionHeader->VirtualAddress),
                sectionHeader->Misc.VirtualSize
            };

            hat::protection prot{};
            if (sectionHeader->Characteristics & IMAGE_SCN_MEM_READ) prot |= hat::protection::Read;
            if (sectionHeader->Characteristics & IMAGE_SCN_MEM_WRITE) prot |= hat::protection::Write;
            if (sectionHeader->Characteristics & IMAGE_SCN_MEM_EXECUTE) prot |= hat::protection::Execute;

            if (!callback(data, prot)) {
                break;
            }
        }
    }
}
#endif
