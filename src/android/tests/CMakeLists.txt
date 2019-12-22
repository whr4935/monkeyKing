
set(SOURCES "")
aux_source_directory(. SOURCES)

foreach(var ${SOURCES})

    string(REGEX REPLACE ".*/" "" var ${var})

    string(REGEX REPLACE ".cpp$|.cc$|.c$" "" target ${var})

    add_executable(${target} ${var})

    add_compile_options("$<$<COMPILE_LANGUAGE:CXX>:-std=gnu++14>")

    add_compile_options("$<$<COMPILE_LANGUAGE:CXX>:-g>")

    target_link_libraries(${target} -lgtest_main -lgtest android_utils)
    set_target_properties(${target} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)

endforeach(var)

