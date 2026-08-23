#include <format>
#include <mutex>
#include <random>

#include <benchmark/benchmark.h>
#include <libhat/scanner.hpp>

#include "scanner/scanner.hpp"

static constexpr hat::cstring_view TEST_PATTERN = "01 02 03 04 05 06 07 08 09";

static auto GenRandomBuffer(const size_t size) {
    std::vector<std::byte> buffer(size);
    std::mt19937_64 generator(123);
    for (size_t i = 0; i < buffer.size(); i += 8) {
        uint64_t value = generator();
        std::memcpy(&buffer[i], &value, sizeof(value));
    }
    return buffer;
}

static void BenchmarkScanner(Scanner& scanner, benchmark::State& state) {
    if (!scanner.Supported()) {
        state.SkipWithMessage("Skipping (not natively supported by hardware or OS)");
        return;
    }

    const size_t size = state.range(0);
    const auto buf = GenRandomBuffer(size);

    const std::byte* expected = nullptr;
    const std::byte* result = nullptr;
    scanner.Setup(TEST_PATTERN);
    for (auto _ : state) {
        benchmark::DoNotOptimize(result = scanner.Find(buf));
    }
    if (expected != result) {
        state.SkipWithError("result did not match expected value");
    }
    state.SetBytesProcessed(static_cast<int64_t>(state.iterations() * size));
}

static consteval int64_t operator""_MiB(const unsigned long long int value) {
    return static_cast<int64_t>(value) * int64_t{1024} * int64_t{1024};
}

int main(int argc, char** argv) {
    for (auto& scanner : Scanner::All()) {
        // Exclude any libhat scanner that is non-X1 or has scan hints applied
        if (scanner.Name().starts_with("libhat")) {
            auto split = std::views::split(scanner.Name(), '+');
            const bool x1 = std::ranges::any_of(split, [](auto&& range) {
                return std::ranges::equal(range, std::string_view{"X1"});
            });
            const bool hinted = std::ranges::any_of(split, [](auto&& range) {
                return std::ranges::equal(range, std::string_view{"hint"});
            });
            if (!x1 || hinted) continue;
        }

        auto* bm = ::benchmark::RegisterBenchmark(
            std::string{scanner.Name()},
            [&scanner](benchmark::State& state) {
                BenchmarkScanner(scanner, state);
            }
        );
        bm->Threads(1);
        bm->MinWarmUpTime(2);
        bm->MinTime(4);
        bm->RangeMultiplier(2);
        bm->Range(4_MiB, 256_MiB);
        bm->UseRealTime();
    }
    ::benchmark::Initialize(&argc, argv);
    ::benchmark::RunSpecifiedBenchmarks();
    ::benchmark::Shutdown();
    return 0;
}
