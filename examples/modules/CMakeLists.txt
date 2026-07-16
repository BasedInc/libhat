add_executable(libhat_example_modules main.cpp)

set_target_properties(libhat_example_modules
    PROPERTIES CXX_SCAN_FOR_MODULES ON)

target_link_libraries(libhat_example_modules
    PRIVATE libhat::libhat_module)
