if(TARGET libhat::libhat_module)
    add_subdirectory(modules)
endif()

function(add_example name)
    set(target_name libhat_example_${name})
    add_executable(${target_name} ${name}/main.cpp)
    target_link_libraries(${target_name} PRIVATE libhat::libhat)
endfunction()

add_example(vfthook)
add_example(libfmt)

CPMAddPackage("gh:fmtlib/fmt#12.2.0")
target_link_libraries(libhat_example_libfmt PRIVATE fmt::fmt)
