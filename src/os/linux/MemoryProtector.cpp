#include <libhat/defines.hpp>
#ifdef LIBHAT_LINUX

#include <libhat/memory_protector.hpp>
#include <libhat/system.hpp>
#include "../../Utils.hpp"

#include "Common.hpp"

#include <sys/mman.h>

namespace hat {

    static std::optional<uint32_t> get_page_prot(const uintptr_t address) {
        std::optional<uint32_t> result;
        iter_mapped_regions([&](const uintptr_t begin, const uintptr_t end, uint32_t prot) {
            if (address >= begin && address < end) {
                result = prot;
                return false;
            }
            return true;
        });
        return result;
    }

    static int to_system_prot(const protection flags) {
        int prot = 0;
        if (static_cast<bool>(flags & protection::Read)) prot |= PROT_READ;
        if (static_cast<bool>(flags & protection::Write)) prot |= PROT_WRITE;
        if (static_cast<bool>(flags & protection::Execute)) prot |= PROT_EXEC;
        return prot;
    }

    memory_protector::memory_protector(const uintptr_t address, const size_t size, const protection flags) : address(address), size(size) {
        const auto pageSize = hat::get_system().page_size;

        const auto oldProt = get_page_prot(address);
        if (!oldProt) {
            return; // Failure indicated via is_set()
        }

        this->oldProtection = *oldProt;
        this->set = 0 == mprotect(
            reinterpret_cast<void*>(detail::fast_align_down(address, pageSize)),
            static_cast<size_t>(detail::fast_align_up(size, pageSize)),
            to_system_prot(flags)
        );
    }

    void memory_protector::restore() {
        const auto pageSize = hat::get_system().page_size;
        mprotect(
            reinterpret_cast<void*>(detail::fast_align_down(address, pageSize)),
            static_cast<size_t>(detail::fast_align_up(size, pageSize)),
            static_cast<int32_t>(this->oldProtection)
        );
    }
}
#endif
