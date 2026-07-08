#include <libhat/defines.hpp>
#ifdef LIBHAT_LINUX

#include <libhat/process.hpp>

#include <dlfcn.h>
#include <link.h>

#include <string.h>

#include <fstream>
#include <map>
#include <memory>
#include <mutex>

#include "Common.hpp"
#include "../../Utils.hpp"

namespace {

    using elf_ehdr_t = ElfW(Ehdr);
    using elf_phdr_t = ElfW(Phdr);
    using elf_shdr_t = ElfW(Shdr);
    using elf_addr_t = ElfW(Addr);
    using elf_half_t = ElfW(Half);

#ifdef LIBHAT_LP64
    constexpr unsigned char elf_class = ELFCLASS64;
#else
    constexpr unsigned char elf_class = ELFCLASS32;
#endif

    using Handle = std::unique_ptr<void, decltype([](void* handle) {
        dlclose(handle);
    })>;

    struct module_implementation {
        module_implementation(Handle handle, const dl_phdr_info& info) :
            handle(std::move(handle)),
            base_address(static_cast<uintptr_t>(info.dlpi_addr)),
            path(info.dlpi_name && *info.dlpi_name != '\0' ? info.dlpi_name : "/proc/self/exe"),
            program_headers(info.dlpi_phdr),
            num_program_headers(info.dlpi_phnum) {}

        [[nodiscard]] uintptr_t address() const {
            return this->base_address;
        }

        [[nodiscard]] std::span<const elf_phdr_t> headers() const {
            return {this->program_headers, this->num_program_headers};
        }

        [[nodiscard]] auto& sections() const {
            std::call_once(this->init_sections_flag, &module_implementation::init_sections, this);
            return std::as_const(this->name_to_section);
        }

    private:
        void init_sections() const {
            std::ifstream file{this->path, std::ios::binary};
            if (!file.is_open()) {
                return; // ?????
            }

            elf_ehdr_t ehdr{};
            if (!file.read(reinterpret_cast<char*>(&ehdr), sizeof(ehdr))) {
                return; // Invalid ELF
            }

            const auto& e_ident = ehdr.e_ident;
            if (e_ident[EI_MAG0] != ELFMAG0 || e_ident[EI_MAG1] != ELFMAG1
                || e_ident[EI_MAG2] != ELFMAG2 || e_ident[EI_MAG3] != ELFMAG3
                || e_ident[EI_CLASS] != elf_class) {
                return; // Invalid ELF
            }

            if (!ehdr.e_shnum || ehdr.e_shentsize < sizeof(elf_shdr_t)) {
                return; // No section header table, or entries are smaller than expected. Nothing to do
            }

            std::vector<char> sections(ehdr.e_shnum * ehdr.e_shentsize);
            if (!file.seekg(static_cast<std::streamoff>(ehdr.e_shoff), std::ios::beg)) {
                return;
            }
            if (!file.read(sections.data(), static_cast<std::streamsize>(sections.size()))) {
                return;
            }

            elf_shdr_t shstrtab{};
            if (ehdr.e_shstrndx >= ehdr.e_shnum) {
                return;
            }
            std::memcpy(&shstrtab, sections.data() + ehdr.e_shstrndx * ehdr.e_shentsize, sizeof(shstrtab));

            std::vector<char> strings(shstrtab.sh_size);
            if (!file.seekg(static_cast<std::streamoff>(shstrtab.sh_offset), std::ios::beg)) {
                return;
            }
            if (!file.read(strings.data(), static_cast<std::streamsize>(strings.size()))) {
                return;
            }

            for (auto it = sections.begin(); it != sections.end(); it += ehdr.e_shentsize) {
                elf_shdr_t header{};
                std::memcpy(&header, std::to_address(it), sizeof(header));
                if (header.sh_type == SHT_NULL) {
                    continue;
                }

                if (!header.sh_addr || header.sh_name > strings.size() || (header.sh_flags & SHF_ALLOC) == 0) {
                    continue;
                }

                const char* cstr = strings.data() + header.sh_name;
                const size_t size = strnlen(cstr, strings.size() - header.sh_name);
                const std::string_view name{cstr, size};

                const std::span data{
                    reinterpret_cast<std::byte*>(this->base_address) + header.sh_addr,
                    header.sh_size
                };

                hat::protection prot = hat::protection::Read;
                if (header.sh_flags & SHF_WRITE) prot |= hat::protection::Write;
                if (header.sh_flags & SHF_EXECINSTR) prot |= hat::protection::Execute;

                this->name_to_section.emplace(std::piecewise_construct,
                    std::forward_as_tuple(name),
                    std::forward_as_tuple(data, prot)
                );
            }
        }

        using sections_t = std::map<std::string, std::pair<std::span<std::byte>, hat::protection>, std::less<>>;

        Handle                 handle;
        uintptr_t              base_address;
        const char*            path;
        const elf_phdr_t*      program_headers;
        elf_half_t             num_program_headers;
        mutable std::once_flag init_sections_flag;
        mutable sections_t     name_to_section;
    };
}

namespace hat::process {

    hat::process::module get_process_module() {
        const auto module = get_module({});
        if (!module) {
            std::abort();
        }
        return *module;
    }

    uintptr_t module::address() const {
        const auto mimpl = static_cast<const module_implementation*>(this->impl.get());
        return mimpl->address();
    }

    std::span<std::byte> module::get_module_data() const {
        const auto mimpl = static_cast<const module_implementation*>(this->impl.get());

        size_t max{};
        for (auto& header : mimpl->headers()) {
            max = std::max(max, static_cast<size_t>(header.p_vaddr + header.p_memsz));
        }
        return {reinterpret_cast<std::byte*>(mimpl->address()), max};
    }

    std::span<std::byte> module::get_executable_data() const {
        if (const auto text = this->get_section_data(".text"); !text.empty()) {
            return text;
        }

        const auto mimpl = static_cast<const module_implementation*>(this->impl.get());

        for (auto& [data, prot] : mimpl->sections() | std::views::values) {
            if (prot == (hat::protection::Read | hat::protection::Execute)) {
                return data;
            }
        }

        for (auto& header : mimpl->headers()) {
            if ((header.p_flags & PF_R) && !(header.p_flags & PF_W) && (header.p_flags & PF_X)) {
                return {
                    reinterpret_cast<std::byte*>(mimpl->address() + header.p_vaddr),
                    header.p_memsz
                };
            }
        }
        return {};
    }

    std::span<std::byte> module::get_section_data(const std::string_view name) const {
        const auto mimpl = static_cast<module_implementation*>(this->impl.get());

        auto& sections = mimpl->sections();
        if (const auto it = sections.find(name); it != sections.end()) {
            return it->second.first;
        }
        return {};
    }

    void module::for_each_section(const std::function<bool(std::string_view, std::span<std::byte>, hat::protection)>& callback) const {
        const auto mimpl = static_cast<module_implementation*>(this->impl.get());

        for (auto& [name, section] : mimpl->sections()) {
            if (!callback(name, section.first, section.second)) {
                break;
            }
        }
    }

    void module::for_each_segment(const std::function<bool(std::span<std::byte>, hat::protection)>& callback) const {
        const auto mimpl = static_cast<const module_implementation*>(this->impl.get());

        for (auto& header : mimpl->headers()) {
            const std::span data{
                reinterpret_cast<std::byte*>(mimpl->address() + header.p_vaddr),
                header.p_memsz
            };

            hat::protection prot{};
            if (header.p_flags & PF_R) prot |= hat::protection::Read;
            if (header.p_flags & PF_W) prot |= hat::protection::Write;
            if (header.p_flags & PF_X) prot |= hat::protection::Execute;

            if (!callback(data, prot)) {
                break;
            }
        }
    }

    std::optional<hat::process::module> get_module(const std::string_view name) {
        const std::string buffer{name};
        const char* file = buffer.empty() ? nullptr : buffer.c_str();

        const Handle handle{dlopen(file, RTLD_LAZY | RTLD_NOLOAD)};
        if (!handle) {
            return {};
        }

        std::optional<hat::process::module> module{};
        auto callback = [&](const dl_phdr_info& info) {
            if (!info.dlpi_addr) return 0;
            Handle h{dlopen(info.dlpi_name, RTLD_LAZY | RTLD_NOLOAD)};
            if (h != handle) {
                return 0;
            }

            module = hat::process::module{std::make_shared<module_implementation>(std::move(h), info)};
            return 1;
        };

        dl_iterate_phdr(
            [](dl_phdr_info* info, size_t, void* data) {
                return (*static_cast<decltype(callback)*>(data))(*info);
            },
            &callback);

        return module;
    }

    std::optional<hat::process::module> module_at(void* address) {
        Dl_info dlinfo{};
        if (!dladdr(address, &dlinfo)) {
            return {};
        }

        std::optional<hat::process::module> module{};
        auto callback = [&](const dl_phdr_info& info) {
            if (info.dlpi_addr != std::bit_cast<elf_addr_t>(dlinfo.dli_fbase)) {
                return 0;
            }

            module = hat::process::module{std::make_shared<module_implementation>(
                Handle{dlopen(info.dlpi_name, RTLD_LAZY | RTLD_NOLOAD)},
                info
            )};
            return 1;
        };

        dl_iterate_phdr(
            [](dl_phdr_info* info, size_t, void* data) {
                return (*static_cast<decltype(callback)*>(data))(*info);
            },
            &callback);

        return module;
    }

    static bool regionHasFlag(const std::span<const std::byte> region, const uint32_t flags) {
        if (region.empty()) {
            return false;
        }

        auto begin = std::bit_cast<uintptr_t>(region.data());
        const auto end = begin + region.size();

        iter_mapped_regions([&, found = false](const uintptr_t b, const uintptr_t e, const uint32_t prot) mutable {
            if (found) {
                // intervals must be contiguous
                if (b != begin) {
                    return false;
                }
            } else if (begin >= b && begin < e) {
                found = true;
            }

            if (found) {
                if (!(prot & flags)) {
                    return false;
                }
                begin = e;
            }

            return begin < end && begin >= b;
        });
        return begin >= end;
    }

    bool is_readable(const std::span<const std::byte> region) {
        return regionHasFlag(region, PROT_READ);
    }

    bool is_writable(const std::span<const std::byte> region) {
        return regionHasFlag(region, PROT_WRITE);
    }

    bool is_executable(const std::span<const std::byte> region) {
        return regionHasFlag(region, PROT_EXEC);
    }
}

#endif
