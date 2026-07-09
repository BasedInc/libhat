#pragma once

#ifndef LIBHAT_MODULE
    #include <cstddef>
    #include <cstdint>
    #include <functional>
    #include <memory>
    #include <optional>
    #include <span>
    #include <string_view>
#endif

#include "export.hpp"
#include "memory.hpp"

LIBHAT_EXPORT namespace hat::process {

    /// A shared reference to a loaded module in the current process. The underlying module will not be unloaded so long
    /// as the associated @code hat::process::module@endcode is within scope.
    class module {
    public:
        /// Returns the base address of the module in memory, as a uintptr_t
        [[nodiscard]] std::uintptr_t address() const;

        /// Returns the complete memory region for the given module. This may include portions which are uncommitted.
        /// To verify whether the region is safe to read, use @code hat::process::is_readable@endcode.
        [[nodiscard]] std::span<std::byte> get_module_data() const;

        /// Returns the executable memory region containing machine code for the module. The standard section which
        /// contains executable code for the current platform will be returned first. If it cannot be identified
        /// by name, the first executable region defined by the module will be returned instead.
        [[nodiscard]] std::span<std::byte> get_executable_data() const;

        /// Returns the memory region for a named section. On Linux-based platforms, section names are lazily loaded
        /// from the file that initialized the module, and internally cached for subsequent calls. On systems using the
        /// Mach-O format, "SEGNAME,SECNAME" is supported for disambiguation. i.e. "__TEXT,__const" vs "__DATA,__const"
        [[nodiscard]] std::span<std::byte> get_section_data(std::string_view name) const;

        /// Invokes the callback for each named linker section defined by this module as long as it returns true. The
        /// returned byte range is not guaranteed to have page aligned begin and end addresses. The returned protections
        /// are yielded from the section headers, and may not reflect the current virtual protections for the relevant
        /// memory pages. On Linux-based platforms, section names are lazily loaded from the file that initialized the
        /// module, and internally cached for subsequent calls.
        void for_each_section(const std::function<bool(std::string_view name, std::span<std::byte>, hat::protection)>& callback) const;

        /// Invokes the callback for each memory segment defined by this module as long as it returns true. Depending on
        /// the platform, a segment may be represented by multiple linker sections. The returned byte range is not
        /// guaranteed to have page aligned begin and end addresses. The returned protections are yielded from the
        /// segment headers, and may not reflect the current virtual protections for the relevant memory pages.
        void for_each_segment(const std::function<bool(std::span<std::byte>, hat::protection)>& callback) const;

        [[nodiscard]] bool operator==(const module& other) const noexcept {
            return this->address() == other.address();
        }

        [[nodiscard]] auto operator<=>(const module& other) const noexcept {
            return this->address() <=> other.address();
        }

    private:
        explicit module(std::shared_ptr<void> impl)
            : impl(std::move(impl)) {}

        friend hat::process::module get_process_module();
        friend std::optional<hat::process::module> get_module(std::string_view);
        friend std::optional<hat::process::module> module_at(void* address);

        std::shared_ptr<void> impl{};
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

    /// Returns the module containing the specified address. If the given address is not located within a
    /// loaded module, std::nullopt is returned instead.
    [[nodiscard]] std::optional<hat::process::module> module_at(void* address);
}
