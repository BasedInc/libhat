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
        constexpr scan_result(const std::byte* result) : result(result) {} // NOLINT(google-explicit-constructor)

        /// Reads an integer of the specified type located at an offset from the signature result
        template<std::integral Int>
        [[nodiscard]] constexpr Int read(size_t offset) const {
            return *reinterpret_cast<const Int*>(this->result + offset);
        }

        /// Reads an integer of the specified type which represents an index into an array with the given element type
        template<std::integral Int, typename ArrayType>
        [[nodiscard]] constexpr size_t index(size_t offset) const {
            return static_cast<size_t>(read<Int>(offset)) / sizeof(ArrayType);
        }

        /// Resolve the relative address located at an offset from the signature result
        [[nodiscard]] constexpr const std::byte* rel(size_t offset) const {
            return this->has_result() ? this->result + this->read<rel_t>(offset) + offset + sizeof(rel_t) : nullptr;
        }

        [[nodiscard]] constexpr bool has_result() const {
            return this->result != nullptr;
        }

        [[nodiscard]] constexpr const std::byte* operator*() const {
            return this->result;
        }

        [[nodiscard]] constexpr const std::byte* get() const {
            return this->result;
        }
    private:
        const std::byte* result;
    };

    namespace detail {

        template<typename T>
        concept byte_iterator = std::forward_iterator<T>
            && std::contiguous_iterator<T>
            && std::same_as<std::iter_value_t<T>, std::byte>;

        enum class scan_mode {
            Auto,      // Automatically choose the mode to use
            Search,    // std::search
            FastFirst, // std::find + std::equal
            AVX2,      // x86 AVX2
            AVX512,    // x86 AVX512
            Neon,      // ARM Neon

            // Fallback mode to use for SIMD remaining bytes
            Single = FastFirst
        };

        template<scan_mode>
        scan_result find_pattern(const std::byte* begin, const std::byte* end, signature_view signature);

        template<>
        scan_result find_pattern<scan_mode::Auto>(const std::byte* begin, const std::byte* end, signature_view signature);

        template<>
        constexpr scan_result find_pattern<scan_mode::FastFirst>(const std::byte* begin, const std::byte* end, signature_view signature) {
            const auto firstByte = *signature[0];
            const auto scanEnd = end - signature.size() + 1;

            for (auto i = begin; i != scanEnd; i++) {
                // Use std::find to efficiently find the first byte
                if LIBHAT_IF_CONSTEVAL {
                    i = std::find(i, scanEnd, firstByte);
                } else {
                    i = std::find(std::execution::unseq, i, scanEnd, firstByte);
                }
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
    template<detail::byte_iterator Iter>
    constexpr scan_result find_pattern(
        Iter            begin,
        Iter            end,
        signature_view  signature
    ) {
        if LIBHAT_IF_CONSTEVAL {
            return detail::find_pattern<detail::scan_mode::Single>(std::to_address(begin), std::to_address(end), signature);
        } else {
            return detail::find_pattern<detail::scan_mode::Auto>(std::to_address(begin), std::to_address(end), signature);
        }
    }
}
