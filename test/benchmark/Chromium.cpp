#include <fstream>
#include <filesystem>

#include <benchmark/benchmark.h>
#include <libhat/scanner.hpp>

#include <format>

#include "scanner/scanner.hpp"

#define WIDE_STR_(x) L ## #x
#define WIDE_STR(x) WIDE_STR_(x)

static constexpr hat::cstring_view DLL_MAIN_PATTERN = "48 89 5C 24 08 48 89 74 24 10 57 48 83 EC ?? 49 8B F8 8B DA";

static std::span<const std::byte> GetFileData() {
    static std::vector<std::byte> data = []{
        const std::filesystem::path path{WIDE_STR(CHROME_DLL_PATH)};
        std::ifstream file(path, std::ios::binary);
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

static void BenchmarkScanner(Scanner& scanner, benchmark::State& state) {
    if (!scanner.Supported()) {
        state.SkipWithMessage("Skipping (not natively supported by hardware or OS)");
        return;
    }

    const auto buf = GetFileData();

    const std::byte* result = nullptr;
    scanner.Setup(DLL_MAIN_PATTERN);
    for (auto _ : state) {
        benchmark::DoNotOptimize(result = scanner.Find(buf));
    }
    if (!result) {
        state.SkipWithError("Scanner failed to find match in buffer");
    }
    state.SetBytesProcessed(static_cast<int64_t>(state.iterations() * (result - buf.data())));
}

int main(int argc, char** argv) {
    for (auto& scanner : Scanner::All()) {
        auto* bm = ::benchmark::RegisterBenchmark(
            std::string{scanner.Name()},
            [&scanner](benchmark::State& state) {
                BenchmarkScanner(scanner, state);
            }
        );
        bm->Threads(1);
        bm->MinWarmUpTime(2);
        bm->MinTime(4);
        bm->UseRealTime();
    }
    ::benchmark::Initialize(&argc, argv);
    ::benchmark::RunSpecifiedBenchmarks();
    ::benchmark::Shutdown();
    return 0;
}
