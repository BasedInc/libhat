project(libhat_tests)

if (NOT CPM_SOURCE_CACHE AND NOT ENV{CPM_SOURCE_CACHE})
    set(CPM_SOURCE_CACHE ${PROJECT_SOURCE_DIR}/.cpmcache)
endif()

include(cmake/get_cpm.cmake)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)

if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()

CPMAddPackage("gh:google/googletest#v1.17.0")
CPMAddPackage("gh:google/benchmark#v1.9.4")

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 21)
    target_compile_options(gtest PRIVATE "-Wno-character-conversion")
endif()

if(LIBHAT_TESTING_SDE)
    set(LIBHAT_SETUP_SDE OFF)
    if(WIN32)
        CPMAddPackage(
            NAME            intel_sde
            URL             https://downloadmirror.intel.com/831748/sde-external-9.44.0-2024-08-22-win.tar.xz
            URL_HASH        SHA256=ED1562840510ABEB958DFE4DFBFF5BEA9A960A0075839222E984393425E15FBF
            DOWNLOAD_ONLY   TRUE
        )
        set(LIBHAT_SETUP_SDE ON)
        set(LIBHAT_SDE_EXECUTABLE sde.exe)
    elseif(UNIX)
        CPMAddPackage(
            NAME            intel_sde
            URL             https://downloadmirror.intel.com/831748/sde-external-9.44.0-2024-08-22-lin.tar.xz
            URL_HASH        SHA256=C6BC16FC6D1855049EA22DAF214A8C00331713BC6A7B0C4D6955D6ED84148B9B
            DOWNLOAD_ONLY   TRUE
        )
        set(LIBHAT_SETUP_SDE ON)
        set(LIBHAT_SDE_EXECUTABLE sde64)
    else()
        message("Intel SDE not supported by host platform, testing support may be limited" WARNING)
    endif()
    if(LIBHAT_SETUP_SDE)
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
register_test(libhat_test_scanner tests/Scanner.cpp)

if(LIBHAT_TESTING_SAMPLE_BIN)
    CPMAddPackage(
        NAME            chromium_win_x64
        URL             https://www.googleapis.com/download/storage/v1/b/chromium-browser-snapshots/o/Win_x64%2F1533392%2Fchrome-win.zip?generation=1761109878458576&alt=media
        URL_HASH        SHA256=C53AC5F55C8EC6CD61E90D3D50DB7AF05D17708EBCD3141E0008CBE2F82DF5BF
        DOWNLOAD_ONLY   TRUE
    )
    register_test(libhat_benchmark_chromium benchmark/Chromium.cpp)
    set(CHROME_DLL_PATH "${chromium_win_x64_SOURCE_DIR}/chrome.dll")
    target_compile_definitions(libhat_benchmark_chromium PRIVATE CHROME_DLL_PATH=${CHROME_DLL_PATH})
endif()

add_executable(libhat_hwinfo info/HardwareInfo.cpp)
target_link_libraries(libhat_hwinfo PRIVATE libhat)
