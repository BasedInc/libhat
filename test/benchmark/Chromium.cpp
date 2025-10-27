#include <fstream>

#include <benchmark/benchmark.h>
#include <libhat/scanner.hpp>

#include <format>

#define WIDE_STR_(x) L ## #x
#define WIDE_STR(x) WIDE_STR_(x)

static constexpr auto DllMainSignature = hat::compile_signature<"48 89 5C 24 08 48 89 74 24 10 57 48 83 EC ?? 49 8B F8">();

static std::span<const std::byte> get_file_data() {
    static std::vector<std::byte> data = []{
        std::ifstream file(WIDE_STR(CHROME_DLL_PATH), std::ios::binary);
        if (!file.is_open()) {
            std::terminate();
        }

        file.seekg(0, std::ios::end);
        const std::streampos fileSize = file.tellg();
        file.seekg(0, std::ios::beg);

        std::vector<std::byte> contents(static_cast<size_t>(fileSize));
        file.read(reinterpret_cast<char*>(contents.data()), fileSize);
        return contents;
    }();
    return data;
}

static void BM_find(benchmark::State& state) {
    const auto buf = get_file_data();

    hat::const_scan_result result;
    for (auto _ : state) {
        benchmark::DoNotOptimize(result = hat::find_pattern(buf, DllMainSignature));
    }
    if (!result.has_result()) {
        std::terminate();
    }
    state.SetBytesProcessed(state.iterations() * (result.get() - buf.data()));
}

static void BM_find_align(benchmark::State& state) {
    const auto buf = get_file_data();

    hat::const_scan_result result;
    for (auto _ : state) {
        benchmark::DoNotOptimize(result = hat::find_pattern(buf, DllMainSignature, hat::scan_alignment::X16));
    }
    if (!result.has_result()) {
        std::terminate();
    }
    state.SetBytesProcessed(state.iterations() * (result.get() - buf.data()));
}

static void BM_find_hint(benchmark::State& state) {
    const auto buf = get_file_data();

    hat::const_scan_result result;
    for (auto _ : state) {
        benchmark::DoNotOptimize(result = hat::find_pattern(buf, DllMainSignature, hat::scan_alignment::X1, hat::scan_hint::x86_64));
    }
    if (!result.has_result()) {
        std::terminate();
    }
    state.SetBytesProcessed(state.iterations() * (result.get() - buf.data()));
}

static void BM_find_align_hint(benchmark::State& state) {
    const auto buf = get_file_data();

    hat::const_scan_result result;
    for (auto _ : state) {
        benchmark::DoNotOptimize(result = hat::find_pattern(buf, DllMainSignature, hat::scan_alignment::X16, hat::scan_hint::x86_64));
    }
    if (!result.has_result()) {
        std::terminate();
    }
    state.SetBytesProcessed(state.iterations() * (result.get() - buf.data()));
}

#define LIBHAT_BENCHMARK(...) BENCHMARK(__VA_ARGS__) \
    ->Threads(1)                                     \
    ->MinWarmUpTime(2)                               \
    ->MinTime(4)                                     \
    ->UseRealTime();

LIBHAT_BENCHMARK(BM_find);
LIBHAT_BENCHMARK(BM_find_align);
LIBHAT_BENCHMARK(BM_find_hint);
LIBHAT_BENCHMARK(BM_find_align_hint);

BENCHMARK_MAIN();
