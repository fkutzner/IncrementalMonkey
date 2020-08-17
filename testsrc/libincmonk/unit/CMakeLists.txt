nm_add_tool(incmonktests.libincmonk.unit
  FuzzTraceTests.cpp
)

target_link_libraries(incmonktests.libincmonk.unit PRIVATE libincmonk gtest gmock gtest_main)
