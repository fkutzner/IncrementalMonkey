cmake_minimum_required(VERSION 3.12)
cmake_policy(VERSION 3.12)

project(IncrementalMonkey-CompleteBuild VERSION 0.1.0)

include(ExternalProject)

ExternalProject_Add(CryptoMiniSat
  GIT_REPOSITORY "https://github.com/msoos/cryptominisat"
  GIT_TAG 5.8.0
  PREFIX "${CMAKE_CURRENT_BINARY_DIR}"
  CMAKE_ARGS
    -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}
    -DENABLE_PYTHON_INTERFACE=OFF
    -DONLY_SIMPLE=ON
    -DMANPAGE=OFF
    -DNOM4RI=ON
    -DNOSQLITE=ON
)

ExternalProject_Add(IncrementalMonkey
  DEPENDS CryptoMiniSat

  SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/.."
  PREFIX "${CMAKE_CURRENT_BINARY_DIR}"
  CMAKE_ARGS
    -DCMAKE_PREFIX_PATH="${CMAKE_INSTALL_PREFIX}"
    -DCMAKE_INSTALL_PREFIX="${CMAKE_INSTALL_PREFIX}"

  TEST_COMMAND "${CMAKE_CURRENT_BINARY_DIR}/src/IncrementalMonkey-build/bin/incmonktests.libincmonk.unit"
  TEST_AFTER_INSTALL TRUE
)

