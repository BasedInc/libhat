#include <mutex>
#include <random>
#include <unordered_map>

#include <benchmark/benchmark.h>
#include <libhat/scanner.hpp>

#include "vendor/UC1.hpp"
#include "vendor/UC2.hpp"

static constexpr std::string_view test_pattern = "01 02 03 04 05 06 07 08 09";

static auto gen_random_buffer(const size_t size) {
    std::vector<std::byte> buffer(size);
    std::default_random_engine generator(123);
    std::uniform_int_distribution<uint64_t> distribution(0, 0xFFFFFFFFFFFFFFFF);
    
    // Fill in 8-byte chunks for speed
    size_t i = 0;
    for (; i + 7 < size; i += 8) {
        uint64_t rnd = distribution(generator);
        std::memcpy(&buffer[i], &rnd, sizeof(uint64_t));
    }
    
    // Fill remaining bytes
    std::uniform_int_distribution<int> byte_dist(0, 255);
    for (; i < size; ++i) {
        buffer[i] = static_cast<std::byte>(byte_dist(generator));
    }
    
    return buffer;
}

static void BM_Throughput_libhat(benchmark::State& state) {
    const size_t size = state.range(0);
    const auto buf = gen_random_buffer(size);
    const auto begin = std::to_address(buf.begin());
    const auto end = std::to_address(buf.end());

    const auto sig = hat::parse_signature(test_pattern).value();
    // Pre-bake the scanner so it's crispy fast
    hat::scanner scanner(sig);
    
    for (auto _ : state) {
        // Use the pre-compiled scanner
        benchmark::DoNotOptimize(scanner.find(begin, end));
    }
    state.SetBytesProcessed(static_cast<int64_t>(state.iterations() * size));
}

static void BM_Throughput_libhat_parallel(benchmark::State& state) {
    const size_t size = state.range(0);
    const auto buf = gen_random_buffer(size);
    const auto begin = std::to_address(buf.begin());
    const auto end = std::to_address(buf.end());

    const auto sig = hat::parse_signature(test_pattern).value();
    // Use the scanner for that sweet sweet speed
    hat::scanner scanner(sig);
    
    for (auto _ : state) {
        // Go parallel, brrr
        benchmark::DoNotOptimize(scanner.find(std::execution::par, begin, end));
    }
    state.SetBytesProcessed(static_cast<int64_t>(state.iterations() * size));
}

static void BM_Throughput_std_search(benchmark::State& state) {
    const size_t size = state.range(0);
    const auto buf = gen_random_buffer(size);
    const auto begin = std::to_address(buf.begin());
    const auto end = std::to_address(buf.end());

    const auto sig = hat::parse_signature(test_pattern).value();
    for (auto _ : state) {
        benchmark::DoNotOptimize(std::search(begin, end, sig.begin(), sig.end()));
    }
    state.SetBytesProcessed(static_cast<int64_t>(state.iterations() * size));
}

static void BM_Throughput_std_find_std_equal(benchmark::State& state) {
    const size_t size = state.range(0);
    const auto buf = gen_random_buffer(size);
    const auto begin = std::to_address(buf.begin());
    const auto end = std::to_address(buf.end());

    // libhat's "Single" implementation uses std::find + std::equal
    const auto sig = hat::parse_signature(test_pattern).value();
    const auto context = hat::detail::scan_context::create<hat::detail::scan_mode::Single>(sig, hat::scan_alignment::X1, hat::scan_hint::none);
    for (auto _ : state) {
        benchmark::DoNotOptimize(context.scan(begin, end));
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

static constexpr int64_t rangeStart = 1 << 22; // 4 MiB
static constexpr int64_t rangeLimit = 1 << 28; // 256 MiB

#define LIBHAT_BENCHMARK(...) BENCHMARK(__VA_ARGS__) \
    ->Threads(1)                                     \
    ->MinWarmUpTime(2)                               \
    ->MinTime(4)                                     \
    ->Range(rangeStart, rangeLimit)                  \
    ->UseRealTime();

LIBHAT_BENCHMARK(BM_Throughput_libhat);
LIBHAT_BENCHMARK(BM_Throughput_libhat_parallel);
LIBHAT_BENCHMARK(BM_Throughput_std_search);
LIBHAT_BENCHMARK(BM_Throughput_std_find_std_equal);
LIBHAT_BENCHMARK(BM_Throughput_UC1);
LIBHAT_BENCHMARK(BM_Throughput_UC2);

BENCHMARK_MAIN();
