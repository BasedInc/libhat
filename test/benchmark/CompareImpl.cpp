#include <random>

#include <benchmark/benchmark.h>
#include <libhat/scanner.hpp>

static constexpr std::string_view test_pattern = "01 02 03 04 05 06 07 08 09";

static auto gen_random_buffer(const size_t size) {
    std::vector<std::byte> buffer(size);
    std::default_random_engine generator(123);
    std::uniform_int_distribution<uint64_t> distribution(0, 0xFFFFFFFFFFFFFFFF);
    for (size_t i = 0; i < buffer.size(); i += 8) {
        uint64_t value = distribution(generator);
        std::memcpy(&buffer[i], &value, sizeof(value));
    }
    return buffer;
}

template<hat::detail::scan_mode Mode>
static void BM_Throughput(benchmark::State& state) {
    const size_t size = state.range(0);
    const auto buf = gen_random_buffer(size);
    const auto begin = std::to_address(buf.begin());
    const auto end = std::to_address(buf.end());

    const auto sig = hat::parse_signature(test_pattern).value();
    const auto ctx = hat::detail::scan_context::create<Mode>(sig, hat::scan_alignment::X1, hat::scan_hint::none);
    for (auto _ : state) {
        benchmark::DoNotOptimize(ctx.scan(begin, end));
    }
    state.SetBytesProcessed(static_cast<int64_t>(state.iterations() * size));
}

static constexpr int64_t rangeStart = 1 << 22; // 4 MiB
static constexpr int64_t rangeLimit = 1 << 28; // 256 MiB

#define LIBHAT_BENCHMARK(...) BENCHMARK(__VA_ARGS__) \
    ->Threads(1)                                     \
    ->Range(rangeStart, rangeLimit)                  \
    ->UseRealTime();

LIBHAT_BENCHMARK(BM_Throughput<hat::detail::scan_mode::Single>);
#if defined(LIBHAT_AARCH64) || defined(LIBHAT_ARM)
LIBHAT_BENCHMARK(BM_Throughput<hat::detail::scan_mode::Neon>);
#endif

BENCHMARK_MAIN();
