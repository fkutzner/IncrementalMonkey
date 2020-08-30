nm_add_tool(monkey
  Fuzz.cpp
  Fuzz.h
  IncrementalMonkey.cpp
  Print.cpp
  Print.h
  Replay.cpp
  Replay.h
)

target_compile_definitions(monkey PRIVATE "INCMONK_VERSION=\"${CMAKE_PROJECT_VERSION}\"")
target_link_libraries(monkey PRIVATE deps_cli11 libincmonk)
