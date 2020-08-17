nm_add_tool(incmonktests.libincmonk.unit
  FileUtils.cpp
  FileUtils.h
  FuzzTraceTests.cpp
)

target_link_libraries(incmonktests.libincmonk.unit PRIVATE libincmonk deps_gsl gtest gmock gtest_main)
