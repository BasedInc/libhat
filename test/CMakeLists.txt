project(libhat_tests)

include(FetchContent)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY  https://github.com/google/googletest.git
    GIT_TAG         96cd50c082d880a9aab6455dcc5817cfbf0ea45f
)
FetchContent_Declare(
    benchmark
    GIT_REPOSITORY  https://github.com/google/benchmark.git
    GIT_TAG         4682db08bc7bb7e547e0a1056e32392998f8101f
)
FetchContent_MakeAvailable(googletest benchmark)

function(register_test NAME SOURCE)
    add_executable(${NAME} ${SOURCE})
    target_link_libraries(${NAME} PRIVATE gtest benchmark libhat)
    add_test(NAME ${NAME} COMMAND ${NAME} --benchmark_counters_tabular=true)
endfunction()

register_test(libhat_benchmark_compare benchmark/Compare.cpp)

add_executable(libhat_hwinfo info/HardwareInfo.cpp)
target_link_libraries(libhat_hwinfo PRIVATE libhat)