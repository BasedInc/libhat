#pragma once

#ifndef LIBHAT_MODULE
    #include <algorithm>
    #include <array>
    #include <cstring>
    #include <execution>
    #include <memory>
    #include <utility>
#endif

#include "concepts.hpp"
#include "defines.hpp"
#include "export.hpp"
#include "process.hpp"
#include "signature.hpp"

LIBHAT_EXPORT namespace hat {

    template<typename T> requires (sizeof(T) == 1)
    class scan_result_base {
    public:
        using underlying_type = T*;

        constexpr scan_result_base() noexcept : result(nullptr) {}
        explicit(false) constexpr scan_result_base(std::nullptr_t) noexcept : result(nullptr) {}
        explicit(false) constexpr scan_result_base(const underlying_type result) noexcept : result(result) {}

        /// Reads an integer of the specified type located at an offset from the signature result. If there is no
        /// result, the behavior is undefined.
        template<std::integral Int>
        [[nodiscard]] constexpr Int read(const std::size_t offset) const noexcept {
            if LIBHAT_IF_CONSTEVAL {
                constexpr std::size_t N = sizeof(Int);
                return std::bit_cast<Int>([=, this]<std::size_t... Index>(std::index_sequence<Index...>) {
                    return std::array<T, N>{(this->result + offset)[Index]...};
                }(std::make_index_sequence<N>{}));
            } else {
                Int value;
                std::memcpy(&value, this->result + offset, sizeof(Int));
                return value;
            }
        }

        /// Reads an integer of the specified type, which represents an index into an array with the given element type.
        /// If there is no result, the behavior is undefined.
        template<std::integral Int, typename ArrayType>
        [[nodiscard]] constexpr std::size_t index(const std::size_t offset) const noexcept {
            return static_cast<std::size_t>(read<Int>(offset)) / sizeof(ArrayType);
        }

        /// Resolve the relative address located at an offset from the signature result. If there is no result, nullptr
        /// is returned instead. The "offset" parameter is the number of bytes after the result's match that the relative
        /// address is located. For example:
        ///
        ///        | result matches here
        ///        |        | relative address located at +3 (offset)
        ///        v        v
        ///   0x0: 48 8D 05 BE 53 23 01    lea  rax, [rip+0x12353be]
        ///   0x7: <next instruction>
        ///
        /// The "remaining" parameter is the number of bytes after the relative address that the next instruction
        /// begins. In the majority of cases, this parameter can be left as 0. However, consider the following example:
        ///
        ///        | result matches here
        ///        |     | relative address located at +2 (offset)
        ///        |     |           | end of relative address
        ///        v     v           v
        ///   0x0: 83 3D BE 53 23 01 00    cmp    DWORD PTR [rip+0x12353be],0x0
        ///   0x7: <next instruction>
        ///
        /// The "0x0" operand comes after the relative address. The absolute address referred to by the RIP relative
        /// address in this case is 0x12353BE + 0x7 = 0x12353C5. Simply using rel(2) would yield an incorrect result of
        /// 0x12353C4. In this case, rel(2, 1) would yield the expected 0x12353C5.
        [[nodiscard]] constexpr underlying_type rel(std::size_t offset, std::size_t remaining = 0) const noexcept {
            if (!this->has_result()) LIBHAT_UNLIKELY {
                return nullptr;
            }
            using rel32_t = std::int32_t;
            return this->result + this->read<rel32_t>(offset) + offset + sizeof(rel32_t) + remaining;
        }

        [[nodiscard]] constexpr bool has_result() const noexcept {
            return this->result != nullptr;
        }

        [[nodiscard]] constexpr underlying_type operator*() const noexcept {
            return this->result;
        }

        [[nodiscard]] constexpr underlying_type get() const noexcept {
            return this->result;
        }

        [[nodiscard]] constexpr auto operator<=>(const scan_result_base&) const noexcept = default;

    private:
        underlying_type result;
    };

    using scan_result = scan_result_base<std::byte>;
    using const_scan_result = scan_result_base<const std::byte>;

    enum class scan_alignment : std::uint8_t {
        X1 = 1,
        X4 = 4,
        X16 = 16
    };

    enum class scan_hint : std::uint64_t {
        none    = 0,      // no hints
        x86_64  = 1 << 0, // The data being scanned is x86_64 machine code
        pair0   = 1 << 1, // Only utilize byte pair based scanning if the signature starts with a byte pair
        aarch64 = 1 << 2, // The data being scanned is AArch64 machine code
    };

    constexpr scan_hint operator|(scan_hint lhs, scan_hint rhs) {
        using U = std::underlying_type_t<scan_hint>;
        return static_cast<scan_hint>(static_cast<U>(lhs) | static_cast<U>(rhs));
    }

    constexpr scan_hint operator&(scan_hint lhs, scan_hint rhs) {
        using U = std::underlying_type_t<scan_hint>;
        return static_cast<scan_hint>(static_cast<U>(lhs) & static_cast<U>(rhs));
    }

    constexpr scan_hint& operator|=(scan_hint& lhs, const scan_hint rhs) {
        lhs = lhs | rhs;
        return lhs;
    }

    constexpr scan_hint& operator&=(scan_hint& lhs, const scan_hint rhs) {
        lhs = lhs & rhs;
        return lhs;
    }
}

namespace hat::detail {

    class scan_context;

    using scan_function_t = const_scan_result(*)(const std::byte* begin, const std::byte* end, const scan_context& context);

    enum class scan_mode {
        Auto,   // Picks a mode at runtime
        Search, // std::search
        Single, // std::find + std::equal
        SSE,    // x86/x64 SSE 4.1
        AVX2,   // x86/x64 AVX2
        AVX512, // x64 AVX512
        Neon,   // ARMv7+ Neon
    };

    class scan_context {
        static constexpr size_t impl_buffer_size = 48;
        static constexpr size_t impl_buffer_align = alignof(std::max_align_t);
    public:
        scan_context(const scan_context&) = delete;
        scan_context(scan_context&&) = delete;
        scan_context& operator=(const scan_context&) = delete;
        scan_context& operator=(scan_context&&) = delete;

        constexpr ~scan_context() {
            if (impl_deleter_) {
                impl_deleter_(impl_buffer_.data());
                impl_deleter_ = nullptr;
            }
        }

        [[nodiscard]] constexpr signature_view signature() const {
            return signature_;
        }

        template<typename T, typename... Args>
        void emplace(Args&&... args);

        template<typename T>
        [[nodiscard]] const T& get() const;

        [[nodiscard]] constexpr const_scan_result scan(const std::byte* begin, const std::byte* end) const {
            if (signature_.size() > static_cast<std::size_t>(std::distance(begin, end))) LIBHAT_UNLIKELY {
                return {};
            }
            return scanner_(begin, end, *this);
        }

        template<scan_mode mode = scan_mode::Auto>
        static constexpr scan_context create(signature_view signature, scan_alignment alignment, scan_hint hints);

        template<typename T, typename... Args>
        scan_context(const signature_view signature, const scan_function_t scanner, std::type_identity<T>, Args&&... args)
            : signature_(signature), scanner_(scanner)
        {
            emplace<T>(std::forward<Args>(args)...);
        }

        constexpr scan_context(const signature_view signature, const scan_function_t scanner)
            : signature_(signature), scanner_(scanner) {}

    private:
        signature_view signature_{};
        scan_function_t scanner_{};
        void(*impl_deleter_)(const void*){};
        alignas(impl_buffer_align) std::array<std::byte, impl_buffer_size> impl_buffer_;
    };

    LIBHAT_FORCEINLINE constexpr auto to_stride(const scan_alignment alignment) {
        return static_cast<std::underlying_type_t<scan_alignment>>(alignment);
    }

    template<scan_alignment alignment>
    inline constexpr auto alignment_stride = to_stride(alignment);

    template<std::integral type, scan_alignment alignment>
    LIBHAT_FORCEINLINE consteval auto create_alignment_mask() {
        type mask{};
        for (std::size_t i = 0; i < sizeof(type) * 8; i += alignment_stride<alignment>) {
            mask |= static_cast<type>(type(1) << i);
        }
        return mask;
    }

    template<std::size_t alignment>
    LIBHAT_FORCEINLINE const std::byte* align_up(const std::byte* ptr) {
        const std::uintptr_t mod = reinterpret_cast<std::uintptr_t>(ptr) % alignment;
        ptr += mod ? alignment - mod : 0;
        return std::assume_aligned<alignment>(ptr);
    }

    struct scan_parameters {
        signature_view signature{};
        scan_alignment alignment{};
        scan_hint      hints{};
    };

    template<scan_mode>
    scan_context create_context(const scan_parameters&);

    template<>
    scan_context create_context<scan_mode::Auto>(const scan_parameters&);

    template<scan_alignment alignment>
    const_scan_result find_pattern_search(const std::byte* begin, const std::byte* end, const scan_context& context) {
        static constexpr auto stride = alignment_stride<alignment>;

        const auto sig = context.signature();
        const auto scanBegin = align_up<stride>(begin);
        const auto scanEnd = align_up<stride>(end - sig.size() + 1);

        // intentionally kept simple/inefficient since this will only be used for small buffers
        for (auto i = scanBegin; i != scanEnd; i += stride) {
            if (std::equal(sig.begin(), sig.end(), i)) {
                return i;
            }
        }
        return nullptr;
    }

    template<>
    constexpr const_scan_result find_pattern_search<scan_alignment::X1>(const std::byte* begin, const std::byte* end, const scan_context& context) {
        const auto sig = context.signature();
        const auto it = std::search(begin, end, sig.begin(), sig.end());
        return it != end ? it : nullptr;
    }

    template<>
    inline scan_context create_context<scan_mode::Search>(const scan_parameters& params) {
        switch (params.alignment) {
            case scan_alignment::X1: return {params.signature, &find_pattern_search<scan_alignment::X1>};
            case scan_alignment::X4: return {params.signature, &find_pattern_search<scan_alignment::X4>};
            case scan_alignment::X16: return {params.signature, &find_pattern_search<scan_alignment::X16>};
        }
        LIBHAT_UNREACHABLE();
    }

    template<byte_input_iterator T>
    using result_type_for = std::conditional_t<std::is_const_v<std::remove_reference_t<std::iter_reference_t<T>>>,
        const_scan_result, scan_result>;

    template<scan_mode mode>
    constexpr scan_context scan_context::create(const signature_view signature, const scan_alignment alignment, const scan_hint hints) {
        const scan_parameters params{
            .signature = signature,
            .alignment = alignment,
            .hints = hints,
        };
        if LIBHAT_IF_CONSTEVAL {
            if (alignment != scan_alignment::X1) {
                std::abort();
            }
            return {signature, &find_pattern_search<scan_alignment::X1>};
        } else {
            return create_context<mode>(params);
        }
    }
}

LIBHAT_EXPORT namespace hat {

    /// Root implementation of find_pattern
    template<detail::byte_input_iterator Iter>
    [[nodiscard]] constexpr auto find_pattern(
        const Iter            beginIt,
        const Iter            endIt,
        const signature_view  signature,
        const scan_alignment  alignment = scan_alignment::X1,
        const scan_hint       hints = scan_hint::none
    ) noexcept -> detail::result_type_for<Iter> {
        const auto context = detail::scan_context::create(signature, alignment, hints);
        const auto begin = std::to_address(beginIt);
        const auto end = std::to_address(endIt);

        const auto result = context.scan(begin, end);
        return result.has_result()
            ? const_cast<typename detail::result_type_for<Iter>::underlying_type>(result.get())
            : nullptr;
    }

    /// Range overload of find_pattern
    template<detail::byte_input_range Range>
    [[nodiscard]] constexpr auto find_pattern(
        Range&& range,
        const signature_view  signature,
        const scan_alignment  alignment = scan_alignment::X1,
        const scan_hint       hints = scan_hint::none
    ) noexcept -> detail::result_type_for<std::ranges::iterator_t<Range>> {
        return find_pattern(std::ranges::begin(range), std::ranges::end(range), signature, alignment, hints);
    }

    /// Perform a signature scan on a specific section of the process module or a specified module
    [[nodiscard]] inline scan_result find_pattern(
        const signature_view   signature,
        const std::string_view section,
        const process::module& mod = process::get_process_module(),
        const scan_alignment   alignment = scan_alignment::X1,
        const scan_hint        hints = scan_hint::none
    ) noexcept {
        const auto data = mod.get_section_data(section);
        return find_pattern(data.begin(), data.end(), signature, alignment, hints);
    }

    /// Finds all of the matches for the given signature in the input range, and writes the results into the output
    /// range. If there is no space in the output range, the function will exit early. The first element of the returned
    /// pair is an end iterator into the input range at the point in which the pattern search stopped. The second
    /// element of the pair is an end iterator into the output range in which the matched results stop.
    template<detail::byte_input_iterator In, std::output_iterator<detail::result_type_for<In>> Out>
    [[nodiscard]] constexpr auto find_all_pattern(
        const In              beginIn,
        const In              endIn,
        const Out             beginOut,
        const Out             endOut,
        const signature_view  signature,
        const scan_alignment  alignment = scan_alignment::X1,
        const scan_hint       hints = scan_hint::none
    ) noexcept -> std::pair<In, Out> {
        const auto context = detail::scan_context::create(signature, alignment, hints);
        const auto begin = std::to_address(beginIn);
        const auto end = std::to_address(endIn);

        auto i = begin;
        auto out = beginOut;

        while (i < end && out != endOut) {
            const auto result = context.scan(i, end);
            if (!result.has_result()) {
                i = end;
                break;
            }
            *out++ = const_cast<typename detail::result_type_for<In>::underlying_type>(result.get());
            i = result.get() + detail::to_stride(alignment);
        }

        return std::make_pair(std::next(beginIn, i - begin), out);
    }

    template<detail::byte_input_range In, std::ranges::output_range<detail::result_type_for<std::ranges::iterator_t<In>>> Out>
    [[nodiscard]] constexpr auto find_all_pattern(
        In&&                  rangeIn,
        Out&&                 rangeOut,
        const signature_view  signature,
        const scan_alignment  alignment = scan_alignment::X1,
        const scan_hint       hints = scan_hint::none
    ) noexcept -> std::pair<std::ranges::iterator_t<In>, std::ranges::iterator_t<Out>> {
        return find_all_pattern(
            std::ranges::begin(rangeIn), std::ranges::end(rangeIn),
            std::ranges::begin(rangeOut), std::ranges::end(rangeOut),
            signature,
            alignment,
            hints
        );
    }

    /// Finds all of the matches for the given signature in the input range, and writes the results into the output
    /// iterator. The entire input range will be searched and all results written to the output range. The number of
    /// matches found is returned.
    template<detail::byte_input_iterator In, std::output_iterator<detail::result_type_for<In>> Out>
    constexpr std::size_t find_all_pattern(
        const In              beginIn,
        const In              endIn,
        const Out             beginOut,
        const signature_view  signature,
        const scan_alignment  alignment = scan_alignment::X1,
        const scan_hint       hints = scan_hint::none
    ) noexcept {
        const auto context = detail::scan_context::create(signature, alignment, hints);
        const auto begin = std::to_address(beginIn);
        const auto end = std::to_address(endIn);

        auto i = begin;
        auto out = beginOut;
        std::size_t matches{};

        while (i < end) {
            const auto result = context.scan(i, end);
            if (!result.has_result()) {
                break;
            }
            *out++ = const_cast<typename detail::result_type_for<In>::underlying_type>(result.get());
            i = result.get() + detail::to_stride(alignment);
            matches++;
        }

        return matches;
    }

    template<detail::byte_input_range In, std::output_iterator<detail::result_type_for<std::ranges::iterator_t<In>>> Out>
    constexpr std::size_t find_all_pattern(
        In&&                  rangeIn,
        const Out             beginOut,
        const signature_view  signature,
        const scan_alignment  alignment = scan_alignment::X1,
        const scan_hint       hints = scan_hint::none
    ) noexcept {
        return find_all_pattern(std::ranges::begin(rangeIn), std::ranges::end(rangeIn), beginOut, signature, alignment, hints);
    }

    /// Wrapper around the root find_all_pattern implementation that returns a std::vector of the results
    template<detail::byte_input_iterator In>
    [[nodiscard]] constexpr auto find_all_pattern(
        const In             beginIt,
        const In             endIt,
        const signature_view signature,
        const scan_alignment alignment = scan_alignment::X1,
        const scan_hint      hints = scan_hint::none
    ) noexcept -> std::vector<detail::result_type_for<In>> {
        std::vector<detail::result_type_for<In>> results{};
        find_all_pattern(beginIt, endIt, std::back_inserter(results), signature, alignment, hints);
        return results;
    }

    template<detail::byte_input_range In>
    constexpr auto find_all_pattern(
        In&&                  rangeIn,
        const signature_view  signature,
        const scan_alignment  alignment = scan_alignment::X1,
        const scan_hint       hints = scan_hint::none
    ) noexcept -> std::vector<detail::result_type_for<std::ranges::iterator_t<In>>> {
        return find_all_pattern(std::ranges::begin(rangeIn), std::ranges::end(rangeIn), signature, alignment, hints);
    }
}

LIBHAT_EXPORT namespace hat::experimental {

    enum class compiler_type {
        MSVC,
        GNU
    };

    /// Gets the VTable address for a class by its mangled name
    template<compiler_type compiler>
    scan_result find_vtable(
        const std::string&     className,
        const process::module& mod = process::get_process_module()
    );
}
