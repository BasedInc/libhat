#include <libhat/defines.hpp>
#ifdef LIBHAT_MAC

#include <libhat/process.hpp>

#include <mach-o/dyld.h>
#include <mach-o/loader.h>
#include <mach/vm_prot.h>

#include <dlfcn.h>

#include <concepts>
#include <cstdlib>
#include <memory>
#include <string>

namespace hat::process {

#ifdef LIBHAT_LP64
    using mach_header_t = ::mach_header_64;
    using segment_command_t = ::segment_command_64;
    using section_t = ::section_64;
    static constexpr uint32_t mh_magic = MH_MAGIC_64;
    static constexpr uint32_t mh_cigam = MH_CIGAM_64;
    static constexpr uint32_t lc_segment = LC_SEGMENT_64;
#else
    using mach_header_t = ::mach_header;
    using segment_command_t = ::segment_command;
    using section_t = ::section;
    static constexpr uint32_t mh_magic = MH_MAGIC;
    static constexpr uint32_t mh_cigam = MH_CIGAM;
    static constexpr uint32_t lc_segment = LC_SEGMENT;
#endif

    static bool is_valid_header(const mach_header_t* header) {
        return header && (header->magic == mh_magic || header->magic == mh_cigam);
    }

    static void for_each_segment_impl(const uintptr_t address, std::predicate<uintptr_t, const segment_command_t*> auto&& callback) {
        const uint32_t imageCount = _dyld_image_count();
        for (uint32_t i = 0; i < imageCount; i++) {
            const auto* header = reinterpret_cast<const mach_header_t*>(_dyld_get_image_header(i));
            if (!is_valid_header(header)) {
                continue;
            }
            if (std::bit_cast<uintptr_t>(header) != address) {
                continue;
            }

            const auto slide = static_cast<uintptr_t>(_dyld_get_image_vmaddr_slide(i));
            const auto* cmd = reinterpret_cast<const load_command*>(
                reinterpret_cast<const std::byte*>(header) + sizeof(mach_header_t));

            for (uint32_t j = 0; j < header->ncmds; j++) {
                if (cmd->cmd == lc_segment) {
                    const auto* seg = reinterpret_cast<const segment_command_t*>(cmd);
                    if (seg->vmsize != 0 && seg->initprot != 0) {
                        if (!callback(slide, seg)) {
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

    hat::process::module get_process_module() {
        const uint32_t count = _dyld_image_count();
        for (uint32_t i = 0; i != count; i++) {
            const auto* header = reinterpret_cast<const mach_header_t*>(_dyld_get_image_header(i));
            if (!is_valid_header(header)) {
                continue;
            }

            if (header->filetype == MH_EXECUTE) {
                return hat::process::module{std::bit_cast<uintptr_t>(header)};
            }
        }
        std::abort();
    }

    std::span<std::byte> module::get_section_data(std::string_view name) const {
        std::span<std::byte> data{};
        for_each_segment_impl(this->address(), [&](uintptr_t slide, const segment_command_t* seg) {
            hat::protection prot{};
            if (seg->initprot & VM_PROT_READ)    prot |= hat::protection::Read;
            if (seg->initprot & VM_PROT_WRITE)   prot |= hat::protection::Write;
            if (seg->initprot & VM_PROT_EXECUTE) prot |= hat::protection::Execute;

            const auto* sections = reinterpret_cast<const section_t*>(
                reinterpret_cast<const std::byte*>(seg) + sizeof(segment_command_t));
            for (uint32_t i = 0; i < seg->nsects; i++) {
                auto& section = sections[i];
                if (!section.addr) {
                    continue;
                }

                const std::string_view sectionName{
                    static_cast<const char*>(section.sectname),
                    strnlen(static_cast<const char*>(section.sectname), 16)
                };
                const std::span sectionData{
                    reinterpret_cast<std::byte*>(section.addr + slide),
                    section.size
                };

                if (sectionName == name) {
                    data = sectionData;
                    return false;
                }
            }

            return true;
        });
        return data;
    }

    void module::for_each_section(const std::function<bool(std::string_view, std::span<std::byte>, hat::protection)>& callback) const {
        for_each_segment_impl(this->address(), [&](uintptr_t slide, const segment_command_t* seg) {
            hat::protection prot{};
            if (seg->initprot & VM_PROT_READ)    prot |= hat::protection::Read;
            if (seg->initprot & VM_PROT_WRITE)   prot |= hat::protection::Write;
            if (seg->initprot & VM_PROT_EXECUTE) prot |= hat::protection::Execute;

            const auto* sections = reinterpret_cast<const section_t*>(
                reinterpret_cast<const std::byte*>(seg) + sizeof(segment_command_t));
            for (uint32_t i = 0; i < seg->nsects; i++) {
                auto& section = sections[i];
                if (!section.addr) {
                    continue;
                }

                const std::string_view sectionName{
                    static_cast<const char*>(section.sectname),
                    strnlen(static_cast<const char*>(section.sectname), 16)
                };
                const std::span data{
                    reinterpret_cast<std::byte*>(section.addr + slide),
                    section.size
                };

                if (!callback(sectionName, data, prot)) {
                    return false;
                }
            }

            return true;
        });
    }

    void module::for_each_segment(const std::function<bool(std::span<std::byte>, hat::protection)>& callback) const {
        for_each_segment_impl(this->address(), [&](uintptr_t slide, const segment_command_t* seg) {
            const std::span data{
                reinterpret_cast<std::byte*>(seg->vmaddr + slide),
                seg->vmsize
            };

            hat::protection prot{};
            if (seg->initprot & VM_PROT_READ)    prot |= hat::protection::Read;
            if (seg->initprot & VM_PROT_WRITE)   prot |= hat::protection::Write;
            if (seg->initprot & VM_PROT_EXECUTE) prot |= hat::protection::Execute;

            return callback(data, prot);
        });
    }

    std::optional<hat::process::module> get_module(const std::string_view name) {
        if (name.empty()) {
            return get_process_module();
        }

        using Handle = std::unique_ptr<void, decltype([](void* handle) {
            dlclose(handle);
        })>;

        const std::string buffer{name};
        const Handle handle{dlopen(buffer.c_str(), RTLD_LAZY | RTLD_NOLOAD)};
        if (!handle) {
            return {};
        }

        const uint32_t count = _dyld_image_count();
        for (uint32_t i = 0; i < count; i++) {
            const auto* header = reinterpret_cast<const mach_header_t*>(_dyld_get_image_header(i));
            if (!is_valid_header(header)) {
                continue;
            }

            const Handle h{dlopen(_dyld_get_image_name(i), RTLD_LAZY | RTLD_NOLOAD)};
            if (h == handle) {
                return hat::process::module{std::bit_cast<uintptr_t>(header)};
            }
        }

        return {};
    }
}

#endif
