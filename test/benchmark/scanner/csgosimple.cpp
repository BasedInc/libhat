#include "scanner.hpp"

namespace {

    // A slightly modified version of MarkHC's signature scanner in CSGOSimple
    // https://github.com/spirthack/CSGOSimple/blob/c37d4bc36efe99c621eb288fd34299c1692ee1dd/CSGOSimple/helpers/utils.cpp#L226
    struct CSGOSimpleScanner : Scanner {

        std::string_view Name() override {
            return "csgosimple";
        }

        void Setup(const hat::cstring_view signature) override {
            bytes_.clear();
            const auto start = signature.data();
            const auto end = start + signature.size();

            for (auto current = start; current < end; ++current) {
                if (*current == '?') {
                    ++current;
                    if (*current == '?')
                        ++current;
                    bytes_.push_back(-1);
                } else {
                    bytes_.push_back(strtoul(current, const_cast<char**>(&current), 16));
                }
            }
        }

        const std::byte* Find(std::span<const std::byte> buffer) override {
            const auto* begin = std::to_address(buffer.begin());
            const auto* end = std::to_address(buffer.end());
            const auto sizeOfImage = static_cast<size_t>(std::distance(begin, end));
            const auto scanBytes = std::to_address(begin);

            const auto s = bytes_.size();
            const auto d = bytes_.data();

            for (auto i = 0ul; i < sizeOfImage - s; ++i) {
                bool found = true;
                for (auto j = 0ul; j < s; ++j) {
                    if (scanBytes[i + j] != static_cast<std::byte>(d[j]) && d[j] != -1) {
                        found = false;
                        break;
                    }
                }
                if (found) {
                    return &scanBytes[i];
                }
            }
            return nullptr;
        }

    private:
        std::vector<int> bytes_;
    };

    REGISTER_SCANNER(CSGOSimpleScanner);
}
