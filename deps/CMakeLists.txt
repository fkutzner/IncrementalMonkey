set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/deps)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/deps)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/deps)

add_subdirectory(cnf-utils)
add_subdirectory(GSL)


add_library(deps_gsl INTERFACE)
target_include_directories(deps_gsl INTERFACE "${CMAKE_CURRENT_LIST_DIR}/GSL/include")
