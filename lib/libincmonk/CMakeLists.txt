nm_add_library(libincmonk STATIC
  CNF.h
  FuzzTrace.cpp
  FuzzTrace.h
  IPASIRSolver.cpp
  IPASIRSolver.h
)

target_include_directories(libincmonk PUBLIC ${PROJECT_SOURCE_DIR}/lib)
set_property(TARGET libincmonk PROPERTY OUTPUT_NAME incmonk)
