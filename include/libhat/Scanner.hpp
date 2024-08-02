#pragma once

#include <algorithm>
#include <array>
#include <execution>
#include <utility>

#include "Concepts.hpp"
#include "Defines.hpp"
#include "Process.hpp"
#include "Signature.hpp"

namespace hat {

    template<typename T> requires (std::is_pointer_v<T> && sizeof(std::remove_pointer_t<T>) == 1)
    class scan_result_base {
        using rel_t = int32_t;
    public:
        using underlying_type = T;

        constexpr scan_result_base() : result(nullptr) {}
        constexpr scan_result_base(std::nullptr_t) : result(nullptr) {} // NOLINT(google-explicit-constructor)
        constexpr scan_result_base(T result) : result(result) {}        // NOLINT(google-explicit-constructor)

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
        [[nodiscard]] constexpr T rel(size_t offset) const {
            return this->has_result() ? this->result + this->read<rel_t>(offset) + offset + sizeof(rel_t) : nullptr;
        }

        [[nodiscard]] constexpr bool has_result() const {
            return this->result != nullptr;
        }

        [[nodiscard]] constexpr T operator*() const {
            return this->result;
        }

        [[nodiscard]] constexpr T get() const {
            return this->result;
        }

        [[nodiscard]] constexpr bool operator==(const scan_result_base&) const noexcept = default;
    private:
        T result;
    };

    using scan_result = scan_result_base<std::byte*>;
    using const_scan_result = scan_result_base<const std::byte*>;

    enum class scan_alignment {
        X1 = 1,
        X16 = 16
    };

    enum class scan_hint : uint64_t {
        none   = 0,      // no hints
        x86_64 = 1 << 0, // The data being scanned is x86_64 machine code
        pair0  = 1 << 1, // Only utilize byte pair based scanning if the signature starts with a byte pair
    };

    constexpr scan_hint operator|(scan_hint lhs, scan_hint rhs) {
        using U = std::underlying_type_t<scan_hint>;
        return static_cast<scan_hint>(static_cast<U>(lhs) | static_cast<U>(rhs));
    }

    constexpr scan_hint operator&(scan_hint lhs, scan_hint rhs) {
        using U = std::underlying_type_t<scan_hint>;
        return static_cast<scan_hint>(static_cast<U>(lhs) & static_cast<U>(rhs));
    }

    namespace detail {

        class scan_context;

        using scan_function_t = const_scan_result(*)(const std::byte* begin, const std::byte* end, const scan_context& context);

        struct scanner_context {
            size_t vectorSize{};
        };

        class scan_context {
        public:
            signature_view signature{};
            scan_function_t scanner{};
            scan_alignment alignment{};
            scan_hint hints{};
            std::optional<size_t> pairIndex{};

            [[nodiscard]] constexpr const_scan_result scan(const std::byte* begin, const std::byte* end) const {
                return this->scanner(begin, end, *this);
            }

            void auto_resolve_scanner();
            void apply_hints(const scanner_context&);

            static constexpr scan_context create(signature_view signature, scan_alignment alignment, scan_hint hints);
        private:
            scan_context() = default;
        };

        enum class scan_mode {
            Single, // std::find + std::equal
            SSE,    // x86 SSE 4.1
            AVX2,   // x86 AVX2
            AVX512, // x86 AVX512
        };

        template<scan_alignment alignment>
        inline constexpr auto alignment_stride = static_cast<std::underlying_type_t<scan_alignment>>(alignment);

        template<std::integral type, scan_alignment alignment>
        LIBHAT_FORCEINLINE consteval auto create_alignment_mask() {
            type mask{};
            for (size_t i = 0; i < sizeof(type) * 8; i += alignment_stride<alignment>) {
                mask |= (type(1) << i);
            }
            return mask;
        }

        template<scan_alignment alignment>
        LIBHAT_FORCEINLINE const std::byte* next_boundary_align(const std::byte* ptr) {
            constexpr auto stride = alignment_stride<alignment>;
            if constexpr (stride == 1) {
                return ptr;
            }
            uintptr_t mod = reinterpret_cast<uintptr_t>(ptr) % stride;
            ptr += mod ? stride - mod : 0;
            return std::assume_aligned<stride>(ptr);
        }

        template<scan_alignment alignment>
        LIBHAT_FORCEINLINE const std::byte* prev_boundary_align(const std::byte* ptr) {
            constexpr auto stride = alignment_stride<alignment>;
            if constexpr (stride == 1) {
                return ptr;
            }
            const uintptr_t mod = reinterpret_cast<uintptr_t>(ptr) % stride;
            return std::assume_aligned<stride>(ptr - mod);
        }

        template<typename Type>
        LIBHAT_FORCEINLINE const std::byte* align_pointer_as(const std::byte* ptr) {
            constexpr size_t alignment = alignof(Type);
            const uintptr_t mod = reinterpret_cast<uintptr_t>(ptr) % alignment;
            ptr += mod ? alignment - mod : 0;
            return std::assume_aligned<alignment>(ptr);
        }

        template<typename Vector>
        LIBHAT_FORCEINLINE auto segment_scan(
            const std::byte* begin,
            const std::byte* end,
            const size_t signatureSize,
            const size_t cmpOffset
        ) -> std::tuple<std::span<const std::byte>, std::span<const Vector>, std::span<const std::byte>> {
            auto validateRange = [signatureSize](const std::byte* b, const std::byte* e) -> std::span<const std::byte> {
                if (b <= e && static_cast<size_t>(e - b) >= signatureSize) {
                    return {b, e};
                }
                return {};
            };

            // If the scan can't be vectorized, just do the single byte scanner "pre" part
            if (static_cast<size_t>(end - begin) < sizeof(Vector)) {
                return {validateRange(begin, end), {}, {}};
            }

            const auto preBegin = begin;
            const auto vecBegin = reinterpret_cast<const Vector*>(align_pointer_as<Vector>(preBegin + cmpOffset));
            const auto vecEnd = vecBegin + (static_cast<size_t>(end - reinterpret_cast<const std::byte*>(vecBegin)) - signatureSize) / sizeof(Vector);
            const auto preEnd = reinterpret_cast<const std::byte*>(vecBegin) - cmpOffset + signatureSize;
            const auto postBegin = reinterpret_cast<const std::byte*>(vecEnd);
            const auto postEnd = end;

            return {
                validateRange(preBegin, preEnd),
                std::span{vecBegin, vecEnd},
                validateRange(postBegin, postEnd)
            };
        }

        template<scan_mode>
        scan_function_t resolve_scanner(scan_context&);

        template<scan_alignment>
        const_scan_result find_pattern_single(const std::byte* begin, const std::byte* end, const scan_context&);

        template<>
        constexpr const_scan_result find_pattern_single<scan_alignment::X1>(const std::byte* begin, const std::byte* end, const scan_context& context) {
            const auto signature = context.signature;
            const auto firstByte = *signature[0];
            const auto scanEnd = end - signature.size() + 1;

            for (auto i = begin; i != scanEnd; i++) {
                // Use std::find to efficiently find the first byte
                if LIBHAT_IF_CONSTEVAL {
                    i = std::find(i, scanEnd, firstByte);
                } else {
                    i = std::find(std::execution::unseq, i, scanEnd, firstByte);
                }
                if (i == scanEnd) LIBHAT_UNLIKELY {
                    break;
                }
                // Compare everything after the first byte
                auto match = std::equal(signature.begin() + 1, signature.end(), i + 1, [](auto opt, auto byte) {
                    return !opt.has_value() || *opt == byte;
                });
                if (match) LIBHAT_UNLIKELY {
                    return i;
                }
            }
            return nullptr;
        }

        template<>
        inline const_scan_result find_pattern_single<scan_alignment::X16>(const std::byte* begin, const std::byte* end, const scan_context& context) {
            const auto signature = context.signature;
            const auto firstByte = *signature[0];

            const auto scanBegin = next_boundary_align<scan_alignment::X16>(begin);
            const auto scanEnd = prev_boundary_align<scan_alignment::X16>(end - signature.size() + 1);
            if (scanBegin >= scanEnd) {
                return nullptr;
            }

            for (auto i = scanBegin; i != scanEnd; i += 16) {
                if (*i == firstByte) {
                    // Compare everything after the first byte
                    auto match = std::equal(signature.begin() + 1, signature.end(), i + 1, [](auto opt, auto byte) {
                        return !opt.has_value() || *opt == byte;
                    });
                    if (match) LIBHAT_UNLIKELY {
                        return i;
                    }
                }
            }
            return nullptr;
        }

        template<>
        constexpr scan_function_t resolve_scanner<scan_mode::Single>(scan_context& context) {
            switch (context.alignment) {
                case scan_alignment::X1: return &find_pattern_single<scan_alignment::X1>;
                case scan_alignment::X16: return &find_pattern_single<scan_alignment::X16>;
            }
            LIBHAT_UNREACHABLE();
        }

        [[nodiscard]] constexpr auto truncate(const signature_view signature) noexcept {
            // Truncate the leading wildcards from the signature
            size_t offset = 0;
            for (const auto& elem : signature) {
                if (elem.has_value()) {
                    break;
                }
                offset++;
            }
            return std::make_pair(offset, signature.subspan(offset));
        }

        template<byte_input_iterator T>
        using result_type_for = std::conditional_t<std::is_const_v<std::remove_reference_t<std::iter_reference_t<T>>>,
            const_scan_result, scan_result>;

        constexpr scan_context scan_context::create(const signature_view signature, const scan_alignment alignment, const scan_hint hints) {
            scan_context ctx{};
            ctx.signature = signature;
            ctx.alignment = alignment;
            ctx.hints = hints;
            if LIBHAT_IF_CONSTEVAL {
                ctx.scanner = resolve_scanner<scan_mode::Single>(ctx);
            } else {
                ctx.auto_resolve_scanner();
            }
            return ctx;
        }
    }

    /// Perform a signature scan on the entirety of the process module or a specified module
    template<scan_alignment alignment = scan_alignment::X1>
    [[deprecated]] scan_result find_pattern(
        signature_view      signature,
        process::module_t   mod = process::get_process_module()
    );

    /// Perform a signature scan on a specific section of the process module or a specified module
    template<scan_alignment alignment = scan_alignment::X1>
    scan_result find_pattern(
        signature_view      signature,
        std::string_view    section,
        process::module_t   mod = process::get_process_module(),
        scan_hint           hints = scan_hint::none
    );

    /// Root implementation of find_pattern
    template<scan_alignment alignment = scan_alignment::X1, detail::byte_input_iterator Iter>
    constexpr auto find_pattern(
        const Iter            beginIt,
        const Iter            endIt,
        const signature_view  signature,
        const scan_hint       hints = scan_hint::none
    ) -> detail::result_type_for<Iter> {
        const auto [offset, trunc] = detail::truncate(signature);
        const auto begin = std::to_address(beginIt) + offset;
        const auto end = std::to_address(endIt);

        if (begin >= end || trunc.size() > static_cast<size_t>(std::distance(begin, end))) {
            return {nullptr};
        }

        const auto context = detail::scan_context::create(trunc, alignment, hints);
        const const_scan_result result = context.scan(begin, end);
        return result.has_result()
            ? const_cast<typename detail::result_type_for<Iter>::underlying_type>(result.get() - offset)
            : nullptr;
    }

    /// Finds all of the matches for the given signature in the input range, and writes the results into the output
    /// range. If there is no space in the output range, the function will exit early. The first element of the returned
    /// pair is an end iterator into the input range at the point in which the pattern search stopped. The second
    /// element of the pair is an end iterator into the output range in which the matched results stop.
    template<scan_alignment alignment = scan_alignment::X1, detail::byte_input_iterator In, std::output_iterator<detail::result_type_for<In>> Out>
    constexpr auto find_all_pattern(
        const In              beginIn,
        const In              endIn,
        const Out             beginOut,
        const Out             endOut,
        const signature_view  signature,
        const scan_hint       hints = scan_hint::none
    ) -> std::pair<In, Out> {
        const auto [offset, trunc] = detail::truncate(signature);
        const auto begin = std::to_address(beginIn) + offset;
        const auto end = std::to_address(endIn);

        auto i = begin;
        auto out = beginOut;

        const auto context = detail::scan_context::create(trunc, alignment, hints);

        while (i < end && out != endOut && trunc.size() <= static_cast<size_t>(std::distance(i, end))) {
            const auto result = context.scan(i, end);
            if (!result.has_result()) {
                i = end;
                break;
            }
            const auto addr = const_cast<typename detail::result_type_for<In>::underlying_type>(result.get() - offset);
            *out++ = addr;
            i = addr + detail::alignment_stride<alignment>;
        }

        return std::make_pair(std::next(beginIn, i - begin), out);
    }

    /// Root implementation of find_all_pattern
    template<scan_alignment alignment = scan_alignment::X1, detail::byte_input_iterator In, std::output_iterator<detail::result_type_for<In>> Out>
    constexpr size_t find_all_pattern(
        const In              beginIn,
        const In              endIn,
        const Out             outIn,
        const signature_view  signature,
        const scan_hint       hints = scan_hint::none
    ) {
        const auto [offset, trunc] = detail::truncate(signature);
        const auto begin = std::to_address(beginIn) + offset;
        const auto end = std::to_address(endIn);

        auto i = begin;
        auto out = outIn;
        size_t matches{};

        const auto context = detail::scan_context::create(trunc, alignment, hints);

        while (begin < end && trunc.size() <= static_cast<size_t>(std::distance(i, end))) {
            const auto result = context.scan(i, end);
            if (!result.has_result()) {
                break;
            }
            const auto addr = const_cast<typename detail::result_type_for<In>::underlying_type>(result.get() - offset);
            *out++ = addr;
            i = addr + detail::alignment_stride<alignment>;
            matches++;
        }

        return matches;
    }

    /// Wrapper around the root find_all_pattern implementation that returns a std::vector of the results
    template<scan_alignment alignment = scan_alignment::X1, detail::byte_input_iterator In>
    constexpr auto find_all_pattern(
        const In             beginIt,
        const In             endIt,
        const signature_view signature
    ) -> std::vector<detail::result_type_for<In>> {
        std::vector<detail::result_type_for<In>> results{};
        find_all_pattern<alignment>(beginIt, endIt, std::back_inserter(results), signature);
        return results;
    }
}

namespace hat::experimental {

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

#include "Scanner.inl"
