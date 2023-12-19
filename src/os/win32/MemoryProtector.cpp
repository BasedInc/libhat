#include <libhat/Defines.hpp>
#ifdef LIBHAT_WINDOWS

#include <libhat/MemoryProtector.hpp>

#include <Windows.h>

namespace hat {
    static DWORD ToWinProt(const protection flags) {
        const bool r = static_cast<bool>(flags & protection::Read);
        const bool w = static_cast<bool>(flags & protection::Write);
        const bool x = static_cast<bool>(flags & protection::Execute);

        if (x && w) return PAGE_EXECUTE_READWRITE;
        if (x && r) return PAGE_EXECUTE_READ;
        if (x)      return PAGE_EXECUTE;
        if (w)      return PAGE_READWRITE;
        if (r)      return PAGE_READONLY;
        return PAGE_NOACCESS;
    }

    memory_protector::memory_protector(const uintptr_t address, const size_t size, const protection flags) : address(address), size(size) {
        VirtualProtect(reinterpret_cast<LPVOID>(this->address), this->size, ToWinProt(flags), reinterpret_cast<PDWORD>(&this->oldProtection));
    }

    memory_protector::~memory_protector() {
        DWORD temp;
        VirtualProtect(reinterpret_cast<LPVOID>(this->address), this->size, this->oldProtection, &temp);
    }
}
#endif
