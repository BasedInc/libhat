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

    static std::shared_ptr<void> create_impl(const HMODULE module, const bool owning) {
        // CRUCIAL performance optimization to reduce indirection (vs. having an implementation struct)
        if (owning) {
            return std::shared_ptr<void>{
                module,
                [](const HMODULE p) {
                    if (p) {
                        FreeLibrary(p);
                    }
                }
            };
        } else {
            return std::shared_ptr<void>{module, [](const HMODULE) {}};
        }
    }

    static const IMAGE_NT_HEADERS& getNTHeaders(const hat::process::module& mod) {
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
        return module{create_impl(GetModuleHandleW(nullptr), false)};
    }

    std::optional<hat::process::module> get_module(const std::string_view name) {
        std::wstring buffer;

        if (!name.empty()) {
            const int size = MultiByteToWideChar(CP_UTF8, 0, name.data(), static_cast<int>(name.size()), nullptr, 0);
            if (!size) {
                return {};
            }

            buffer.resize(static_cast<size_t>(size));

            MultiByteToWideChar(CP_UTF8, 0, name.data(), static_cast<int>(name.size()), buffer.data(), size);
        }

        const wchar_t* file = buffer.empty() ? nullptr : buffer.c_str();
        HMODULE out{};
        if (GetModuleHandleExW(0, file, &out)) {
            return hat::process::module{create_impl(out, true)};
        }
        return {};
    }

    std::optional<hat::process::module> module_at(void* address) {
        HMODULE out{};
        const auto status = GetModuleHandleExW(
            GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
            reinterpret_cast<LPCWSTR>(address),
            &out
        );
        if (status) {
            return hat::process::module{create_impl(out, true)};
        }
        return {};
    }

    uintptr_t module::address() const {
        return std::bit_cast<uintptr_t>(this->impl.get());
    }

    std::span<std::byte> hat::process::module::get_module_data() const {
        auto* const scanBytes = reinterpret_cast<std::byte*>(this->address());
        const size_t sizeOfImage = getNTHeaders(*this).OptionalHeader.SizeOfImage;
        return {scanBytes, sizeOfImage};
    }

    std::span<std::byte> module::get_executable_data() const {
        if (const auto text = this->get_section_data(".text"); !text.empty()) {
            return text;
        }

        const auto& ntHeaders = getNTHeaders(*this);

        const auto* section = IMAGE_FIRST_SECTION(&ntHeaders);
        for (WORD i = 0; i < ntHeaders.FileHeader.NumberOfSections; i++, section++) {
            const auto c = section->Characteristics;
            if ((c & IMAGE_SCN_MEM_READ) && !(c & IMAGE_SCN_MEM_WRITE) && (c & IMAGE_SCN_MEM_EXECUTE)) {
                return {
                    reinterpret_cast<std::byte*>(this->address() + section->VirtualAddress),
                    section->Misc.VirtualSize
                };
            }
        }

        return {};
    }

    std::span<std::byte> module::get_section_data(const std::string_view name) const {
        const auto& ntHeaders = getNTHeaders(*this);

        const auto* sectionHeader = IMAGE_FIRST_SECTION(&ntHeaders);
        for (WORD i = 0; i < ntHeaders.FileHeader.NumberOfSections; i++, sectionHeader++) {
            const std::string_view sectionName{
                reinterpret_cast<const char*>(sectionHeader->Name),
                strnlen_s(reinterpret_cast<const char*>(sectionHeader->Name), IMAGE_SIZEOF_SHORT_NAME)
            };
            if (sectionName == name) {
                return {
                    reinterpret_cast<std::byte*>(this->address() + sectionHeader->VirtualAddress),
                    static_cast<size_t>(sectionHeader->Misc.VirtualSize)
                };
            }
        }
        return {};
    }

    void module::for_each_section(const std::function<bool(std::string_view, std::span<std::byte>, hat::protection)>& callback) const {
        const auto& ntHeaders = getNTHeaders(*this);

        const auto* sectionHeader = IMAGE_FIRST_SECTION(&ntHeaders);
        for (WORD i = 0; i < ntHeaders.FileHeader.NumberOfSections; i++, sectionHeader++) {
            const std::string_view name{
                reinterpret_cast<const char*>(sectionHeader->Name),
                strnlen_s(reinterpret_cast<const char*>(sectionHeader->Name), IMAGE_SIZEOF_SHORT_NAME)
            };
            const std::span data{
                reinterpret_cast<std::byte*>(this->address() + sectionHeader->VirtualAddress),
                sectionHeader->Misc.VirtualSize
            };

            hat::protection prot{};
            if (sectionHeader->Characteristics & IMAGE_SCN_MEM_READ) prot |= hat::protection::Read;
            if (sectionHeader->Characteristics & IMAGE_SCN_MEM_WRITE) prot |= hat::protection::Write;
            if (sectionHeader->Characteristics & IMAGE_SCN_MEM_EXECUTE) prot |= hat::protection::Execute;

            if (!callback(name, data, prot)) {
                break;
            }
        }
    }

    void module::for_each_segment(const std::function<bool(std::span<std::byte>, hat::protection)>& callback) const {
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
