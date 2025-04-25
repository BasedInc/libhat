#include <libhat/defines.hpp>
#ifdef LIBHAT_LINUX

#include <libhat/process.hpp>

#include <dlfcn.h>
#include <link.h>

#include <memory>

#include "Common.hpp"

namespace hat::process {

    hat::process::module get_process_module() {
        const auto module = get_module({});
        if (!module) {
            std::abort();
        }
        return *module;
    }

    void module::for_each_segment(const std::function<bool(std::span<std::byte>, hat::protection)>& callback) const {
        auto phdrCallback = [&](const dl_phdr_info& info) {
            const auto addr = std::bit_cast<uintptr_t>(info.dlpi_addr);
            if (addr != this->address()) {
                return 0;
            }

            for (size_t i = 0; i < info.dlpi_phnum; i++) {
                auto& header = info.dlpi_phdr[i];
                if (header.p_type != PT_LOAD) {
                    continue;
                }

                const std::span data{
                    reinterpret_cast<std::byte*>(this->address() + header.p_vaddr),
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

            return 1;
        };

        dl_iterate_phdr(
            [](dl_phdr_info* info, size_t, void* data) {
                return (*static_cast<decltype(phdrCallback)*>(data))(*info);
            },
            &phdrCallback);
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
        auto callback = [&](const dl_phdr_info& info) {
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
        auto addr = std::bit_cast<uintptr_t>(region.data());
        const auto end = addr + region.size();

        iter_mapped_regions([&](const uintptr_t b, const uintptr_t e, const uint32_t prot) {
            if (addr >= b && addr < e) {
                if (!(prot & flags)) {
                    return false;
                }
                addr = e;
            } else if (addr >= e) {
                return false;
            }
            return addr < end;
        });

        return addr >= end;
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
