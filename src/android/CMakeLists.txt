
find_library(SAFE_IOP libsafe_iop)

if (NOT SAFE_IOP)
    message(FATAL_ERROR "safe_iop not found!")
else()
    message(safe_iop=${SAFE_IOP} ddd ${libsafe_iop})
endif()


MACRO(SUBDIRLIST result curdir)
  FILE(GLOB children RELATIVE ${curdir} ${curdir}/*)
  SET(dirlist "")
  FOREACH(child ${children})
    IF(IS_DIRECTORY ${curdir}/${child})
      LIST(APPEND dirlist ${child})
    ENDIF()
  ENDFOREACH()
  SET(${result} ${dirlist})
ENDMACRO()


aux_source_directory(foundation SOURCES)
aux_source_directory(utils SOURCES)

set(MODULE_NAME android_utils)

message(SOURCES=${SOURCES})

add_library(${MODULE_NAME} STATIC ${SOURCES})

target_include_directories(${MODULE_NAME} PUBLIC include include/foundation)

target_compile_definitions(${MODULE_NAME} PUBLIC HAVE_PTHREADS)

target_compile_options(${MODULE_NAME} PUBLIC -fPIC)

set(CMAKE_CXX_FLAGS "-std=gnu++14 ${CMAKE_CXX_FLAGS}")

target_link_libraries(${MODULE_NAME} -pthread -lm -lsafe_iop)
#set_target_properties(${MODULE_NAME} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)

add_subdirectory(foundation/tests)
add_subdirectory(tests)

