set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/deps)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/deps)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/deps)

add_subdirectory(CLI11 EXCLUDE_FROM_ALL)
add_library(deps_cli11 INTERFACE)
target_include_directories(deps_cli11 INTERFACE "${CMAKE_CURRENT_LIST_DIR}/CLI11/include")

add_subdirectory(GSL EXCLUDE_FROM_ALL)
add_library(deps_gsl INTERFACE)
target_include_directories(deps_gsl INTERFACE "${CMAKE_CURRENT_LIST_DIR}/GSL/include")


find_package(cryptominisat5 REQUIRED 5.8.0)
add_library(deps_cms INTERFACE)
target_link_libraries(deps_cms INTERFACE cryptominisat5)
target_include_directories(deps_cms INTERFACE ${CRYPTOMINISAT5_INCLUDE_DIRS})


add_library(deps_ipasir INTERFACE)
target_include_directories(deps_ipasir INTERFACE "${CMAKE_CURRENT_LIST_DIR}/ipasir")

add_library(deps_tomlxx INTERFACE)
target_include_directories(deps_tomlxx INTERFACE "${CMAKE_CURRENT_LIST_DIR}/tomlplusplus")

add_library(deps_dl INTERFACE)
if ("${CMAKE_SYSTEM_NAME}" STREQUAL "Linux")
  target_link_libraries(deps_dl INTERFACE dl)
endif()

