module;

#include <algorithm>
#include <array>
#include <execution>
#include <utility>
#include <span>

#include "Concepts.hpp"
#include "Defines.hpp"

export module libhat.scanner;
export import libhat.signature;
import libhat.process;

export namespace hat {

    class scan_result {
        using rel_t = int32_t;
    public:
        constexpr scan_result() : result(nullptr) {}
        constexpr scan_result(std::nullptr_t) : result(nullptr) {}          // NOLINT(google-explicit-constructor)
        constexpr scan_result(const std::byte* result) : result(result) {}  // NOLINT(google-explicit-constructor)

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

    enum class scan_alignment {
        X1 = 1,
        X16 = 16
    };

    namespace detail {

        enum class scan_mode {
            FastFirst, // std::find + std::equal
            SSE,       // x86 SSE 4.1
            AVX2,      // x86 AVX2
            AVX512,    // x86 AVX512

            // Fallback mode to use for SIMD remaining bytes
            Single = FastFirst
        };

        template<scan_alignment alignment>
        inline constexpr auto alignment_stride = static_cast<std::underlying_type_t<scan_alignment>>(alignment);

        template<std::integral type, scan_alignment alignment, auto stride = alignment_stride<alignment>>
        inline consteval auto create_alignment_mask() {
            type mask{};
            for (size_t i = 0; i < sizeof(type) * 8; i += stride) {
                mask |= (type(1) << i);
            }
            return mask;
        }

        template<scan_alignment alignment, auto stride = alignment_stride<alignment>>
        inline const std::byte* next_boundary_align(const std::byte* ptr) {
            if constexpr (stride == 1) {
                return ptr;
            }
            uintptr_t mod = reinterpret_cast<uintptr_t>(ptr) % stride;
            ptr += mod ? stride - mod : 0;
            return ptr;
        }

        template<scan_alignment alignment, auto stride = alignment_stride<alignment>>
        inline const std::byte* prev_boundary_align(const std::byte* ptr) {
            if constexpr (stride == 1) {
                return ptr;
            }
            uintptr_t mod = reinterpret_cast<uintptr_t>(ptr) % stride;
            return ptr - mod;
        }

        template<scan_mode, scan_alignment>
        scan_result find_pattern(const std::byte* begin, const std::byte* end, signature_view signature);

        template<scan_alignment alignment>
        scan_result find_pattern(const std::byte* begin, const std::byte* end, signature_view signature);

        template<>
        inline constexpr scan_result find_pattern<scan_mode::FastFirst, scan_alignment::X1>(const std::byte* begin, const std::byte* end, signature_view signature) {
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

        template<>
        inline scan_result find_pattern<scan_mode::FastFirst, scan_alignment::X16>(const std::byte* begin, const std::byte* end, signature_view signature) {
            const auto firstByte = *signature[0];

            const auto scanBegin = next_boundary_align<scan_alignment::X16>(begin);
            const auto scanEnd = prev_boundary_align<scan_alignment::X16>(end - signature.size() + 1);
            if (scanBegin >= scanEnd) {
                return {};
            }

            for (auto i = scanBegin; i != scanEnd; i += 16) {
                if (*i == firstByte) {
                    // Compare everything after the first byte
                    auto match = std::equal(signature.begin() + 1, signature.end(), i + 1, [](auto opt, auto byte) {
                        return !opt.has_value() || *opt == byte;
                    });
                    if (match) {
                        return i;
                    }
                }
            }
            return nullptr;
        }
    }

    /// Perform a signature scan on the entirety of the process module or a specified module
    template<scan_alignment alignment = scan_alignment::X1>
    scan_result find_pattern(
        signature_view      signature,
        process::module_t   mod = process::get_process_module()
    );

    /// Perform a signature scan on a specific section of the process module or a specified module
    template<scan_alignment alignment = scan_alignment::X1>
    scan_result find_pattern(
        signature_view      signature,
        std::string_view    section,
        process::module_t   mod = process::get_process_module()
    );

    /// Root implementation of find_pattern
    template<scan_alignment alignment = scan_alignment::X1, detail::byte_iterator Iter>
    constexpr scan_result find_pattern(
        Iter            begin,
        Iter            end,
        signature_view  signature
    ) {
        if LIBHAT_IF_CONSTEVAL {
            return detail::find_pattern<detail::scan_mode::Single, alignment>(std::to_address(begin), std::to_address(end), signature);
        } else {
            return detail::find_pattern<alignment>(std::to_address(begin), std::to_address(end), signature);
        }
    }
}

export namespace hat::experimental {

    enum class compiler_type {
        MSVC,
        GNU
    };

    /// Gets the VTable address for a class by its mangled name
    template<compiler_type compiler>
    scan_result find_vtable(
        const std::string&  className,
        process::module_t   mod = process::get_process_module()
    );
}
