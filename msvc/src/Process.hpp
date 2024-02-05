#pragma once

#include <span>
#include <string_view>

namespace hat::process {

    // TODO: Consider using a typedef or class instead? idk
    enum class module_t : uintptr_t {};

    /// Returns the module for the current process's base executable
    auto get_process_module() -> module_t;

    /// Returns a module by its given name in the current process
    auto get_module(const std::string& name) -> module_t;

    /// Returns the module located at the specified base address
    auto module_at(uintptr_t address) -> module_t;

    /// Returns the module located at the specified base address
    auto module_at(std::byte* address) -> module_t;

    /// Returns the complete memory region for the given module. This may include portions which are uncommitted.
    auto get_module_data(module_t mod) -> std::span<std::byte>;

    /// Returns the memory region for a named section in the given module
    auto get_section_data(module_t mod, std::string_view name) -> std::span<std::byte>;
}
