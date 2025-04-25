#pragma once

#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <utility>

#include "memory.hpp"

namespace hat {

    /// RAII wrapper for setting memory protection flags
    class memory_protector {
    public:
        [[nodiscard]] memory_protector(uintptr_t address, size_t size, protection flags);

        ~memory_protector() {
            if (this->set) {
                this->restore();
            }
        }

        memory_protector(memory_protector&& o) noexcept :
            address(o.address),
            size(o.size),
            oldProtection(o.oldProtection),
            set(std::exchange(o.set, false)) {}

        memory_protector& operator=(memory_protector&& o) = delete;
        memory_protector(const memory_protector&) = delete;
        memory_protector& operator=(const memory_protector&) = delete;

        /// Returns true if the memory protect operation was successful
        [[nodiscard]] bool is_set() const noexcept {
            return this->set;
        }

    private:
        void restore();

        uintptr_t address;
        size_t size;
        uint32_t oldProtection{}; // Memory protection flags native to Operating System
        bool set{};
    };
}
