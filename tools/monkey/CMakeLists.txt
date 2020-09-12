nm_add_tool(monkey
  Fuzz.cpp
  Fuzz.h
  IncrementalMonkey.cpp
  PrintCPP.cpp
  PrintCPP.h
  PrintICNF.cpp
  PrintICNF.h
  Replay.cpp
  Replay.h
  Utils.cpp
  Utils.h
)

target_compile_definitions(monkey PRIVATE "INCMONK_VERSION=\"${CMAKE_PROJECT_VERSION}\"")
target_link_libraries(monkey PRIVATE deps_cli11 libincmonk)
