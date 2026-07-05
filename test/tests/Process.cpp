#include <gtest/gtest.h>
#include <libhat/scanner.hpp>

TEST(ProcessTest, ProcessModuleMatchesEmptyStr) {
    EXPECT_EQ(hat::process::get_process_module(), hat::process::get_module({}));
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
