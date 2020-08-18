nm_add_library(libincmonk STATIC
  CNF.h
  FuzzTrace.cpp
  FuzzTrace.h
  FuzzTraceExec.cpp
  FuzzTraceExec.h
  IPASIRSolver.cpp
  IPASIRSolver.h
  Oracle.h
  OracleCMS.cpp

  generators/FuzzTraceGenerator.h
  generators/GiraldezLevyGenerator.cpp
  generators/GiraldezLevyGenerator.h
  generators/RandomSATGenerator.cpp
  generators/RandomSATGenerator.h
)

target_include_directories(libincmonk PUBLIC ${PROJECT_SOURCE_DIR}/lib)
target_link_libraries(libincmonk PRIVATE deps_gsl deps_cms)
set_property(TARGET libincmonk PROPERTY OUTPUT_NAME incmonk)
