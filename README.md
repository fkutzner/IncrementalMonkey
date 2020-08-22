# Incremental Monkey 🐒

A random testing tool for IPASIR SAT solvers

## Usage

Compile your solver as a shared library exposing the IPASIR 
interface (for instance `solver.so`) and run
```
# monkey fuzz solver.so
```

For each solver crash or failure to produce a correct result,
`monkey` creates a file `monkey-<id>-<runNumber>-crashed.mtr`
rsp. `monkey-<id>-<runNumber>-incorrect.mtr` in the current working
directory. These files contain traces of IPASIR calls inducing the
undesired behavior. Note that `monkey` does not distinguish failures
in `solver.so` from test oracle failures.

The testing process can be customized in a number of ways
(test instance generation parameters, timeout, execution limits, ...).
Run `monkey fuzz --help` for more details.

To apply a single trace file to your solver, run
```
# monkey replay solver.so monkey-m01-crashed.mtr
```
This command does not execute the solver in subprocesses and can
easily be debugged.

Making regression test cases out of `monkey` traces is easy:
```
# monkey print --function-name foonction monkey-m01-crashed.mtr
```
prints `monkey-m01-crashed.mtr` as a C++11 function `foonction`.

### Broken example solvers

* `lib/libcrashing-ipasir-solver.so`: randomly crashes
* `lib/libincorrect-ipasir-solver.so`: randomly produces wrong results

## Building

Currently, Incremental Monkey can be used on macOS (10.15 and later)
and Linux. You'll need a not-so-ancient compiler and C++ standard
library (supporting C++17 - GCC 8 or Clang 10 will do),  `git` and 
`cmake` (>= 3.12). It might also be usable on Cygwin, but as it relies
on rapidly `fork()`ing subprocesses, running it on Cygwin might only work
well when generating harder problem instances.

Build summary: build `https://github.com/fkutzner/IncrementalMonkey` with
a CryptoMiniSat 5.8.0 installation in your `CMAKE_PREFIX_PATH`.

### Step-by-step build guide

Build CryptoMiniSat:

```
# git clone --depth 1 --branch 5.8.0 https://github.com/msoos/cryptominisat
# mkdir cms-build
# cd cms-build
# cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_INSTALL_PREFIX=(pwd)/../cms-install ../cryptominisat
# cmake --build . --target install
```

Build Incremental Monkey:
```
# git clone https://github.com/fkutzner/IncrementalMonkey
# cd IncrementalMonkey
# git submodule init
# git submodule update
# cd ..
# mkdir monkey-build
# cd monkey-build
# cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_PREFIX_PATH=(pwd)/../cms-install ../IncrementalMonkey
# cmake --build .
```

Run:
```
# bin/monkey --help
```



