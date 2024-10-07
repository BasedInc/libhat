#include <libhat/Defines.hpp>
#ifdef LIBHAT_LINUX

#include <charconv>
#include <fstream>
#include <optional>
#include <string>

#include <libhat/MemoryProtector.hpp>
#include <libhat/System.hpp>
#include "../../Utils.hpp"

#include <sys/mman.h>

namespace hat {

    static int to_system_prot(const protection flags) {
        int prot = 0;
        if (static_cast<bool>(flags & protection::Read)) prot |= PROT_READ;
        if (static_cast<bool>(flags & protection::Write)) prot |= PROT_WRITE;
        if (static_cast<bool>(flags & protection::Execute)) prot |= PROT_EXEC;
        return prot;
    }

    static std::optional<int> get_page_prot(const uintptr_t address) {
        std::ifstream f("/proc/self/maps");
        std::string s;
        while (std::getline(f, s)) {
            const char* it = s.data();
            const char* end = s.data() + s.size();
            std::from_chars_result res{};

            uintptr_t pageBegin;
            res = std::from_chars(it, end, pageBegin, 16);
            if (res.ec != std::errc{} || res.ptr == end) {
                continue;
            }
            it = res.ptr + 1; // +1 to skip the hyphen

            uintptr_t pageEnd;
            res = std::from_chars(it, end, pageEnd, 16);
            if (res.ec != std::errc{} || res.ptr == end) {
                continue;
            }
            it = res.ptr + 1; // +1 to skip the space

            std::string_view remaining{it, end};
            if (address >= pageBegin && address < pageEnd && remaining.size() >= 3) {
                int prot = 0;
                if (remaining[0] == 'r') prot |= PROT_READ;
                if (remaining[1] == 'w') prot |= PROT_WRITE;
                if (remaining[2] == 'x') prot |= PROT_EXEC;
                return prot;
            }
        }
        return std::nullopt;
    }

    memory_protector::memory_protector(const uintptr_t address, const size_t size, const protection flags) : address(address), size(size) {
        const auto pageSize = hat::get_system().page_size;

        const auto oldProt = get_page_prot(address);
        if (!oldProt) {
            return; // Failure indicated via is_set()
        }

        this->oldProtection = static_cast<uint32_t>(*oldProt);
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
            this->oldProtection
        );
    }
}
#endif
