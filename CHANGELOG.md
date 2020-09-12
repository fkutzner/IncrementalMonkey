# Incremental Monkey Changelog

This changelog's format is based on [keep a changelog 1.0.0](https://keepachangelog.com/en/1.0.0/).

## [Unreleased]

### Added
- Added the `print-icnf` command, printing traces as ICNF problem instances
- Added Simplifier's Paradise, a random incremental SAT instance generator for particularly simplifiable problems

### Changed

- Renamed the `print` command to `print-cpp`

## [0.1.0] - 2020-09-07
### Added
- Added random-testing procedures for IPASIR SAT solvers
- Added CryptoMiniSat test oracle
- Added a random SAT instance generator based on the Community Attachment Model [1]
- Added extended random testing via `incmonk_havoc_init` and `incmonk_init`
- Added a configuration file parser


[1]: Jesús Giráldez-Cru and Jordi Levy. "A modularity-based random SAT instances generator.", IJCAI'15: Proceedings of the 24th International Conference on Artificial Intelligence
