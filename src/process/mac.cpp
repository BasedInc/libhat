#include <libhat/defines.hpp>
#ifdef LIBHAT_MAC

#include <libhat/process.hpp>

#include <mach-o/dyld.h>
#include <mach-o/loader.h>
#include <mach/mach.h>
#include <mach/mach_vm.h>
#include <mach/vm_prot.h>

#include <dlfcn.h>

#include <concepts>
#include <cstdlib>
#include <memory>
#include <string>

namespace {
#ifdef LIBHAT_LP64
    using mach_header_t = ::mach_header_64;
    using segment_command_t = ::segment_command_64;
    using section_t = ::section_64;
    static constexpr std::uint32_t mh_magic = MH_MAGIC_64;
    static constexpr std::uint32_t mh_cigam = MH_CIGAM_64;
    static constexpr std::uint32_t lc_segment = LC_SEGMENT_64;
#else
    using mach_header_t = ::mach_header;
    using segment_command_t = ::segment_command;
    using section_t = ::section;
    static constexpr std::uint32_t mh_magic = MH_MAGIC;
    static constexpr std::uint32_t mh_cigam = MH_CIGAM;
    static constexpr std::uint32_t lc_segment = LC_SEGMENT;
#endif

    using Handle = std::unique_ptr<void, decltype([](void* handle) {
        dlclose(handle);
    })>;

    struct module_implementation {
        module_implementation(Handle handle, const mach_header_t* header, std::ptrdiff_t slide) :
            handle_(std::move(handle)),
            header(header),
            vmaddr_slide(slide) {}

        [[nodiscard]] std::uintptr_t address() const {
            return std::bit_cast<std::uintptr_t>(this->header);
        }

        [[nodiscard]] void* handle() const {
            return this->handle_.get();
        }

        [[nodiscard]] std::ptrdiff_t slide() const {
            return this->vmaddr_slide;
        }

        void for_each_segment(std::predicate<const segment_command_t*> auto&& callback) const {
            const auto* cmd = reinterpret_cast<const load_command*>(
                reinterpret_cast<const std::byte*>(header) + sizeof(mach_header_t));

            for (std::uint32_t j = 0; j < header->ncmds; j++) {
                if (cmd->cmd == lc_segment) {
                    const auto* seg = reinterpret_cast<const segment_command_t*>(cmd);
                    if (seg->vmsize != 0 && seg->initprot != 0) {
                        if (!callback(seg)) {
                            return;
                        }
                    }
                }
                cmd = reinterpret_cast<const load_command*>(
                    reinterpret_cast<const std::byte*>(cmd) + cmd->cmdsize);
            }
        }

        void for_each_section(std::predicate<const segment_command_t*, const section_t*> auto&& callback) const {
            this->for_each_segment([&](const segment_command_t* seg) {
                const auto* sections = reinterpret_cast<const section_t*>(
                    reinterpret_cast<const std::byte*>(seg) + sizeof(segment_command_t));
                for (std::uint32_t i = 0; i < seg->nsects; i++) {
                    auto* sec = &sections[i];
                    if (sec->addr) {
                        if (!callback(seg, sec)) {
                            return false;
                        }
                    }
                }
                return true;
            });
        }

    private:
        Handle               handle_;
        const mach_header_t* header;
        std::ptrdiff_t       vmaddr_slide;
    };
}

namespace hat::process {

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

#ifdef LIBHAT_LP64
    static bool regionHasFlags(const std::span<const std::byte> region, const vm_prot_t flags) {
        for (auto* addr = region.data(); addr < region.data() + region.size();) {
            mach_vm_address_t vm_addr = std::bit_cast<mach_vm_address_t>(addr);
            mach_vm_size_t size{};
            vm_region_basic_info_64 info{};
            mach_msg_type_number_t infoCount = VM_REGION_BASIC_INFO_COUNT_64;
            mach_port_t objectName{};

            const auto result = mach_vm_region(
                mach_task_self(),
                &vm_addr,
                &size,
                VM_REGION_BASIC_INFO_64,
                reinterpret_cast<vm_region_info_t>(&info),
                &infoCount,
                &objectName
            );
            if (result != KERN_SUCCESS) {
                return false;
            }
            if (!(info.protection & flags)) {
                return false;
            }
            addr = std::bit_cast<const std::byte*>(vm_addr) + size;
        }
        return true;
    }

    bool is_readable(const std::span<const std::byte> region) {
        return regionHasFlags(region, VM_PROT_READ);
    }

    bool is_writable(const std::span<const std::byte> region) {
        return regionHasFlags(region, VM_PROT_WRITE);
    }

    bool is_executable(const std::span<const std::byte> region) {
        return regionHasFlags(region, VM_PROT_EXECUTE);
    }
#endif

    hat::process::module get_process_module() {
        const std::uint32_t count = _dyld_image_count();
        for (std::uint32_t i = 0; i != count; i++) {
            const auto* header = reinterpret_cast<const mach_header_t*>(_dyld_get_image_header(i));
            if (!is_valid_header(header)) {
                continue;
            }

            if (header->filetype == MH_EXECUTE) {
                return hat::process::module{std::make_shared<module_implementation>(
                    Handle{dlopen(_dyld_get_image_name(i), RTLD_LAZY | RTLD_NOLOAD)},
                    header,
                    _dyld_get_image_vmaddr_slide(i)
                )};
            }
        }
        std::abort();
    }

    std::uintptr_t module::address() const {
        const auto mimpl = static_cast<const module_implementation*>(this->impl.get());
        return mimpl->address();
    }

    std::uintptr_t module::get_symbol(const std::string_view name) const {
        const auto mimpl = static_cast<const module_implementation*>(this->impl.get());

        const std::string buffer{name};
        return std::bit_cast<uintptr_t>(dlsym(mimpl->handle(), buffer.c_str()));
    }

    std::span<std::byte> module::get_module_data() const {
        const auto mimpl = static_cast<const module_implementation*>(this->impl.get());

        const std::uintptr_t offset = static_cast<std::uintptr_t>(mimpl->slide()) - mimpl->address();
        std::size_t max{};
        mimpl->for_each_segment([&](const segment_command_t* seg) {
            max = std::max(max, static_cast<std::size_t>(seg->vmaddr + seg->vmsize + offset));
            return true;
        });
        return {reinterpret_cast<std::byte*>(mimpl->address()), max};
    }

    std::span<std::byte> module::get_executable_data() const {
        if (const auto text = this->get_section_data("__TEXT,__text"); !text.empty()) {
            return text;
        }

        const auto mimpl = static_cast<const module_implementation*>(this->impl.get());

        std::span<std::byte> data{};
        mimpl->for_each_section([&](const segment_command_t* seg, const section_t* sec) {
            if ((seg->initprot & VM_PROT_READ) && !(seg->initprot & VM_PROT_WRITE) && (seg->initprot & VM_PROT_EXECUTE)) {
                if (sec->flags & S_ATTR_PURE_INSTRUCTIONS) {
                    data = {
                        reinterpret_cast<std::byte*>(sec->addr) + mimpl->slide(),
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
        const auto mimpl = static_cast<const module_implementation*>(this->impl.get());

        std::span<std::byte> data{};
        mimpl->for_each_section([&](const segment_command_t* seg, const section_t* sec) {
            const std::string_view segmentName{
                static_cast<const char*>(seg->segname),
                strnlen(static_cast<const char*>(seg->segname), 16)
            };
            const std::string_view sectionName{
                static_cast<const char*>(sec->sectname),
                strnlen(static_cast<const char*>(sec->sectname), 16)
            };
            const std::span sectionData{
                reinterpret_cast<std::byte*>(sec->addr) + mimpl->slide(),
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
        const auto mimpl = static_cast<const module_implementation*>(this->impl.get());

        mimpl->for_each_section([&](const segment_command_t* seg, const section_t* sec) {
            const std::string_view segmentName{
                static_cast<const char*>(seg->segname),
                strnlen(static_cast<const char*>(seg->segname), 16)
            };
            const std::string_view sectionName{
                static_cast<const char*>(sec->sectname),
                strnlen(static_cast<const char*>(sec->sectname), 16)
            };
            const std::span data{
                reinterpret_cast<std::byte*>(sec->addr) + mimpl->slide(),
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
        const auto mimpl = static_cast<const module_implementation*>(this->impl.get());

        mimpl->for_each_segment([&](const segment_command_t* seg) {
            const std::span data{
                reinterpret_cast<std::byte*>(seg->vmaddr) + mimpl->slide(),
                seg->vmsize
            };
            return callback(data, to_hat_prot(seg->initprot));
        });
    }

    std::optional<hat::process::module> get_module(const std::string_view name) {
        if (name.empty()) {
            return get_process_module();
        }

        const std::string buffer{name};
        const Handle handle{dlopen(buffer.c_str(), RTLD_LAZY | RTLD_NOLOAD)};
        if (!handle) {
            return {};
        }

        const std::uint32_t count = _dyld_image_count();
        for (std::uint32_t i = 0; i < count; i++) {
            const auto* header = reinterpret_cast<const mach_header_t*>(_dyld_get_image_header(i));
            if (!is_valid_header(header)) {
                continue;
            }

            Handle h{dlopen(_dyld_get_image_name(i), RTLD_LAZY | RTLD_NOLOAD)};
            if (h != handle) {
                continue;
            }

            return hat::process::module{std::make_shared<module_implementation>(
                std::move(h),
                header,
                _dyld_get_image_vmaddr_slide(i)
            )};
        }

        return {};
    }

    std::optional<hat::process::module> module_at(const void* address) {
        Dl_info dlinfo{};
        if (!dladdr(address, &dlinfo)) {
            return {};
        }

        const std::uint32_t count = _dyld_image_count();
        for (std::uint32_t i = 0; i < count; i++) {
            const auto* header = reinterpret_cast<const mach_header_t*>(_dyld_get_image_header(i));
            if (header != dlinfo.dli_fbase) {
                continue;
            }

            return hat::process::module{std::make_shared<module_implementation>(
                Handle{dlopen(_dyld_get_image_name(i), RTLD_LAZY | RTLD_NOLOAD)},
                header,
                _dyld_get_image_vmaddr_slide(i)
            )};
        }

        return {};
    }
}

#endif
