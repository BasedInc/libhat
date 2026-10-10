#pragma once

#include <benchmark/benchmark.h>
#include <libhat/cstring_view.hpp>

#include <memory>
#include <ranges>
#include <span>
#include <string_view>
#include <vector>

class Scanner {
public:
    virtual ~Scanner() = default;

    virtual std::string_view Name() = 0;
    virtual bool Supported() { return true; }
    virtual void Setup(hat::cstring_view signature) = 0;
    virtual const std::byte* Find(std::span<const std::byte> buffer) = 0;

    template<std::derived_from<Scanner> T>
    static Scanner& Register() {
        return *Registry().emplace_back(std::make_unique<T>());
    }

    static auto All() {
        return std::as_const(Registry()) | std::views::transform([](auto& impl) -> Scanner& { return *impl; });
    }

private:
    static std::vector<std::unique_ptr<Scanner>>& Registry() {
        static std::vector<std::unique_ptr<Scanner>> instance;
        return instance;
    }
};

#define REGISTER_SCANNER(...) auto& BENCHMARK_PRIVATE_NAME() = Scanner::Register<__VA_ARGS__>()
