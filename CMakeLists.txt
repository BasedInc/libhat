cmake_minimum_required(VERSION 3.23)

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
option(LIBHAT_EXAMPLES "Include example targets" ${PROJECT_IS_TOP_LEVEL})

# Compile Options
option(LIBHAT_WARNINGS_AS_ERRORS "Treat warnings as compiler errors" ${PROJECT_IS_TOP_LEVEL})

# Testing Options
option(LIBHAT_TESTING "Enable tests" ${PROJECT_IS_TOP_LEVEL})
option(LIBHAT_TESTING_SANITIZE "Enable address sanitizer when testing" ON)
option(LIBHAT_TESTING_SDE "Run tests using Intel Software Development Emulator" ON)
option(LIBHAT_TESTING_SAMPLE_BIN "Enables tests that require downloading a large sample binary" ON)

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

file(GLOB_RECURSE LIBHAT_HEADERS RELATIVE ${CMAKE_CURRENT_LIST_DIR} CONFIGURE_DEPENDS "include/*.hpp")
list(APPEND LIBHAT_HEADERS include/libhat/platform.h)

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

target_compile_features(libhat PUBLIC cxx_std_20)
target_sources(libhat PUBLIC
    FILE_SET headers
    TYPE HEADERS
    BASE_DIRS include
    FILES ${LIBHAT_HEADERS}
)
set_target_properties(libhat PROPERTIES
    PREFIX ""
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN TRUE
)
if(UNIX)
    set_target_properties(libhat PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif()

set(LIBHAT_INSTALL_TARGETS libhat)
set(LIBHAT_INSTALL_FILE_SET FILE_SET headers)

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
    target_sources(libhat_c PUBLIC
        FILE_SET headers
        TYPE HEADERS
        BASE_DIRS include
        FILES include/libhat/c/libhat.h
    )

    set_target_properties(libhat_c PROPERTIES
        PREFIX ""
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN TRUE
    )
    target_compile_definitions(libhat_c PRIVATE
        LIBHAT_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
        LIBHAT_VERSION_MINOR=${PROJECT_VERSION_MINOR}
        LIBHAT_VERSION_PATCH=${PROJECT_VERSION_PATCH}
        LIBHAT_VERSION="${PROJECT_VERSION}"
    )

    add_library(libhat::libhat_c ALIAS libhat_c)
    target_link_libraries(libhat_c PRIVATE libhat)
    list(APPEND LIBHAT_INSTALL_TARGETS libhat_c)
endif()

if(LIBHAT_MODULE_TARGET)
    add_library(libhat_module)
    set_target_properties(libhat_module PROPERTIES PREFIX "")
    target_compile_features(libhat_module PUBLIC cxx_std_20)
    target_link_libraries(libhat_module PRIVATE libhat::libhat)
    target_sources(libhat_module PUBLIC
        FILE_SET modules
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

    list(APPEND LIBHAT_INSTALL_TARGETS libhat_module)
    list(APPEND LIBHAT_INSTALL_FILE_SET
        FILE_SET modules
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/libhat"
    )
endif()

if(LIBHAT_TESTING)
    include(CTest)
    enable_testing()
    add_subdirectory(test)
endif()

if(LIBHAT_INSTALL_TARGET)
    set(namespace "libhat::")
    set(targets_export_name "libhat-targets")
    set(version_config "${PROJECT_BINARY_DIR}/libhat-config-version.cmake")
    set(project_config_in "${CMAKE_CURRENT_LIST_DIR}/cmake/libhat-config.cmake.in")
    set(project_config "${PROJECT_BINARY_DIR}/libhat-config.cmake")
    set(export_dest_dir "${CMAKE_INSTALL_LIBDIR}/cmake/libhat")

    # Headers, binaries, libraries
    install(
        TARGETS ${LIBHAT_INSTALL_TARGETS}
        EXPORT ${targets_export_name}
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        ${LIBHAT_INSTALL_FILE_SET}
    )

    include(CMakePackageConfigHelpers)
    configure_package_config_file(
        "${project_config_in}"
        "${project_config}"
        INSTALL_DESTINATION "${export_dest_dir}"
    )
    write_basic_package_version_file(
        "${version_config}"
        VERSION "${LIBHAT_VERSION}"
        COMPATIBILITY SameMajorVersion
    )

    # CMake
    export(
        TARGETS ${LIBHAT_INSTALL_TARGETS}
        NAMESPACE ${namespace}
        FILE ${PROJECT_BINARY_DIR}/${targets_export_name}.cmake
    )
    install(
        EXPORT ${targets_export_name}
        NAMESPACE "${namespace}"
        DESTINATION "${export_dest_dir}"
    )
    install(
        FILES
            "${project_config}"
            "${version_config}"
        DESTINATION "${export_dest_dir}"
    )
endif()
