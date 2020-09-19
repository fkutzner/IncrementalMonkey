nm_add_tool(incmonktests.libincmonk.unit
  ConfigTests.cpp
  ConfigTomlUtilsTests.cpp
  FileUtils.cpp
  FileUtils.h
  ForkTests.cpp
  FuzzTraceExecTests.cpp
  FuzzTracePrintersTests.cpp
  FuzzTraceTests.cpp
  MuxGeneratorTests.cpp
  OracleTests.cpp

  verifier/AssignmentTests.cpp
  verifier/BoundedMapTests.cpp
  verifier/ClauseTests.cpp
)

target_link_libraries(incmonktests.libincmonk.unit PRIVATE
  libincmonk
  deps_gsl
  deps_tomlxx
  gtest
  gmock
  gtest_main
)

add_test(NAME incmonktests.libincmonk.unit COMMAND incmonktests.libincmonk.unit)