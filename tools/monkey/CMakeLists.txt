nm_add_tool(monkey
  Fuzz.cpp
  Fuzz.h
  GenTrace.h
  GenTrace.cpp
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


if (IM_IPASIR_LIB)
  message(STATUS "Linking to IPASIR library: ${IM_IPASIR_LIB}. When invoking monkey, pass \"preloaded\" as the IPASIR DSO name to use this library.")
  target_link_libraries(monkey PRIVATE ${IM_IPASIR_LIB})
  target_compile_definitions(monkey PRIVATE IM_LINKTIME_IPASIR)
endif()
