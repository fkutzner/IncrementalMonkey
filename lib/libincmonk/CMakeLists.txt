nm_add_library(libincmonk STATIC
  CNF.h
  FastRand.h
  Fork.h
  Fork.cpp
  FuzzTrace.cpp
  FuzzTrace.h
  FuzzTraceExec.cpp
  FuzzTraceExec.h
  IPASIRSolver.cpp
  IPASIRSolver.h
  Oracle.h
  OracleCMS.cpp
  SolveCmdScheduler.cpp
  SolveCmdScheduler.h

  generators/CommunityAttachmentGenerator.cpp
  generators/CommunityAttachmentGenerator.h
  generators/FuzzTraceGenerator.h
  generators/RandomSATGenerator.cpp
  generators/RandomSATGenerator.h
)

target_include_directories(libincmonk PUBLIC ${PROJECT_SOURCE_DIR}/lib)
target_link_libraries(libincmonk PRIVATE deps_gsl deps_cms deps_tomlxx)
set_property(TARGET libincmonk PROPERTY OUTPUT_NAME incmonk)
