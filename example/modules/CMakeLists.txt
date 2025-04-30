add_executable(libhat_module_test main.cpp)

set_target_properties(libhat_module_test
    PROPERTIES CXX_SCAN_FOR_MODULES ON)

target_link_libraries(libhat_module_test
    PRIVATE libhat::libhat-module)
