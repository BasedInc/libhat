#include <libhat/Defines.hpp>
#ifdef LIBHAT_WINDOWS

#include <libhat/MemoryProtector.hpp>
#include <libhat/Process.hpp>
#include <libhat/Scanner.hpp>

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

    template<>
    scan_result find_vtable<compiler_type::MSVC>(const std::string& className, hat::process::module_t mod) {
        // Tracing cross-references
        // Type Descriptor => Object Locator => VTable
        auto sig = string_to_signature(".?AV" + className + "@@");

        // TODO: Have a better solution for this
        // 3rd character may be 'V' for classes and 'U' for structs
        sig[3] = {};

        auto typeDesc = *find_pattern(sig, ".data", mod);
        if (!typeDesc) {
            return nullptr;
        }
        // 0x10 is the offset from the type descriptor name to the type descriptor header
        typeDesc -= 2 * sizeof(void*);

        // The actual xref refers to an offset from the base module
        const auto loffset = static_cast<uint32_t>(typeDesc - reinterpret_cast<std::byte*>(mod));
        auto locator = object_to_signature(loffset);
        // FIXME: These appear to be the values just for basic classes with single inheritance. We should be using a
        //        different method to differentiate the object locator from the base class descriptor.
        #ifdef LIBHAT_X86_64
        locator.insert(locator.begin(), {
            std::byte{0x01}, std::byte{0x00}, std::byte{0x00}, std::byte{0x00}, // signature
            std::byte{0x00}, std::byte{0x00}, std::byte{0x00}, std::byte{0x00}, // offset
            std::byte{0x00}, std::byte{0x00}, std::byte{0x00}, std::byte{0x00}  // constructor displacement offset
        });
        #else
        locator.insert(locator.begin(), {
            std::byte{0x00}, std::byte{0x00}, std::byte{0x00}, std::byte{0x00}, // signature
            std::byte{0x00}, std::byte{0x00}, std::byte{0x00}, std::byte{0x00}, // offset
            std::byte{0x00}, std::byte{0x00}, std::byte{0x00}, std::byte{0x00}  // constructor displacement offset
        });
        #endif
        const auto objectLocator = *find_pattern(locator, ".rdata", mod);
        if (!objectLocator) {
            return nullptr;
        }

        const auto vtable = *find_pattern(object_to_signature(objectLocator), ".data", mod);
        return vtable ? vtable + sizeof(void*) : nullptr;
    }

    template<>
    scan_result find_vtable<compiler_type::GNU>(const std::string& className, hat::process::module_t mod) {
        // Tracing cross-references
        // Type Descriptor Name => Type Info => VTable
        const auto sig = string_to_signature(std::to_string(className.size()) + className + "\0");
        const auto typeName = *find_pattern(sig, ".rdata", mod);
        if (!typeName) {
            return nullptr;
        }
        auto typeInfo = *find_pattern(object_to_signature(typeName), ".rdata", mod);
        if (!typeInfo) {
            return nullptr;
        }
        // A single pointer is the offset from the type name pointer to the start of the type info
        typeInfo -= sizeof(void*);

        const auto vtable = *find_pattern(object_to_signature(typeInfo), ".rdata", mod);
        return vtable ? vtable + sizeof(void*) : nullptr;
    }
}

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
