aux_source_directory(. SOURCES)

set(MODULE_NAME monkeyking_base)

message(SOURCES=${SOURCES})

add_library(${MODULE_NAME} STATIC ${SOURCES})

target_include_directories(${MODULE_NAME} PUBLIC include)

target_compile_options(${MODULE_NAME} PUBLIC -fPIC)

set(CMAKE_CXX_FLAGS "-std=gnu++14 ${CMAKE_CXX_FLAGS}")

target_link_libraries(${MODULE_NAME} -pthread -lm)
#set_target_properties(${MODULE_NAME} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)

#add_subdirectory(foundation/tests)
#add_subdirectory(tests)

