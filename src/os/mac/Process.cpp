#include <libhat/defines.hpp>
#ifdef LIBHAT_MAC

#include <libhat/process.hpp>

#include <mach-o/dyld.h>
#include <mach-o/loader.h>
#include <mach/vm_prot.h>

#include <dlfcn.h>

#include <memory>

namespace hat::process {

    // no-op on 32 bit binaries
    void module::for_each_segment(const std::function<bool(std::span<std::byte>, hat::protection)>& callback) const {
        const uint32_t imageCount = _dyld_image_count();
        for (uint32_t i = 0; i < imageCount; i++) {
            const auto* header = reinterpret_cast<const mach_header_64*>(_dyld_get_image_header(i));
            if (header == nullptr) {
                continue;
            }
            if (std::bit_cast<uintptr_t>(header) != this->address()) {
                continue;
            }

            const auto slide = static_cast<uintptr_t>(_dyld_get_image_vmaddr_slide(i));
            const auto* cmd = reinterpret_cast<const load_command*>(
                reinterpret_cast<const std::byte*>(header) + sizeof(mach_header_64));

            for (uint32_t j = 0; j < header->ncmds; j++) {
                if (cmd->cmd == LC_SEGMENT_64) {
                    const auto* seg = reinterpret_cast<const segment_command_64*>(cmd);

                    // skip __PAGEZERO and any unmapped segment
                    if (seg->vmsize != 0 && seg->initprot != 0) {
                        const std::span data{
                            reinterpret_cast<std::byte*>(seg->vmaddr + slide),
                            seg->vmsize
                        };

                        hat::protection prot{};
                        if (seg->initprot & VM_PROT_READ)    prot |= hat::protection::Read;
                        if (seg->initprot & VM_PROT_WRITE)   prot |= hat::protection::Write;
                        if (seg->initprot & VM_PROT_EXECUTE) prot |= hat::protection::Execute;

                        if (!callback(data, prot)) {
                            return;
                        }
                    }
                }
                cmd = reinterpret_cast<const load_command*>(
                    reinterpret_cast<const std::byte*>(cmd) + cmd->cmdsize);
            }
            return;
        }
    }

    std::optional<hat::process::module> get_module(const std::string_view name) {
        using Handle = std::unique_ptr<void, decltype([](void* handle) {
            dlclose(handle);
        })>;

        std::unique_ptr<char[]> buffer;

        if (!name.empty()) {
            buffer = std::make_unique<char[]>(name.size() + 1);
            std::ranges::copy(name, buffer.get());
        }

        const Handle handle{dlopen(buffer.get(), RTLD_LAZY | RTLD_NOLOAD)};
        if (!handle) {
            return {};
        }
        
        std::optional<hat::process::module> module{};

        uint32_t imageCount = _dyld_image_count();
        for (uint32_t i = 0; i < imageCount; i++) {
            const auto* header = reinterpret_cast<const mach_header_64*>(_dyld_get_image_header(i));
            if (header == nullptr)
                continue;

            const Handle h{dlopen(_dyld_get_image_name(i), RTLD_LAZY | RTLD_NOLOAD)};
            if (h == handle) {
                module = hat::process::module{std::bit_cast<uintptr_t>(header)};
            }
        }

        return module;
    }
}

#endif
