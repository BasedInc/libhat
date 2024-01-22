#include <mutex>
#include <random>
#include <unordered_map>

#include <benchmark/benchmark.h>
#include <libhat/Scanner.hpp>

#include "vendor/UC1.hpp"
#include "vendor/UC2.hpp"

static constexpr std::string_view test_pattern = "01 02 03 04 05 06 07 08 09";

static auto gen_random_buffer(const size_t size) {
    std::vector<std::byte> buffer(size);
    std::default_random_engine generator(123);
    std::uniform_int_distribution<uint64_t> distribution(0, 0xFFFFFFFFFFFFFFFF);
    for (size_t i = 0; i < buffer.size(); i += 8) {
        *reinterpret_cast<uint64_t*>(&buffer[i]) = distribution(generator);
    }
    return buffer;
}

static void BM_Throughput_Libhat(benchmark::State& state) {
    const size_t size = state.range(0);
    const auto buf = gen_random_buffer(size);
    const auto begin = std::to_address(buf.begin());
    const auto end = std::to_address(buf.end());

    const auto sig = hat::parse_signature(test_pattern).value();
    for (auto _ : state) {
        benchmark::DoNotOptimize(hat::find_pattern(begin, end, sig));
    }
    state.SetBytesProcessed(static_cast<int64_t>(state.iterations() * size));
}

static void BM_Throughput_UC1(benchmark::State& state) {
    const size_t size = state.range(0);
    const auto buf = gen_random_buffer(size);
    const auto begin = std::to_address(buf.begin());
    const auto end = std::to_address(buf.end());

    const auto sig = UC1::pattern_to_byte(test_pattern);
    for (auto _ : state) {
        benchmark::DoNotOptimize(UC1::PatternScan(begin, end, sig));
    }
    state.SetBytesProcessed(static_cast<int64_t>(state.iterations() * size));
}

static void BM_Throughput_UC2(benchmark::State& state) {
    const size_t size = state.range(0);
    const auto buf = gen_random_buffer(size);
    const auto begin = std::to_address(buf.begin());
    const auto end = std::to_address(buf.end());

    for (auto _ : state) {
        benchmark::DoNotOptimize(UC2::findPattern(begin, end, test_pattern.data()));
    }
    state.SetBytesProcessed(static_cast<int64_t>(state.iterations() * size));
}

static int64_t rangeStart = 1 << 22; // 4 MiB
static int64_t rangeLimit = 1 << 28; // 256 MiB

BENCHMARK(BM_Throughput_Libhat)->Threads(1)->MinWarmUpTime(1)->MinTime(2)->Range(rangeStart, rangeLimit)->UseRealTime();
BENCHMARK(BM_Throughput_UC1)->Threads(1)->MinWarmUpTime(1)->MinTime(2)->Range(rangeStart, rangeLimit)->UseRealTime();
BENCHMARK(BM_Throughput_UC2)->Threads(1)->MinWarmUpTime(1)->MinTime(2)->Range(rangeStart, rangeLimit)->UseRealTime();

BENCHMARK_MAIN();
