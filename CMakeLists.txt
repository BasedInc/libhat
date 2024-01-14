cmake_minimum_required(VERSION 3.0)

if (NOT CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 20)
endif()

project(libhat)

option(LIBHAT_STATIC_C_LIB "Build a static version of the C library" OFF)
option(LIBHAT_SHARED_C_LIB "Build a shared version of the C library" OFF)
option(LIBHAT_INSTALL_TARGET "Creates install rules for the libhat target" ON)
option(LIBHAT_DISABLE_AVX512 "Disables AVX512 scanning" OFF)

if (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    set_source_files_properties(src/arch/x86/AVX2.cpp PROPERTIES COMPILE_FLAGS "/arch:AVX2")
    set_source_files_properties(src/arch/x86/AVX512.cpp PROPERTIES COMPILE_FLAGS "/arch:AVX512")
elseif (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set_source_files_properties(src/arch/x86/SSE.cpp PROPERTIES COMPILE_FLAGS "-msse4.1")
    set_source_files_properties(src/arch/x86/AVX2.cpp PROPERTIES COMPILE_FLAGS "-mavx -mavx2 -mbmi")
    set_source_files_properties(src/arch/x86/AVX512.cpp PROPERTIES COMPILE_FLAGS "-mavx512f -mavx512bw -mbmi")
    set_source_files_properties(src/arch/x86/System.cpp PROPERTIES COMPILE_FLAGS "-mxsave")
endif ()

set(LIBHAT_SRC
    src/Process.cpp
    src/Scanner.cpp
    src/System.cpp

    src/os/win32/MemoryProtector.cpp
    src/os/win32/Process.cpp
    src/os/win32/Scanner.cpp

    src/arch/x86/SSE.cpp
    src/arch/x86/AVX2.cpp
    src/arch/x86/AVX512.cpp
    src/arch/x86/System.cpp

    src/arch/arm/System.cpp)

add_library(libhat STATIC ${LIBHAT_SRC})

target_include_directories(libhat PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/>
    $<INSTALL_INTERFACE:include>
)

target_compile_definitions(libhat PUBLIC
    "$<$<BOOL:${LIBHAT_DISABLE_AVX512}>:LIBHAT_DISABLE_AVX512>"
)

if (LIBHAT_STATIC_C_LIB OR LIBHAT_SHARED_C_LIB)
    set(LIBHAT_C_SOURCES
        src/c/libhat.cpp
    )

    if (LIBHAT_STATIC_C_LIB)
        add_library(libhat_c STATIC ${LIBHAT_C_SOURCES})
    else()
        add_library(libhat_c SHARED ${LIBHAT_C_SOURCES})
        target_compile_definitions(libhat_c PRIVATE "LIBHAT_BUILD_SHARED_LIB")
    endif()

    target_link_libraries(libhat_c PRIVATE libhat)
    target_include_directories(libhat_c PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/>
        $<INSTALL_INTERFACE:include>
    )
endif()

if (LIBHAT_INSTALL_TARGET)
    install(TARGETS libhat
        EXPORT libhat-targets
        RUNTIME DESTINATION "bin"
        ARCHIVE DESTINATION "lib"
        LIBRARY DESTINATION "lib"
    )
endif()
