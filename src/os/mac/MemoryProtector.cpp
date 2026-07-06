#include <libhat/defines.hpp>
#if defined(LIBHAT_MAC) && defined(LIBHAT_LP64)

#include <libhat/memory_protector.hpp>

#include <mach/mach.h>
#include <mach/mach_vm.h>

#include <optional>

namespace hat {

    static std::optional<uint32_t> get_page_prot(const uintptr_t address) {
        mach_vm_address_t addr = static_cast<mach_vm_address_t>(address);
        mach_vm_size_t size{};
        vm_region_basic_info_64 info{};
        mach_msg_type_number_t infoCount = VM_REGION_BASIC_INFO_COUNT_64;
        mach_port_t objectName{};

        const auto result = mach_vm_region(
            mach_task_self(),
            &addr,
            &size,
            VM_REGION_BASIC_INFO_64,
            reinterpret_cast<vm_region_info_t>(&info),
            &infoCount,
            &objectName
        );

        if (result != KERN_SUCCESS) {
            return std::nullopt;
        }

        return static_cast<uint32_t>(info.protection);
    }

    static vm_prot_t to_system_prot(const protection flags) {
        vm_prot_t prot = 0;
        if (static_cast<bool>(flags & protection::Read)) prot |= VM_PROT_READ;
        if (static_cast<bool>(flags & protection::Write)) prot |= VM_PROT_WRITE;
        if (static_cast<bool>(flags & protection::Execute)) prot |= VM_PROT_EXECUTE;
        return prot;
    }

    memory_protector::memory_protector(const uintptr_t address, const size_t size, const protection flags) : address(address), size(size) {
        const auto oldProt = get_page_prot(address);
        if (!oldProt) {
            return; // Failure indicated via is_set()
        }

        this->oldProtection = *oldProt;
        this->set = KERN_SUCCESS == mach_vm_protect(
            mach_task_self(),
            static_cast<mach_vm_address_t>(this->address),
            static_cast<mach_vm_size_t>(this->size),
            false,
            to_system_prot(flags)
        );
    }

    void memory_protector::restore() {
        mach_vm_protect(
            mach_task_self(),
            static_cast<mach_vm_address_t>(this->address),
            static_cast<mach_vm_size_t>(this->size),
            false,
            static_cast<vm_prot_t>(this->oldProtection)
        );
    }
}
#endif
