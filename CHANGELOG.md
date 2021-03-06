# Incremental Monkey Changelog

This changelog's format is based on [keep a changelog 1.0.0](https://keepachangelog.com/en/1.0.0/).

## [Unreleased]

### Fixed
- When no initial random seed is specified explicitly, the seed was set to 10. It is chosen randomly now.

## [0.2.0] - 2020-09-17

### Added
- Added the `print-icnf` command, printing traces as ICNF problem instances
- Added Simplifier's Paradise, a random incremental SAT instance generator for particularly simplifiable problems
- Trace-reading commands now read the trace from the standard input when `-` is specified as the trace filename.
- Added the `gen-trace` command for generating random traces
- Added `--parse-permissive` parsing mode for traces
- `monkey` now performs IPASIR lookups within the current process' own symbols if the specified DSO name is `preloaded`
- Added support for dynamically linking a fixed IPASIR solver to `monkey` via the `IM_IPASIR_LIB` build option
- Added `--crash-on-failure` failure mode for `monkey replay`

### Changed

- Renamed the `print` command to `print-cpp`
- Made the failure trace format more fuzzer-friendly
- New configuration defaults: generating more smaller instances

## [0.1.0] - 2020-09-07
### Added
- Added random-testing procedures for IPASIR SAT solvers
- Added CryptoMiniSat test oracle
- Added a random SAT instance generator based on the Community Attachment Model [1]
- Added extended random testing via `incmonk_havoc_init` and `incmonk_init`
- Added a configuration file parser


[1]: Jesús Giráldez-Cru and Jordi Levy. "A modularity-based random SAT instances generator.", IJCAI'15: Proceedings of the 24th International Conference on Artificial Intelligence
