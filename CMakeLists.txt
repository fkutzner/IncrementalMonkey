# Copyright (c) 2020 Felix Kutzner (github.com/fkutzner)
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#
# Except as contained in this notice, the name(s) of the above copyright holders
# shall not be used in advertising or otherwise to promote the sale, use or
# other dealings in this Software without prior written authorization.

cmake_minimum_required(VERSION 3.12)
cmake_policy(VERSION 3.12)

project(IncrementalMonkey VERSION 0.1.0)

option(IM_ENABLE_COVERAGE OFF "Enable code coverage instrumentation")
option(IM_IPASIR_LIB "" "(Optional) IPASIR shared library, for early linking")

### NiceMake setup
set(NM_CONF_GTEST_TAG "release-1.10.0")
set(NM_OPT_PREFIX "IM")
include(etc/cmake/nicemake/NiceMake.cmake)


### Compiler & linker configuration
include(etc/cmake/CompileOptions.cmake)


### Build dependencies
add_subdirectory(deps)


## Build Incremental Monkey
add_subdirectory(lib)
add_subdirectory(tools)


### Testing
nm_add_gtest()
enable_testing()
add_subdirectory(testsrc)


nm_add_doxygen()
include(etc/cmake/HelperTargets.cmake)


### Installation
set(INCMONK_PKG_NAME "IncrementalMonkey")

install(
  TARGETS monkey
  EXPORT ${INCMONK_PKG_NAME}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
)

install(
  FILES
    ${CMAKE_CURRENT_LIST_DIR}/LICENSE
    ${CMAKE_CURRENT_LIST_DIR}/LICENSES-3rdParty
    ${CMAKE_CURRENT_LIST_DIR}/README.md
  DESTINATION
    share/doc/${INCMONK_PKG_NAME}
)

# Generate and install CMake scripts for find_package
install(EXPORT ${INCMONK_PKG_NAME}
        DESTINATION lib/cmake/${INCMONK_PKG_NAME}
        FILE "${INCMONK_PKG_NAME}Config.cmake")
