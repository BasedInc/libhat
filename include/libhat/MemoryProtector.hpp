#pragma once

#include <cstdint>
#include <type_traits>
#include <utility>

namespace hat {

    enum class protection : uint8_t {
        Read    = 0b001,
        Write   = 0b010,
        Execute = 0b100
    };

    constexpr protection operator|(protection lhs, protection rhs) {
        using U = std::underlying_type_t<protection>;
        return static_cast<protection>(static_cast<U>(lhs) | static_cast<U>(rhs));
    }

    constexpr protection operator&(protection lhs, protection rhs) {
        using U = std::underlying_type_t<protection>;
        return static_cast<protection>(static_cast<U>(lhs) & static_cast<U>(rhs));
    }

    /// RAII wrapper for setting memory protection flags
    class memory_protector {
    public:
        memory_protector(uintptr_t address, size_t size, protection flags);

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

        memory_protector& operator=(memory_protector&& o) noexcept = delete;
        memory_protector(const memory_protector&) = delete;
        memory_protector& operator=(const memory_protector&) = delete;

        /// Returns true if the memory protect operation was successful
        [[nodiscard]] bool is_set() const {
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
