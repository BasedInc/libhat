#include <libhat/scanner.hpp>

#include "scan_context.hpp"

namespace {

    struct anchor_matcher {

        anchor_matcher(const hat::signature_view& signature, const std::span<std::size_t>& anchors, const std::size_t literals) :
            signature_(signature),
            anchors_(anchors),
            literals_(literals) {}

        [[nodiscard]] std::pair<hat::signature_element, std::size_t> top() const {
            return anchor(0);
        }

        [[nodiscard]] bool matches(const std::byte* buffer) const {
            for (size_t i = 1; i < literals_; i++) {
                const auto [element, offset] = anchor(i);
                if (buffer[offset] != element) {
                    std::swap(anchors_[i], anchors_[i - 1]);
                    return false;
                }
            }

            for (size_t i = literals_; i < anchors_.size(); i++) {
                const auto [element, offset] = anchor(i);
                if (buffer[offset] != element) {
                    return false;
                }
            }

            return true;
        }

        [[nodiscard]] std::size_t literals() const {
            return literals_;
        }

    private:
        [[nodiscard]] std::pair<hat::signature_element, std::size_t> anchor(const size_t index) const {
            const auto offset = anchors_[index];
            return {signature_[offset], offset};
        }

        hat::signature_view    signature_;
        std::span<std::size_t> anchors_;
        std::size_t            literals_;
    };

    struct std_context {
        mutable std::vector<std::size_t> anchors{};
        std::size_t literals{};

        explicit std_context(const hat::detail::scan_parameters& params) {
            this->anchors.resize(static_cast<size_t>(
                std::ranges::count_if(params.signature, &hat::signature_element::any)));
            auto it = this->anchors.begin();

            // Add fully masked bytes
            for (size_t i{}; auto e : params.signature) {
                if (e.all()) {
                    *it++ = i;
                }
                i++;
            }
            this->literals = static_cast<std::size_t>(std::distance(this->anchors.begin(), it));

            // Add partially masked bytes
            for (size_t i{}; auto e : params.signature) {
                if (!e.all() && !e.none()) {
                    *it++ = i;
                }
                i++;
            }
        }
    };

    anchor_matcher create_anchor_matcher(const hat::detail::scan_context& context) {
        const auto& std = context.get<std_context>();
        return {context.signature(), std.anchors, std.literals};
    }
}

namespace hat::detail {

    template<scan_alignment alignment>
    static const_scan_result find_pattern_single(const std::byte* begin, const std::byte* end, const scan_context& context) {
        static constexpr auto stride = alignment_stride<alignment>;

        const auto scanBegin = align_up<stride>(begin);
        const auto scanEnd = align_up<stride>(end - context.signature().size() + 1);
        if (scanBegin >= scanEnd) {
            return nullptr;
        }

        const auto matcher = create_anchor_matcher(context);
        for (auto i = scanBegin; i != scanEnd; i += stride) {
            if (matcher.literals() > 0) {
                const auto [anchor, offset] = matcher.top();
                while (i != scanEnd) {
                    if (i[offset] == anchor) {
                        break;
                    }
                    i += stride;
                }
                if (i == scanEnd) LIBHAT_UNLIKELY break;
            }
            if (matcher.matches(i)) {
                return i;
            }
        }

        return nullptr;
    }

    template<>
    const_scan_result find_pattern_single<scan_alignment::X1>(const std::byte* begin, const std::byte* end, const scan_context& context) {
        const auto signature = context.signature();
        const auto scanEnd = end - signature.size() + 1;

        const auto matcher = create_anchor_matcher(context);
        for (auto i = begin; i != scanEnd; i++) {
            // This check should get hoisted out by the optimizer
            if (matcher.literals() > 0) {
                const auto [anchor, offset] = matcher.top();

                // Use std::find to efficiently find the first byte
                #ifndef _MSC_VER
                    i = static_cast<const std::byte*>(
                        std::memchr(i + offset, static_cast<unsigned char>(anchor.value()), static_cast<std::size_t>(scanEnd - i)));
                    if (!i) LIBHAT_UNLIKELY break;
                    i -= offset;
                #elif __cpp_lib_execution >= 201902L
                    i = std::find(std::execution::unseq, i + offset, scanEnd + offset, anchor.value()) - offset;
                    if (i == scanEnd) LIBHAT_UNLIKELY break;
                #else
                    i = std::find(i + offset, scanEnd + offset, anchor.value()) - offset;
                    if (i == scanEnd) LIBHAT_UNLIKELY break;
                #endif
            }
            if (matcher.matches(i)) {
                return i;
            }
        }
        return nullptr;
    }

    template<>
    scan_context create_context<scan_mode::Single>(const scan_parameters& params) {
        switch (params.alignment) {
            case scan_alignment::X1: return {params.signature, &find_pattern_single<scan_alignment::X1>, std::type_identity<std_context>{}, params};
            case scan_alignment::X4: return {params.signature, &find_pattern_single<scan_alignment::X4>, std::type_identity<std_context>{}, params};
            case scan_alignment::X16: return {params.signature, &find_pattern_single<scan_alignment::X16>, std::type_identity<std_context>{}, params};
        }
        LIBHAT_UNREACHABLE();
    }
}
