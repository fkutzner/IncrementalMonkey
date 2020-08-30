#!/bin/bash
set -e

echo "Originally configured C++ compiler: ${CXX}"
echo "Originally configured C compiler: ${CC}"


host_os=`uname`
if [[ "${host_os}"  == "Linux" ]] && [[ "${CC}" =~ clang ]]
then
  export CC=clang-9
  export CXX=clang++-9
fi

echo "Tool versions:"
echo "Using C++ compiler: ${CXX}"
echo "Using C compiler: ${CC}"
cmake --version
clang --version
gcc --version
echo "(End tool versions)"

build_and_test() {
  echo "Compilation database:"
  cat compile_commands.json

  echo "Building..."
  cmake --build . -- -j2

  echo -n "Testing "
  bin/incmonktests.libincmonk.unit
}

BUILD_DIR=$(pwd)
build_cms() {
  mkdir cms
  pushd cms
  git clone --depth 1 --branch 5.8.0 https://github.com/msoos/cryptominisat
  pushd cryptominisat
  git submodule init && git submodule update
  popd
  mkdir cms-build
  cd cms-build
  cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="${BUILD_DIR}/cms/install" ../cryptominisat
  make -j2 install
  popd
}


echo "*** Building CryptoMiniSat"
build_cms
echo "*** Building IncrementalMonkey"
cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH="${BUILD_DIR}/cms/install" -DCMAKE_EXPORT_COMPILE_COMMANDS=ON ${TRAVIS_BUILD_DIR}
build_and_test
