nm_add_library(libincmonk STATIC
  CNF.h
  FuzzTrace.cpp
  FuzzTrace.h
  IPASIRSolver.cpp
  IPASIRSolver.h
  Oracle.h
  OracleCMS.cpp
)

target_include_directories(libincmonk PUBLIC ${PROJECT_SOURCE_DIR}/lib)
target_link_libraries(libincmonk PRIVATE deps_gsl deps_cms)
set_property(TARGET libincmonk PROPERTY OUTPUT_NAME incmonk)
