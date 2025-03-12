#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>

namespace hat::process {

    class module {
    public:
        /// Returns the base address of the module in memory, as a uintptr_t
        [[nodiscard]] uintptr_t address() const {
            return this->baseAddress;
        }

        /// Returns the complete memory region for the given module. This may include portions which are uncommitted.
        /// To verify whether the region is safe to read, use hat::process::is_readable.
        [[nodiscard]] std::span<std::byte> get_module_data() const;

        /// Returns the memory region for a named section
        [[nodiscard]] std::span<std::byte> get_section_data(std::string_view name) const;
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
