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

if(LIBHAT_TESTING_SDE)
    set(LIBHAT_SETUP_SDE OFF)
    if(WIN32)
        FetchContent_Declare(
            intel_sde
            URL         https://downloadmirror.intel.com/831748/sde-external-9.44.0-2024-08-22-win.tar.xz
            URL_HASH    SHA256=ED1562840510ABEB958DFE4DFBFF5BEA9A960A0075839222E984393425E15FBF
        )
        set(LIBHAT_SETUP_SDE ON)
        set(LIBHAT_SDE_EXECUTABLE sde.exe)
    elseif(UNIX)
        FetchContent_Declare(
            intel_sde
            URL         https://downloadmirror.intel.com/831748/sde-external-9.44.0-2024-08-22-lin.tar.xz
            URL_HASH    SHA256=C6BC16FC6D1855049EA22DAF214A8C00331713BC6A7B0C4D6955D6ED84148B9B
        )
        set(LIBHAT_SETUP_SDE ON)
        set(LIBHAT_SDE_EXECUTABLE sde64)
    else()
        message("Intel SDE not supported by host platform, testing support may be limited" WARNING)
    endif()
    if(LIBHAT_SETUP_SDE)
        FetchContent_MakeAvailable(intel_sde)
        set(LIBHAT_TEST_COMMAND_PREFIX
            ${intel_sde_SOURCE_DIR}/${LIBHAT_SDE_EXECUTABLE}
            -gnr # Supports AVX512 emulation
            --
        )
    endif()
endif()

function(register_test NAME SOURCE)
    add_executable(${NAME} ${SOURCE})
    target_link_libraries(${NAME} PRIVATE gtest gtest_main benchmark libhat)
    add_test(NAME ${NAME} COMMAND ${LIBHAT_TEST_COMMAND_PREFIX} $<TARGET_FILE:${NAME}> --benchmark_counters_tabular=true)
endfunction()

register_test(libhat_benchmark_compare benchmark/Compare.cpp)

add_executable(libhat_hwinfo info/HardwareInfo.cpp)
target_link_libraries(libhat_hwinfo PRIVATE libhat)
