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



### Broken example solvers

* `lib/libcrashing-ipasir-solver.so`: randomly crashes
* `lib/libincorrect-ipasir-solver.so`: randomly produces wrong results

## Advanced testing

To perform option fuzzing and to obtain shorter error traces, you can
trigger e.g. extra simplifications between `ipasir_solve` calls by
implementing the following two functions:

* `ipasir_havoc(void* ipasirSolver, uint64_t randomVal)` - this function is called at
   random as part of the trace execution, with changing values of `randomVal`.
* `ipasir_havoc_init(uint64_t randomVal)` - this function is called before the
   solver under test is created.

Implementing these two functions is **optional**. `monkey` determines their presence
on startup and disables `ipasir_havoc` and `ipasir_havoc_init` calls when they
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

**Summary:** build `https://github.com/fkutzner/IncrementalMonkey` with
a CryptoMiniSat 5.8.0 installation in your `CMAKE_PREFIX_PATH`.

### Step-by-step build guide

In the following snippets, `<INSTALLDIR>` is an arbitrary directory
where you want `bin/monkey` to end up (e.g. `~/tools` or
`/usr/local`).

Build CryptoMiniSat:

```
# git clone --depth 1 --branch 5.8.0 https://github.com/msoos/cryptominisat
# mkdir cms-build
# cd cms-build
# cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_INSTALL_PREFIX=<INSTALLDIR> ../cryptominisat
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
# cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_PREFIX_PATH=<INSTALLDIR> ../IncrementalMonkey
# cmake --build . --target install
```

Run `monkey`:
```
# cd <INSTALLDIR>
# bin/monkey --help
```



