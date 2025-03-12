#include <gtest/gtest.h>
#include <libhat/scanner.hpp>
#include <format>

template<hat::detail::scan_mode Mode, size_t SignatureSize, size_t MaxBufferSize>
struct FindPatternParameters {
    static_assert(SignatureSize <= MaxBufferSize);
    static_assert(SignatureSize < 0xFF);

    static constexpr auto mode = Mode;
    static constexpr auto signature_size = SignatureSize;
    static constexpr auto max_buffer_size = MaxBufferSize;
};

template<typename T>
class FindPatternTest;

template<typename T>
concept FindPatternTestCallback = std::invocable<T,
    hat::const_scan_result, /* actual */
    const std::byte*        /* expected */
>;

template<hat::detail::scan_mode Mode, size_t SignatureSize, size_t MaxBufferSize>
class FindPatternTest<FindPatternParameters<Mode, SignatureSize, MaxBufferSize>> : public ::testing::Test {
protected:
    void run_cases(
        const hat::scan_alignment alignment,
        FindPatternTestCallback auto&& callback
    ) {
        hat::fixed_signature<SignatureSize> sig{};
        for (int i{}; i < SignatureSize; i++) {
            sig[i] = static_cast<std::byte>(i + 1);
        }
        hat::fixed_signature<SignatureSize> sigWildcard = sig;
        if constexpr (SignatureSize >= 2) {
            sigWildcard[1] = std::nullopt;
        }

        const auto contextA = hat::detail::scan_context::create<Mode>(sig, alignment, hat::scan_hint::none);
        const auto contextB = hat::detail::scan_context::create<Mode>(sigWildcard, alignment, hat::scan_hint::none);

        for (size_t size = SignatureSize; size != MaxBufferSize; size++) {
            for (size_t offset{}; offset != size - SignatureSize + 1; offset++) {
                std::vector code(size, std::byte{0x00});
                auto* const begin = std::to_address(code.begin());
                auto* const end = std::to_address(code.end());

                ASSERT_FALSE(contextA.scan(begin, end).has_result());
                ASSERT_FALSE(contextB.scan(begin, end).has_result());

                std::ranges::copy(sig | std::views::transform(&hat::signature_element::value),
                    begin + offset);

                callback(contextA.scan(begin, end), begin + offset);
                callback(contextB.scan(begin, end), begin + offset);
            }
        }
    }
};

using FindPatternTestTypes = ::testing::Types<
#if defined(LIBHAT_X86_64) || defined(LIBHAT_X86)
    FindPatternParameters<hat::detail::scan_mode::SSE, 1, 256>,
    FindPatternParameters<hat::detail::scan_mode::SSE, 3, 256>,
    FindPatternParameters<hat::detail::scan_mode::SSE, 8, 256>,
    FindPatternParameters<hat::detail::scan_mode::SSE, 16, 256>,
    FindPatternParameters<hat::detail::scan_mode::SSE, 32, 256>,
    FindPatternParameters<hat::detail::scan_mode::SSE, 64, 256>,

    FindPatternParameters<hat::detail::scan_mode::AVX2, 1, 256>,
    FindPatternParameters<hat::detail::scan_mode::AVX2, 3, 256>,
    FindPatternParameters<hat::detail::scan_mode::AVX2, 8, 256>,
    FindPatternParameters<hat::detail::scan_mode::AVX2, 16, 256>,
    FindPatternParameters<hat::detail::scan_mode::AVX2, 32, 256>,
    FindPatternParameters<hat::detail::scan_mode::AVX2, 64, 256>,
#endif
#ifdef LIBHAT_X86_64
    FindPatternParameters<hat::detail::scan_mode::AVX512, 1, 256>,
    FindPatternParameters<hat::detail::scan_mode::AVX512, 3, 256>,
    FindPatternParameters<hat::detail::scan_mode::AVX512, 8, 256>,
    FindPatternParameters<hat::detail::scan_mode::AVX512, 16, 256>,
    FindPatternParameters<hat::detail::scan_mode::AVX512, 32, 256>,
    FindPatternParameters<hat::detail::scan_mode::AVX512, 64, 256>,
#endif
    FindPatternParameters<hat::detail::scan_mode::Single, 1, 256>,
    FindPatternParameters<hat::detail::scan_mode::Single, 3, 256>,
    FindPatternParameters<hat::detail::scan_mode::Single, 8, 256>,
    FindPatternParameters<hat::detail::scan_mode::Single, 16, 256>,
    FindPatternParameters<hat::detail::scan_mode::Single, 32, 256>,
    FindPatternParameters<hat::detail::scan_mode::Single, 64, 256>
>;

class FindPatternTestNameGenerator {
public:
    template<typename T>
    static std::string GetName(int) {
        return std::format("{}/{}/{}", getModeName<T::mode>(), T::signature_size, T::max_buffer_size);
    }

private:
    template<hat::detail::scan_mode Mode>
    static consteval std::string_view getModeName() {
        if constexpr (Mode == hat::detail::scan_mode::Single) return "Single";
        else if constexpr (Mode == hat::detail::scan_mode::SSE) return "SSE";
        else if constexpr (Mode == hat::detail::scan_mode::AVX2) return "AVX2";
        else if constexpr (Mode == hat::detail::scan_mode::AVX512) return "AVX512";
        else static_assert(sizeof(Mode) == 0);
    }
};

TYPED_TEST_SUITE(FindPatternTest, FindPatternTestTypes, FindPatternTestNameGenerator);

TYPED_TEST(FindPatternTest, ScanX1) {
    this->run_cases(hat::scan_alignment::X1, [](auto result, auto expected) {
        ASSERT_TRUE(result.has_result());
        ASSERT_EQ(result.get(), expected);
    });
}

TYPED_TEST(FindPatternTest, ScanX16) {
    this->run_cases(hat::scan_alignment::X16, [](auto result, auto expected) {
        if (std::bit_cast<uintptr_t>(expected) % 16 == 0) {
            ASSERT_TRUE(result.has_result());
            ASSERT_EQ(result.get(), expected);
        } else {
            ASSERT_FALSE(result.has_result());
        }
    });
}
