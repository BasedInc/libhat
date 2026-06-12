#include <gtest/gtest.h>
#include <libhat/scanner.hpp>

TEST(ProcessTest, ProcessModuleMatchesEmptyStr) {
    EXPECT_EQ(hat::process::get_process_module(), hat::process::get_module({}));
}
