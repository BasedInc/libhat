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
