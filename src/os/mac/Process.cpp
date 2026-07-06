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

    static hat::protection to_hat_prot(const vm_prot_t prot) {
        hat::protection ret{};
        if (prot & VM_PROT_READ)    ret |= hat::protection::Read;
        if (prot & VM_PROT_WRITE)   ret |= hat::protection::Write;
        if (prot & VM_PROT_EXECUTE) ret |= hat::protection::Execute;
        return ret;
    }

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

    static void for_each_section_impl(const uintptr_t address, std::predicate<uintptr_t, const segment_command_t*, const section_t*> auto&& callback) {
        for_each_segment_impl(address, [&](uintptr_t slide, const segment_command_t* seg) {
            const auto* sections = reinterpret_cast<const section_t*>(
                reinterpret_cast<const std::byte*>(seg) + sizeof(segment_command_t));
            for (uint32_t i = 0; i < seg->nsects; i++) {
                auto* sec = &sections[i];
                if (sec->addr) {
                    if (!callback(slide, seg, sec)) {
                        return false;
                    }
                }
            }
            return true;
        });
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

    std::span<std::byte> module::get_module_data() const {
        size_t max{};
        for_each_segment_impl(this->address(), [&](uintptr_t slide, const segment_command_t* seg) {
            max = std::max(max, static_cast<size_t>(seg->vmaddr + slide - this->address() + seg->vmsize));
            return true;
        });
        return {reinterpret_cast<std::byte*>(this->address()), max};
    }

    std::span<std::byte> module::get_executable_data() const {
        if (const auto text = this->get_section_data("__TEXT,__text"); !text.empty()) {
            return text;
        }

        std::span<std::byte> data{};
        for_each_section_impl(this->address(), [&](uintptr_t slide, const segment_command_t* seg, const section_t* sec) {
            if ((seg->initprot & VM_PROT_READ) && !(seg->initprot & VM_PROT_WRITE) && (seg->initprot & VM_PROT_EXECUTE)) {
                if (sec->flags & S_ATTR_PURE_INSTRUCTIONS) {
                    data = {
                        reinterpret_cast<std::byte*>(sec->addr + slide),
                        sec->size
                    };
                    return false;
                }
            }
            return true;
        });
        return {};
    }

    std::span<std::byte> module::get_section_data(std::string_view name) const {
        std::span<std::byte> data{};
        for_each_section_impl(this->address(), [&](uintptr_t slide, const segment_command_t* seg, const section_t* sec) {
            const std::string_view segmentName{
                static_cast<const char*>(seg->segname),
                strnlen(static_cast<const char*>(seg->segname), 16)
            };
            const std::string_view sectionName{
                static_cast<const char*>(sec->sectname),
                strnlen(static_cast<const char*>(sec->sectname), 16)
            };
            const std::span sectionData{
                reinterpret_cast<std::byte*>(sec->addr + slide),
                sec->size
            };

            // Check for "SEGMENT,SECTION" first
            const bool qualified = name.size() == segmentName.size() + sectionName.size() + 1
                && name.starts_with(segmentName)
                && name[segmentName.size()] == ','
                && name.ends_with(sectionName);

            if (qualified || sectionName == name) {
                data = sectionData;
                return false;
            }
            return true;
        });
        return data;
    }

    void module::for_each_section(const std::function<bool(std::string_view, std::span<std::byte>, hat::protection)>& callback) const {
        for_each_section_impl(this->address(), [&](uintptr_t slide, const segment_command_t* seg, const section_t* sec) {
            const std::string_view segmentName{
                static_cast<const char*>(seg->segname),
                strnlen(static_cast<const char*>(seg->segname), 16)
            };
            const std::string_view sectionName{
                static_cast<const char*>(sec->sectname),
                strnlen(static_cast<const char*>(sec->sectname), 16)
            };
            const std::span data{
                reinterpret_cast<std::byte*>(sec->addr + slide),
                sec->size
            };
            std::string qualified;
            qualified.reserve(segmentName.size() + sectionName.size() + 1);
            qualified += segmentName;
            qualified += ',';
            qualified += sectionName;
            return callback(qualified, data, to_hat_prot(seg->initprot));
        });
    }

    void module::for_each_segment(const std::function<bool(std::span<std::byte>, hat::protection)>& callback) const {
        for_each_segment_impl(this->address(), [&](uintptr_t slide, const segment_command_t* seg) {
            const std::span data{
                reinterpret_cast<std::byte*>(seg->vmaddr + slide),
                seg->vmsize
            };
            return callback(data, to_hat_prot(seg->initprot));
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
