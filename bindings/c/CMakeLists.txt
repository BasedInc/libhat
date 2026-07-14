if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    message(FATAL_ERROR "This file cannot be built directly and must be included via the root CMakeLists.txt")
endif()

set(LIBHAT_C_HEADERS include/libhat/c/libhat.h)
set(LIBHAT_C_SOURCES src/libhat.cpp)

if(LIBHAT_STATIC_C_LIB)
    add_library(libhat_c STATIC ${LIBHAT_C_SOURCES})
elseif(LIBHAT_SHARED_C_LIB)
    add_library(libhat_c SHARED ${LIBHAT_C_SOURCES})
    target_compile_definitions(libhat_c
        PRIVATE LIBHAT_BUILD_SHARED_LIB
        INTERFACE LIBHAT_USE_SHARED_LIB
    )
else()
    message(FATAL_ERROR "Expected LIBHAT_STATIC_C_LIB or LIBHAT_SHARED_C_LIB to be defined")
endif()
target_sources(libhat_c PUBLIC
    FILE_SET headers
    TYPE HEADERS
    BASE_DIRS include
    FILES ${LIBHAT_C_HEADERS}
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
set(LIBHAT_INSTALL_TARGETS "${LIBHAT_INSTALL_TARGETS}" PARENT_SCOPE)
