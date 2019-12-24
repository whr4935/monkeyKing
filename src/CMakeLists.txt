add_subdirectory(android)
add_subdirectory(base)


set(MODULE_NAME monkeyking)
add_library(${MODULE_NAME} SHARED)
set_target_properties(${MODULE_NAME} PROPERTIES  LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)

target_link_libraries(${MODULE_NAME}
    "-Wl,--whole-archive"
    android_utils
    monkeyking_base
    "-Wl,--no-whole-archive")
