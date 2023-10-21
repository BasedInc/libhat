#include <libhat/Defines.hpp>
#ifdef LIBHAT_WINDOWS

#include <libhat/MemoryProtector.hpp>

#include <Windows.h>

namespace hat {
    static DWORD ToWinProt(protection flags) {
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
#endif
