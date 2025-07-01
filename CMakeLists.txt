cmake_minimum_required(VERSION 3.12)

set(LIBHAT_VERSION_MAJOR 0)
set(LIBHAT_VERSION_MINOR 6)
set(LIBHAT_VERSION_PATCH 0)
set(LIBHAT_VERSION ${LIBHAT_VERSION_MAJOR}.${LIBHAT_VERSION_MINOR}.${LIBHAT_VERSION_PATCH})

project(libhat
    VERSION ${LIBHAT_VERSION}
    DESCRIPTION "A high-performance, modern, C++20 library designed around game hacking"
    HOMEPAGE_URL "https://github.com/BasedInc/libhat"
    LANGUAGES C CXX
)

include(CheckCXXCompilerFlag)

option(LIBHAT_STATIC_C_LIB "Build a static version of the C library" OFF)
option(LIBHAT_SHARED_C_LIB "Build a shared version of the C library" OFF)
option(LIBHAT_INSTALL_TARGET "Creates install rules for the libhat target" ON)
option(LIBHAT_DISABLE_SSE "Disables SSE scanning" OFF)
option(LIBHAT_DISABLE_AVX512 "Disables AVX512 scanning" OFF)
option(LIBHAT_TESTING "Enable tests" ${PROJECT_IS_TOP_LEVEL})
option(LIBHAT_TESTING_ASAN "Enable address sanitizer when testing" ON)
option(LIBHAT_TESTING_SDE "Run tests using Intel Software Development Emulator" ON)
option(LIBHAT_MODULE_TARGET "Create target for the module interface" OFF)
option(LIBHAT_USE_STD_MODULE "Compile the module target using the std module" OFF)
option(LIBHAT_EXAMPLES "Include example targets" ${PROJECT_IS_TOP_LEVEL})

option(LIBHAT_HINT_X86_64 "Enables support for the x86_64 scan hint, requires a small (less than 1KB) data table" ON)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    # Just assume that the compiler accepts the required x86/x64 options if it'll accept -mmmx
    # CMake architecture detection SUCKS!!!
    check_cxx_compiler_flag("-mmmx" LIBHAT_COMPILER_X86_OPTIONS)

    if(LIBHAT_COMPILER_X86_OPTIONS)
        set_source_files_properties(src/arch/x86/SSE.cpp PROPERTIES COMPILE_FLAGS "-msse4.1")
        set_source_files_properties(src/arch/x86/AVX2.cpp PROPERTIES COMPILE_FLAGS "-mavx -mavx2 -mbmi")
        set_source_files_properties(src/arch/x86/AVX512.cpp PROPERTIES COMPILE_FLAGS "-mavx512f -mavx512bw -mbmi")
        set_source_files_properties(src/arch/x86/System.cpp PROPERTIES COMPILE_FLAGS "-mxsave")
    endif()
endif()

if(LIBHAT_TESTING AND LIBHAT_TESTING_ASAN)
    if(MSVC AND NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        add_compile_options($<$<CONFIG:Debug>:/fsanitize=address>)
    endif()
    # TODO: Linux
endif()

set(LIBHAT_SRC
    src/Scanner.cpp
    src/System.cpp

    src/os/linux/MemoryProtector.cpp
    src/os/linux/Process.cpp

    src/os/unix/System.cpp

    src/os/win32/MemoryProtector.cpp
    src/os/win32/Process.cpp
    src/os/win32/Scanner.cpp
    src/os/win32/System.cpp

    src/arch/x86/SSE.cpp
    src/arch/x86/AVX2.cpp
    src/arch/x86/AVX512.cpp
    src/arch/x86/System.cpp

    src/arch/arm/System.cpp)

add_library(libhat STATIC ${LIBHAT_SRC})
add_library(libhat::libhat ALIAS libhat)

if(UNIX)
    set_target_properties(libhat PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif()

target_compile_features(libhat PUBLIC cxx_std_20)

if (MSVC)
    if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        target_compile_options(libhat PRIVATE /clang:-Wall /clang:-Wextra /clang:-Wconversion /clang:-Werror)
    else()
        target_compile_options(libhat PRIVATE /W3 /WX)
    endif()
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(libhat PRIVATE -Wall -Wextra -Wconversion -Werror
            # temp fix for macOS CI failing due to incorrect LIBHAT_COMPILER_X86_OPTIONS value
            -Wno-unused-command-line-argument
    )
endif()

target_include_directories(libhat PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/>
    $<INSTALL_INTERFACE:include>
)

target_compile_definitions(libhat PUBLIC
    "$<$<BOOL:${LIBHAT_DISABLE_SSE}>:LIBHAT_DISABLE_SSE>"
    "$<$<BOOL:${LIBHAT_DISABLE_AVX512}>:LIBHAT_DISABLE_AVX512>"
    "$<$<BOOL:${LIBHAT_HINT_X86_64}>:LIBHAT_HINT_X86_64>"
)

if(LIBHAT_STATIC_C_LIB OR LIBHAT_SHARED_C_LIB)
    set(LIBHAT_C_SOURCES
        src/c/libhat.cpp
    )

    if(LIBHAT_STATIC_C_LIB)
        add_library(libhat_c STATIC ${LIBHAT_C_SOURCES})
        if(UNIX)
            set_target_properties(libhat_c PROPERTIES POSITION_INDEPENDENT_CODE ON)
        endif()
    else()
        add_library(libhat_c SHARED ${LIBHAT_C_SOURCES})
        target_compile_definitions(libhat_c
            PRIVATE LIBHAT_BUILD_SHARED_LIB
            INTERFACE LIBHAT_USE_SHARED_LIB
        )
    endif()

    add_library(libhat::libhat_c ALIAS libhat_c)
    target_link_libraries(libhat_c PRIVATE libhat)
    target_include_directories(libhat_c PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/>
        $<INSTALL_INTERFACE:include>
    )
endif()

if(LIBHAT_MODULE_TARGET)
    add_library(libhat_module)
    target_compile_features(libhat_module PUBLIC cxx_std_20)
    target_link_libraries(libhat_module PRIVATE libhat::libhat)
    target_sources(libhat_module PUBLIC
        FILE_SET CXX_MODULES
        TYPE CXX_MODULES
        FILES module/libhat.cppm
    )
    target_compile_definitions(libhat_module PRIVATE
        "$<$<BOOL:${LIBHAT_USE_STD_MODULE}>:LIBHAT_USE_STD_MODULE>"
    )
    add_library(libhat::libhat_module ALIAS libhat_module)

    if(LIBHAT_EXAMPLES)
        add_subdirectory(example/modules)
    endif()
endif()

if(LIBHAT_TESTING)
    include(CTest)
    enable_testing()
    add_subdirectory(test)
endif()

if(LIBHAT_INSTALL_TARGET)
    install(TARGETS libhat
        EXPORT libhat-targets
        RUNTIME DESTINATION "bin"
        ARCHIVE DESTINATION "lib"
        LIBRARY DESTINATION "lib"
    )
endif()
