nm_add_tool(monkey
  Fuzz.cpp
  Fuzz.h
  IncrementalMonkey.cpp
  Replay.cpp
  Replay.h
)

target_link_libraries(monkey deps_cli11 libincmonk)
