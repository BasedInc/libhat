#include "scanner.hpp"

#include <libhat/scanner.hpp>

#include <format>

namespace {

    using hat::scan_alignment;
    using hat::scan_hint;
    using hat::detail::scan_mode;

    std::string_view ModeToName(const scan_mode mode) {
#define MODE_CASE(Mode) case scan_mode::Mode: return #Mode;
            switch (mode) {
                MODE_CASE(Auto);
                MODE_CASE(Search);
                MODE_CASE(Single);
                MODE_CASE(SSE);
                MODE_CASE(AVX2);
                MODE_CASE(AVX512);
                MODE_CASE(Neon);
            }
#undef MODE_CASE
        LIBHAT_UNREACHABLE();
    }

    template<scan_mode Mode, scan_alignment Alignment, scan_hint Hints>
    struct LibhatScanner : Scanner {

        std::string_view Name() override {
            static std::string name = std::format("libhat+{}+X{}{}",
                ModeToName(Mode),
                std::to_string(static_cast<std::uint8_t>(Alignment)),
                static_cast<bool>(Hints) ? "+hint" : "");
            return name;
        }

        bool Supported() override {
            return hat::detail::is_supported(Mode);
        }

        void Setup(const hat::cstring_view signature) override {
            signature_ = hat::parse_signature(signature).value();
        }

        const std::byte* Find(const std::span<const std::byte> buffer) override {
            const auto context = hat::detail::scan_context::create<Mode>(
                signature_, Alignment, Hints);

            const auto* begin = std::to_address(buffer.begin());
            const auto* end = std::to_address(buffer.end());
            return context.scan(begin, end).get();
        }

    private:
        hat::signature signature_;
    };

#define REGISTER_LIBHAT_SCANNER(mode)                                                         \
    REGISTER_SCANNER(LibhatScanner<scan_mode::mode, scan_alignment::X1, scan_hint::none>);    \
    REGISTER_SCANNER(LibhatScanner<scan_mode::mode, scan_alignment::X4, scan_hint::none>);    \
    REGISTER_SCANNER(LibhatScanner<scan_mode::mode, scan_alignment::X16, scan_hint::none>);   \
    REGISTER_SCANNER(LibhatScanner<scan_mode::mode, scan_alignment::X1, scan_hint::x86_64>);  \
    REGISTER_SCANNER(LibhatScanner<scan_mode::mode, scan_alignment::X4, scan_hint::x86_64>);  \
    REGISTER_SCANNER(LibhatScanner<scan_mode::mode, scan_alignment::X16, scan_hint::x86_64>);

    REGISTER_LIBHAT_SCANNER(Auto);
    REGISTER_LIBHAT_SCANNER(Search);
    REGISTER_LIBHAT_SCANNER(Single);
    REGISTER_LIBHAT_SCANNER(SSE);
    REGISTER_LIBHAT_SCANNER(AVX2);
    REGISTER_LIBHAT_SCANNER(AVX512);
    REGISTER_LIBHAT_SCANNER(Neon);
}
