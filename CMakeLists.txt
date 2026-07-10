cmake_minimum_required(VERSION 3.12)

set(LIBHAT_VERSION_MAJOR 0)
set(LIBHAT_VERSION_MINOR 10)
set(LIBHAT_VERSION_PATCH 0)
set(LIBHAT_VERSION ${LIBHAT_VERSION_MAJOR}.${LIBHAT_VERSION_MINOR}.${LIBHAT_VERSION_PATCH})

project(libhat
    VERSION ${LIBHAT_VERSION}
    DESCRIPTION "A high-performance, modern, C++20 library designed around game hacking"
    HOMEPAGE_URL "https://github.com/BasedInc/libhat"
    LANGUAGES C CXX
)

include(CheckCXXCompilerFlag)

# Compile Target Options
option(LIBHAT_STATIC_C_LIB "Build a static version of the C library" OFF)
option(LIBHAT_SHARED_C_LIB "Build a shared version of the C library" OFF)
option(LIBHAT_MODULE_TARGET "Create target for the module interface" OFF)
option(LIBHAT_USE_STD_MODULE "Compile the module target using the std module" OFF)
option(LIBHAT_INSTALL_TARGET "Creates install rules for the libhat target" ON)

# Compile Options
option(LIBHAT_WARNINGS_AS_ERRORS "Treat warnings as compiler errors" ${PROJECT_IS_TOP_LEVEL})

# Testing Options
option(LIBHAT_TESTING "Enable tests" ${PROJECT_IS_TOP_LEVEL})
option(LIBHAT_TESTING_SANITIZE "Enable address sanitizer when testing" ON)
option(LIBHAT_TESTING_SDE "Run tests using Intel Software Development Emulator" ON)
option(LIBHAT_TESTING_SAMPLE_BIN "Enables tests that require downloading a large sample binary" ON)
option(LIBHAT_EXAMPLES "Include example targets" ${PROJECT_IS_TOP_LEVEL})

# Library Feature Options
option(LIBHAT_FEATURE_SSE "Enables SSE scanning, has no effect if the target isn't x86 or x86_64" ON)
option(LIBHAT_FEATURE_AVX512 "Enables AVX512 scanning, has no effect if the target isn't x86_64" ON)
option(LIBHAT_HINT_X86_64 "Enables support for the x86_64 scan hint, requires a small (2KB) data table" ON)
option(LIBHAT_HINT_AARCH64 "Enables support for the aarch64 scan hint, requires a small (2KB) data table" ON)

if(LIBHAT_TESTING AND LIBHAT_TESTING_SANITIZE)
    if(MSVC)
        add_compile_options($<$<AND:$<CONFIG:Debug>,$<CXX_COMPILER_ID:MSVC>>:/fsanitize=address>)
    else()
        add_compile_options($<$<CONFIG:Debug>:-fsanitize=address,undefined>)
        add_link_options($<$<CONFIG:Debug>:-fsanitize=address,undefined>)
    endif()
endif()

set(LIBHAT_SRC
    src/Scanner.cpp
    src/System.cpp

    src/os/mac/MemoryProtector.cpp
    src/os/mac/Process.cpp

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

    src/arch/arm/Neon.cpp
    src/arch/arm/System.cpp
)

add_library(libhat STATIC ${LIBHAT_SRC})
add_library(libhat::libhat ALIAS libhat)

set_target_properties(libhat PROPERTIES
    PREFIX ""
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN TRUE
)
if(UNIX)
    set_target_properties(libhat PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif()

target_compile_features(libhat PUBLIC cxx_std_20)

if (MSVC)
    if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        target_compile_options(libhat PRIVATE /clang:-Wall /clang:-Wextra /clang:-Wconversion
            $<$<BOOL:${LIBHAT_WARNINGS_AS_ERRORS}>:/clang:-Werror>
        )
    else()
        target_compile_options(libhat PRIVATE /W3
            $<$<BOOL:${LIBHAT_WARNINGS_AS_ERRORS}>:/WX>
        )
    endif()
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(libhat PRIVATE -Wall -Wextra -Wconversion
        $<$<BOOL:${LIBHAT_WARNINGS_AS_ERRORS}>:-Werror>
    )
endif()

target_include_directories(libhat PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/>
    $<INSTALL_INTERFACE:include>
)

target_compile_definitions(libhat PUBLIC
    "$<$<BOOL:${LIBHAT_FEATURE_SSE}>:LIBHAT_FEATURE_SSE>"
    "$<$<BOOL:${LIBHAT_FEATURE_AVX512}>:LIBHAT_FEATURE_AVX512>"
    "$<$<BOOL:${LIBHAT_HINT_X86_64}>:LIBHAT_HINT_X86_64>"
    "$<$<BOOL:${LIBHAT_HINT_AARCH64}>:LIBHAT_HINT_AARCH64>"
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
    set_target_properties(libhat_c PROPERTIES
        PREFIX ""
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN TRUE
    )

    add_library(libhat::libhat_c ALIAS libhat_c)
    target_link_libraries(libhat_c PRIVATE libhat)
    target_include_directories(libhat_c PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/>
        $<INSTALL_INTERFACE:include>
    )
endif()

if(LIBHAT_MODULE_TARGET)
    add_library(libhat_module)
    set_target_properties(libhat_module PROPERTIES PREFIX "")
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
