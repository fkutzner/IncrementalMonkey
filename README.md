# Incremental Monkey 🐒

A random testing tool for IPASIR SAT solvers

## Usage

Incremental Monkey can be used as a standalone tool or
as a an oracle-equipped fuzzing target for other fuzzers, e.g.
`afl-fuzz` or `honggfuzz`.

### Standalone mode

Compile your solver as a shared library exposing the IPASIR 
interface (for instance `solver.so`) and run
```
# monkey fuzz solver.so
```

For each solver crash or failure to produce a correct result,
`monkey` creates a file `monkey-<id>-<runNumber>-crashed.mtr`
rsp. `monkey-<id>-<runNumber>-<failureType>.mtr` in the current working
directory. These files contain traces of IPASIR calls inducing the
undesired behavior. Currently, `monkey` does not distinguish failures
in `solver.so` from test oracle failures. If you suspect a test
oracle failure, you can run the trace on a different
IPASIR implementation using the `replay` command (see below).

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
If the trace was dumped in a run where the test oracle rejected
the tested solver's result, the `ipasir_solve` calls are equipped
with assertions checking the result. In crash traces, oracle data
is lost and no assertions are inserted. In traces with failure
type `invalidmodel` and `invalidfailed`, assertions checking the
`ipasir_solve` call results are inserted, but the invalid model
rsp. invalid failed assumption settings are not checked in the
C++ program.


### Fuzzing target mode

You can hook up Incremental Monkey to other fuzzers by using the `replay`
command, e.g.

```
# afl-fuzz -i corpus -o fuzz-out -- bin/monkey replay --parse-permissive --crash-on-failure path/to/your/solver.so -
```

If you are using code instrumentation (eg. via `afl-clang`), make
sure to **preload the IPASIR library** by adding its full path to the
environment variable `LD_PRELOAD` rsp. `DYLD_INSERT_LIBRARIES`. You
should also compile Incremental Monkey with your fuzzer's instrumentation.

### Broken example solvers

* `lib/libcrashing-ipasir-solver.so`: randomly crashes
* `lib/libincorrect-ipasir-solver.so`: randomly produces wrong results

## Advanced testing

To perform option fuzzing and to obtain shorter error traces, you can
trigger e.g. extra simplifications between `ipasir_solve` calls by
implementing the following two functions:

* `incmonk_havoc(void* ipasirSolver, uint64_t randomVal)` - this function is called at
   random as part of the trace execution, with changing values of `randomVal`.
* `incmonk_havoc_init(uint64_t randomVal)` - this function is called before the
   solver under test is created.

Implementing these two functions is **optional**. `monkey` determines their presence
on startup and disables `incmonk_havoc` and `incmonk_havoc_init` calls when they
are not both supported by the IPASIR library.


## Supported platforms

Incremental monkey is regularly built and tested on **MacOS 10.15** and
**Ubuntu 20.04-LTS**. If you wish to build Incremental Monkey with Clang
on Ubuntu, make sure to upgrade to Clang 9. On either system, you'll need
at least CMake 3.12.

### Other platforms

Requirements:

* GCC 8 or Clang 9
* libstdc++ 8 or libc++ 7
* CMake 3.12
* POSIX

Incremental Monkey might also be usable on Cygwin, but as it relies
on rapidly `fork()`ing subprocesses, it might only work well when
generating harder problem instances.


## Building Incremental Monkey

The simplest way of building Incremental Monkey is to use the CMake
"superbuild", which will also build CryptoMiniSat. To build and install
Incremental Monkey to a directory `<INSTALLDIR>`, run the following steps:

```
git clone https://github.com/fkutzner/IncrementalMonkey
cd IncrementalMonkey
git submodule init && git submodule update
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=<INSTALLDIR> ../buildscript
cmake --build .
```

Run `monkey`:
```
# cd <INSTALLDIR>
# bin/monkey --help
```

