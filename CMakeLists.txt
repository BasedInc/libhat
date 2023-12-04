cmake_minimum_required(VERSION 3.28)
project(libhat)

if (NOT CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 20)
endif()

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

add_library(libhat STATIC)

target_sources(libhat PUBLIC FILE_SET cxx_modules TYPE CXX_MODULES FILES
    include/libhat.ixx
    include/libhat/Callable.ixx
    include/libhat/CompileTime.ixx
    include/libhat/MemoryProtector.ixx
    include/libhat/Process.ixx
    include/libhat/Result.ixx
    include/libhat/Scanner.ixx
    include/libhat/Signature.ixx
)

target_sources(libhat PRIVATE
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
)

target_include_directories(libhat PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/>
)

target_compile_definitions(libhat PUBLIC
    "$<$<BOOL:${LIBHAT_DISABLE_AVX512}>:LIBHAT_DISABLE_AVX512>"
)
