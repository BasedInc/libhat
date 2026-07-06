#include <libhat/defines.hpp>
#ifdef LIBHAT_LINUX

#include <libhat/process.hpp>

#include <dlfcn.h>
#include <link.h>

#include <string.h>

#include <fstream>
#include <memory>

#include "Common.hpp"
#include "../../Utils.hpp"

namespace hat::process {

#ifdef LIBHAT_LP64
    using elf_ehdr_t = Elf64_Ehdr;
    using elf_phdr_t = Elf64_Phdr;
    using elf_shdr_t = Elf64_Shdr;
    static constexpr unsigned char elf_class = ELFCLASS64;
#else
    using elf_ehdr_t = Elf32_Ehdr;
    using elf_phdr_t = Elf32_Phdr;
    using elf_shdr_t = Elf32_Shdr;
    static constexpr unsigned char elf_class = ELFCLASS32;
#endif

    using Handle = std::unique_ptr<void, decltype([](void* handle) {
        dlclose(handle);
    })>;

    static void for_each_segment_impl(const uintptr_t address, std::predicate<const elf_phdr_t&> auto&& callback) {
        auto phdrCallback = [&](const dl_phdr_info& info) {
            const auto addr = std::bit_cast<uintptr_t>(info.dlpi_addr);
            if (addr != address) {
                return 0;
            }

            for (size_t i = 0; i < info.dlpi_phnum; i++) {
                auto& header = info.dlpi_phdr[i];
                if (header.p_type == PT_LOAD || header.p_type == PT_GNU_RELRO) {
                    if (!callback(header)) {
                        break;
                    }
                }
            }

            return 1;
        };

        dl_iterate_phdr(
            [](dl_phdr_info* info, size_t, void* data) {
                return (*static_cast<decltype(phdrCallback)*>(data))(*info);
            },
            &phdrCallback);
    }

    hat::process::module get_process_module() {
        const auto module = get_module({});
        if (!module) {
            std::abort();
        }
        return *module;
    }

    std::span<std::byte> module::get_module_data() const {
        size_t max{};
        for_each_segment_impl(this->address(), [&](const elf_phdr_t& header) {
            max = std::max(max, detail::fast_align_up(header.p_vaddr + header.p_memsz,
                header.p_align ? header.p_align : 1));
            return true;
        });
        return {reinterpret_cast<std::byte*>(this->address()), max};
    }

    std::span<std::byte> module::get_executable_data() const {
        std::span<std::byte> data{};
        for_each_segment_impl(this->address(), [&](const elf_phdr_t& header) {
            if ((header.p_flags & PF_R) && !(header.p_flags & PF_W) && (header.p_flags & PF_X)) {
                data = {
                    reinterpret_cast<std::byte*>(this->address() + header.p_vaddr),
                    header.p_memsz
                };
                return false;
            }
            return true;
        });
        return data;
    }

    std::span<std::byte> module::get_section_data(std::string_view name) const {
        std::span<std::byte> data{};
        this->for_each_section([&](auto section_name, auto section_data, auto) -> bool {
            if (section_name == name) {
                data = section_data;
                return false;
            }
            return true;
        });
        return data;
    }

    void module::for_each_section(const std::function<bool(std::string_view, std::span<std::byte>, hat::protection)>& callback) const {
        auto phdrCallback = [&](const dl_phdr_info& info) {
            const auto addr = std::bit_cast<uintptr_t>(info.dlpi_addr);
            if (addr != this->address()) {
                return 0;
            }

            const char* path = (info.dlpi_name && *info.dlpi_name != '\0')
                ? info.dlpi_name : "/proc/self/exe";
            std::ifstream file{path, std::ios::binary};
            if (!file.is_open()) {
                return 0; // ?????
            }

            elf_ehdr_t ehdr{};
            if (!file.read(reinterpret_cast<char*>(&ehdr), EI_NIDENT)) {
                return 0; // Invalid ELF
            }

            const auto& e_ident = ehdr.e_ident;
            if (e_ident[EI_MAG0] != ELFMAG0 || e_ident[EI_MAG1] != ELFMAG1
                || e_ident[EI_MAG2] != ELFMAG2 || e_ident[EI_MAG3] != ELFMAG3
                || e_ident[EI_CLASS] != elf_class) {
                return 0; // Invalid ELF
            }

            if (!file.read(reinterpret_cast<char*>(&ehdr) + EI_NIDENT, sizeof(ehdr) - EI_NIDENT)) {
                return 0; // Invalid ELF
            }

            if (!ehdr.e_shnum || ehdr.e_shentsize < sizeof(elf_shdr_t)) {
                return 0; // No section header table, or entries are smaller than expected. Nothing to do
            }

            std::vector<char> sections(ehdr.e_shnum * ehdr.e_shentsize);
            if (!file.seekg(static_cast<std::streamoff>(ehdr.e_shoff), std::ios::beg)) {
                return 0;
            }
            if (!file.read(sections.data(), static_cast<std::streamsize>(sections.size()))) {
                return 0;
            }

            elf_shdr_t shstrtab{};
            if (ehdr.e_shstrndx >= ehdr.e_shnum) {
                return 0;
            }
            std::memcpy(&shstrtab, sections.data() + ehdr.e_shstrndx * ehdr.e_shentsize, sizeof(shstrtab));

            std::vector<char> strings(shstrtab.sh_size);
            if (!file.seekg(static_cast<std::streamoff>(shstrtab.sh_offset), std::ios::beg)) {
                return 0;
            }
            if (!file.read(strings.data(), static_cast<std::streamsize>(strings.size()))) {
                return 0;
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
                    reinterpret_cast<std::byte*>(this->address()) + header.sh_addr,
                    header.sh_size
                };

                hat::protection prot = hat::protection::Read;
                if (header.sh_flags & SHF_WRITE) prot |= hat::protection::Write;
                if (header.sh_flags & SHF_EXECINSTR) prot |= hat::protection::Execute;

                if (!callback(name, data, prot)) {
                    break;
                }
            }

            return 1;
        };

        dl_iterate_phdr(
            [](dl_phdr_info* info, size_t, void* data) {
                return (*static_cast<decltype(phdrCallback)*>(data))(*info);
            },
            &phdrCallback);
    }

    void module::for_each_segment(const std::function<bool(std::span<std::byte>, hat::protection)>& callback) const {
        for_each_segment_impl(this->address(), [&](const elf_phdr_t& header) {
            const std::span data{
                reinterpret_cast<std::byte*>(this->address() + header.p_vaddr),
                header.p_memsz
            };

            hat::protection prot{};
            if (header.p_flags & PF_R) prot |= hat::protection::Read;
            if (header.p_flags & PF_W) prot |= hat::protection::Write;
            if (header.p_flags & PF_X) prot |= hat::protection::Execute;

            return callback(data, prot);
        });
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
            const Handle h{dlopen(info.dlpi_name, RTLD_LAZY | RTLD_NOLOAD)};
            if (h == handle) {
                module = hat::process::module{std::bit_cast<uintptr_t>(info.dlpi_addr)};
                return 1;
            }
            return 0;
        };

        dl_iterate_phdr(
            [](dl_phdr_info* info, size_t, void* data) {
                return (*static_cast<decltype(callback)*>(data))(*info);
            },
            &callback);

        return module;
    }

    static bool regionHasFlags(const std::span<const std::byte> region, const uint32_t flags) {
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

    std::optional<hat::process::module> module_at(void* address, [[maybe_unused]] const std::optional<size_t> size) {
        std::optional<hat::process::module> module{};
        auto callback = [&](const dl_phdr_info& info) {
            if (std::bit_cast<void*>(info.dlpi_addr) == address) {
                module = hat::process::module{std::bit_cast<uintptr_t>(info.dlpi_addr)};
                return 1;
            }
            return 0;
        };

        dl_iterate_phdr(
            [](dl_phdr_info* info, size_t, void* data) {
                return (*static_cast<decltype(callback)*>(data))(*info);
            },
            &callback);

        return module;
    }

    bool is_readable(const std::span<const std::byte> region) {
        return regionHasFlags(region, PROT_READ);
    }

    bool is_writable(const std::span<const std::byte> region) {
        return regionHasFlags(region, PROT_WRITE);
    }

    bool is_executable(const std::span<const std::byte> region) {
        return regionHasFlags(region, PROT_EXEC);
    }
}

#endif
