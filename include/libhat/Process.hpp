#pragma once

#include <optional>
#include <span>
#include <string_view>

namespace hat::process {

    // TODO: Consider using a typedef or class instead? idk
    enum class module_t : uintptr_t {};

    /// Returns the module for the current process's base executable
    [[nodiscard]] module_t get_process_module();

    /// Returns an optional containing the module with the given name in the current process
    /// If the module is not found, std::nullopt is returned instead
    [[nodiscard]] std::optional<module_t> get_module(const std::string& name);

    /// Returns the module located at the specified base address. Optionally, a size may be provided to indicate
    /// the size of the allocation pointed to by address, preventing a potential out-of-bounds read.
    /// If the given address does not point to a valid module, std::nullopt is returned instead
    [[nodiscard]] std::optional<module_t> module_at(void* address, std::optional<size_t> size = {});

    /// Returns the complete memory region for the given module. This may include portions which are uncommitted.
    [[nodiscard]] std::span<std::byte> get_module_data(module_t mod);

    /// Returns the memory region for a named section in the given module
    [[nodiscard]] std::span<std::byte> get_section_data(module_t mod, std::string_view name);
}
