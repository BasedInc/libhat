#pragma once

#include <algorithm>
#include <array>
#include <execution>
#include <utility>

#include "Defines.hpp"
#include "Process.hpp"
#include "Signature.hpp"

namespace hat {

    class scan_result {
        using rel_t = int32_t;
    public:
        constexpr scan_result() : result(nullptr) {}
        constexpr scan_result(std::byte* result) : result(result) {} // NOLINT(google-explicit-constructor)

        /// Reads an integer of the specified type located at an offset from the signature result
        template<std::integral Int>
        [[nodiscard]] constexpr Int read(size_t offset) const {
            return *reinterpret_cast<Int*>(this->result + offset);
        }

        /// Reads an integer of the specified type which represents an index into an array with the given element type
        template<std::integral Int, typename ArrayType>
        [[nodiscard]] constexpr size_t index(size_t offset) const {
            return static_cast<size_t>(read<Int>(offset)) / sizeof(ArrayType);
        }

        /// Resolve the relative address located at an offset from the signature result
        [[nodiscard]] constexpr std::byte* rel(size_t offset) const {
            return this->has_result() ? this->result + this->read<rel_t>(offset) + offset + sizeof(rel_t) : nullptr;
        }

        [[nodiscard]] constexpr bool has_result() const {
            return this->result != nullptr;
        }

        [[nodiscard]] constexpr std::byte* operator*() const {
            return this->result;
        }

        [[nodiscard]] constexpr std::byte* get() const {
            return this->result;
        }
    private:
        std::byte* result;
    };

    namespace detail {

        enum class scan_mode {
            Auto,
            Search,
            FastFirst,
            AVX2,
            AVX512,
            Neon
        };

        template<scan_mode>
        scan_result find_pattern(std::byte* begin, std::byte* end, signature_view signature);

        template<>
        scan_result find_pattern<scan_mode::Auto>(std::byte* begin, std::byte* end, signature_view signature);

        template<>
        constexpr scan_result find_pattern<scan_mode::FastFirst>(std::byte* begin, std::byte* end, signature_view signature) {
            const auto firstByte = *signature[0];
            const auto scanEnd = end - signature.size() + 1;

            for (auto i = begin; i != scanEnd; i++) {
                // Use std::find to efficiently find the first byte
                i = std::find(std::execution::unseq, i, scanEnd, firstByte);
                if (i == scanEnd) {
                    break;
                }
                // Compare everything after the first byte
                auto match = std::equal(signature.begin() + 1, signature.end(), i + 1, [](auto opt, auto byte) {
                    return !opt.has_value() || *opt == byte;
                });
                if (match) {
                    return i;
                }
            }
            return nullptr;
        }
    }

    enum class compiler_type {
        MSVC,
        MinGW
    };

    /// Gets the VTable address for a class by its mangled name
    template<compiler_type compiler>
    scan_result find_vtable(
        const std::string&  className,
        process::module_t   module = process::get_process_module()
    );

    /// Perform a signature scan on the entirety of the process module or a specified module
    scan_result find_pattern(
        signature_view      signature,
        process::module_t   module = process::get_process_module()
    );

    /// Perform a signature scan on a specific section of the process module or a specified module
    scan_result find_pattern(
        signature_view      signature,
        std::string_view    section,
        process::module_t   module = process::get_process_module()
    );

    /// Root implementation of FindPattern
    constexpr scan_result find_pattern(
        std::byte*      begin,
        std::byte*      end,
        signature_view  signature
    ) {
        if LIBHAT_IF_CONSTEVAL {
            return detail::find_pattern<detail::scan_mode::FastFirst>(begin, end, signature);
        } else {
            return detail::find_pattern<detail::scan_mode::Auto>(begin, end, signature);
        }
    }
}
