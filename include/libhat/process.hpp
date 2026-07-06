#pragma once

#ifndef LIBHAT_MODULE
    #include <cstddef>
    #include <cstdint>
    #include <functional>
    #include <optional>
    #include <span>
    #include <string_view>
#endif

#include "export.hpp"
#include "memory.hpp"

LIBHAT_EXPORT namespace hat::process {

    class module {
    public:
        /// Returns the base address of the module in memory, as a uintptr_t
        [[nodiscard]] uintptr_t address() const {
            return this->baseAddress;
        }

        /// Returns the complete memory region for the given module. This may include portions which are uncommitted.
        /// To verify whether the region is safe to read, use hat::process::is_readable.
        [[nodiscard]] std::span<std::byte> get_module_data() const;

        /// Returns the executable memory region containing machine code for the module. The standard segment or section
        /// which contains executable code for the current platform will be returned first. If it cannot be identified
        /// by name, the first executable region defined by the module will be returned instead.
        [[nodiscard]] std::span<std::byte> get_executable_data() const;

        /// Returns the memory region for a named section. On Linux-based platforms this is implemented via
        /// {@code for_each_section}, and subsequently requires file I/O and parsing (see below). Caching the return
        /// value should be considered if repeated calls are frequently made. If looking up multiple named sections is
        /// required, consider doing so with a single call to {@code for_each_section}. On systems using the Mach-O
        /// format, "SEGNAME,SECNAME" is supported for disambiguation. i.e. "__TEXT,__const" vs "__DATA,__const"
        [[nodiscard]] std::span<std::byte> get_section_data(std::string_view name) const;

        /// Invokes the callback for each named linker section defined by this module as long as it returns true. The
        /// returned byte range is not guaranteed to have page aligned begin and end addresses. The returned protections
        /// are yielded from the section headers, and may not reflect the current virtual protections for the relevant
        /// memory pages. On Linux-based platforms, section information is not mapped into memory by the loader, so the
        /// implementation of this function has to read the module's associated ELF and parse the respective headers.
        void for_each_section(const std::function<bool(std::string_view name, std::span<std::byte>, hat::protection)>& callback) const;

        /// Invokes the callback for each memory segment defined by this module as long as it returns true. Depending on
        /// the platform, a segment may be represented by multiple linker sections. The returned byte range is not
        /// guaranteed to have page aligned begin and end addresses. The returned protections are yielded from the
        /// segment headers, and may not reflect the current virtual protections for the relevant memory pages.
        void for_each_segment(const std::function<bool(std::span<std::byte>, hat::protection)>& callback) const;

        [[nodiscard]] constexpr auto operator<=>(const module&) const noexcept = default;

    private:
        explicit module(const uintptr_t baseAddress)
            : baseAddress(baseAddress) {}

        friend hat::process::module get_process_module();
        friend std::optional<hat::process::module> get_module(std::string_view);
        friend std::optional<hat::process::module> module_at(void* address, std::optional<size_t> size);

        uintptr_t baseAddress{};
    };

    /// Returns whether the entirety of a given memory region is readable
    [[nodiscard]] bool is_readable(std::span<const std::byte> region);

    /// Returns whether the entirety of a given memory region is writable
    [[nodiscard]] bool is_writable(std::span<const std::byte> region);

    /// Returns whether the entirety of a given memory region is executable
    [[nodiscard]] bool is_executable(std::span<const std::byte> region);

    /// Returns the module for the current process's base executable
    [[nodiscard]] hat::process::module get_process_module();

    /// Returns an optional containing the module with the given name in the current process
    /// If the module is not found, std::nullopt is returned instead
    [[nodiscard]] std::optional<hat::process::module> get_module(std::string_view name);

    /// Returns the module located at the specified base address. Optionally, a size may be provided to indicate
    /// the size of the allocation pointed to by address, preventing a potential out-of-bounds read.
    /// If the given address does not point to a valid module, std::nullopt is returned instead
    [[nodiscard]] std::optional<hat::process::module> module_at(void* address, std::optional<size_t> size = {});
}
