#!/bin/bash
set -e

echo "Originally configured C++ compiler: ${CXX}"
echo "Originally configured C compiler: ${CC}"


host_os=`uname`
if [[ "${host_os}"  == "Linux" ]] && [[ "${CC}" =~ clang ]]
then
  export CC=clang-10
  export CXX=clang++-10
fi

echo "Tool versions:"
echo "Using C++ compiler: ${CXX}"
echo "Using C compiler: ${CC}"
cmake --version
clang --version
gcc --version
echo "(End tool versions)"

# Run the CMake superbuild. This will also execute the unit tests.
cmake -DCMAKE_BUILD_TYPE=Release ${TRAVIS_BUILD_DIR}/buildscript
cmake --build . -- -j2

