#include <gtest/gtest.h>
#include <libhat/scanner.hpp>

TEST(ProcessTest, ProcessModuleMatchesEmptyStr) {
    EXPECT_EQ(hat::process::get_process_module(), hat::process::get_module({}));
}

TEST(ProcessTest, ProcessModuleHasDefaultSegments) {
    bool rx = false; // .text
    bool r = false;  // .rdata
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
