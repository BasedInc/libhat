#include <gtest/gtest.h>

#include <libhat/process.hpp>
#include <libhat/system.hpp>

#ifdef LIBHAT_UNIX
#include <sys/mman.h>

static int to_system_prot(const hat::protection flags) {
    int prot = 0;
    if (static_cast<bool>(flags & hat::protection::Read)) prot |= PROT_READ;
    if (static_cast<bool>(flags & hat::protection::Write)) prot |= PROT_WRITE;
    if (static_cast<bool>(flags & hat::protection::Execute)) prot |= PROT_EXEC;
    return prot;
}

std::shared_ptr<std::byte> virtual_allocate(const hat::protection prot, const size_t size) {
    return std::shared_ptr<std::byte>{
        static_cast<std::byte*>(mmap(nullptr, size, to_system_prot(prot), MAP_PRIVATE | MAP_ANONYMOUS, -1, 0)),
        [size](void* ptr) {
            munmap(ptr, size);
        }
    };
}
#else

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>

static DWORD to_system_prot(const hat::protection flags) {
    const bool r = static_cast<bool>(flags & hat::protection::Read);
    const bool w = static_cast<bool>(flags & hat::protection::Write);
    const bool x = static_cast<bool>(flags & hat::protection::Execute);

    if (x && w) return PAGE_EXECUTE_READWRITE;
    if (x && r) return PAGE_EXECUTE_READ;
    if (x)      return PAGE_EXECUTE;
    if (w)      return PAGE_READWRITE;
    if (r)      return PAGE_READONLY;
    return PAGE_NOACCESS;
}

static std::shared_ptr<std::byte> virtual_allocate(const hat::protection prot, const size_t size) {
    return std::shared_ptr<std::byte>{
        static_cast<std::byte*>(VirtualAlloc(nullptr, size, MEM_COMMIT | MEM_RESERVE, to_system_prot(prot))),
        [](void* ptr) {
            if (ptr) {
                VirtualFree(ptr, 0, MEM_RELEASE);
            }
        }
    };
}
#endif

TEST(SystemTest, ValidateSystemPageSize) {
    auto& system = hat::get_system();
    EXPECT_GE(system.page_size, 4096);
    EXPECT_EQ((system.page_size & (system.page_size - 1)), 0);
}

TEST(ProcessTest, IsRWX) {
    constexpr size_t size = 0x1000;
    {
        const auto page = ::virtual_allocate({}, size);

        const std::span data{page.get(), size};
        EXPECT_FALSE(hat::process::is_readable(data));
        EXPECT_FALSE(hat::process::is_writable(data));
        EXPECT_FALSE(hat::process::is_executable(data));

        const std::span subdata = data.subspan(size / 4, size / 2);
        EXPECT_FALSE(hat::process::is_readable(subdata));
        EXPECT_FALSE(hat::process::is_writable(subdata));
        EXPECT_FALSE(hat::process::is_executable(subdata));
    }
    {
        const auto page = ::virtual_allocate(hat::protection::Read, size);

        const std::span data{page.get(), size};
        EXPECT_TRUE(hat::process::is_readable(data));
        EXPECT_FALSE(hat::process::is_writable(data));
        EXPECT_FALSE(hat::process::is_executable(data));

        const std::span subdata = data.subspan(size / 4, size / 2);
        EXPECT_TRUE(hat::process::is_readable(subdata));
        EXPECT_FALSE(hat::process::is_writable(subdata));
        EXPECT_FALSE(hat::process::is_executable(subdata));
    }
    {
        const auto page = ::virtual_allocate(hat::protection::Read | hat::protection::Write, size);

        const std::span data{page.get(), size};
        EXPECT_TRUE(hat::process::is_readable(data));
        EXPECT_TRUE(hat::process::is_writable(data));
        EXPECT_FALSE(hat::process::is_executable(data));

        const std::span subdata = data.subspan(size / 4, size / 2);
        EXPECT_TRUE(hat::process::is_readable(subdata));
        EXPECT_TRUE(hat::process::is_writable(subdata));
        EXPECT_FALSE(hat::process::is_executable(subdata));
    }
    {
        const auto page = ::virtual_allocate(hat::protection::Read | hat::protection::Execute, size);

        const std::span data{page.get(), size};
        EXPECT_TRUE(hat::process::is_readable(data));
        EXPECT_FALSE(hat::process::is_writable(data));
        EXPECT_TRUE(hat::process::is_executable(data));

        const std::span subdata = data.subspan(size / 4, size / 2);
        EXPECT_TRUE(hat::process::is_readable(subdata));
        EXPECT_FALSE(hat::process::is_writable(subdata));
        EXPECT_TRUE(hat::process::is_executable(subdata));
    }
}

TEST(ProcessTest, ProcessModuleMatchesEmptyStr) {
    EXPECT_EQ(hat::process::get_process_module(), hat::process::get_module({}));
}

TEST(ProcessTest, ProcessModuleDataContainsGTest) {
    const auto func = reinterpret_cast<void*>(&RUN_ALL_TESTS);
    const auto text = hat::process::get_process_module().get_module_data();
    EXPECT_LE(std::to_address(text.begin()), func);
    EXPECT_GT(std::to_address(text.end()), func);
}

TEST(ProcessTest, ProcessExecutableDataContainsGTest) {
    const auto func = reinterpret_cast<void*>(&RUN_ALL_TESTS);
    const auto text = hat::process::get_process_module().get_executable_data();
    EXPECT_LE(std::to_address(text.begin()), func);
    EXPECT_GT(std::to_address(text.end()), func);
}

TEST(ProcessTest, ModuleAtMatchesModuleBounds) {
    const auto mod = hat::process::get_process_module();
    const auto data = mod.get_module_data();

    EXPECT_EQ(hat::process::module_at(&data.front()), mod);
    EXPECT_EQ(hat::process::module_at(&data.back()), mod);
}

TEST(ProcessTest, ProcessModuleHasDefaultSegments) {
    bool rx = false; // .text
#ifdef LIBHAT_MAC
    bool r = true; // only __TEXT and __DATA are expected
#else
    bool r = false;  // .rdata
#endif
    bool rw = false; // .data
    hat::process::get_process_module().for_each_segment([&](auto, auto prot) {
        if (prot == (hat::protection::Read | hat::protection::Execute)) {
            rx = true;
        } else if (prot == (hat::protection::Read)) {
            r = true;
        } else if (prot == (hat::protection::Read | hat::protection::Write)) {
            rw = true;
        }
        return !rx || !r || !rw;
    });
    EXPECT_TRUE(rx);
    EXPECT_TRUE(r);
    EXPECT_TRUE(rw);
}

TEST(ProcessTest, ProcessModuleHasDefaultSections) {
    bool rx = false; // .text
#ifdef LIBHAT_MAC
    bool r = true; // only __TEXT and __DATA are expected
#else
    bool r = false;  // .rdata
#endif
    bool rw = false; // .data
    hat::process::get_process_module().for_each_section([&](auto, auto, auto prot) {
        if (prot == (hat::protection::Read | hat::protection::Execute)) {
            rx = true;
        } else if (prot == (hat::protection::Read)) {
            r = true;
        } else if (prot == (hat::protection::Read | hat::protection::Write)) {
            rw = true;
        }
        return !rx || !r || !rw;
    });
    EXPECT_TRUE(rx);
    EXPECT_TRUE(r);
    EXPECT_TRUE(rw);
}

TEST(ProcessTest, ModuleSectionsMatchLookup) {
    const auto mod = hat::process::get_process_module();
    mod.for_each_section([&](auto name, auto data, auto) {
        auto lookup = mod.get_section_data(name);
        EXPECT_EQ(std::to_address(lookup.begin()), std::to_address(data.begin()));
        EXPECT_EQ(std::to_address(lookup.end()), std::to_address(data.end()));
        return true;
    });
}
