nm_add_library(libincmonk STATIC
  CNF.h
  Config.cpp
  Config.h
  ConfigTomlUtils.cpp
  ConfigTomlUtils.h
  FastRand.h
  Fork.h
  Fork.cpp
  FuzzTrace.cpp
  FuzzTrace.h
  FuzzTraceExec.cpp
  FuzzTraceExec.h
  FuzzTracePrinters.cpp
  FuzzTracePrinters.h
  InterspersionSchedulers.cpp
  InterspersionSchedulers.h
  IPASIRSolver.cpp
  IPASIRSolver.h
  Oracle.h
  OracleCMS.cpp
  StochasticsUtils.cpp
  StochasticsUtils.h
  TBool.h

  generators/CommunityAttachmentGenerator.cpp
  generators/CommunityAttachmentGenerator.h
  generators/FuzzTraceGenerator.h
  generators/MuxGenerator.cpp
  generators/MuxGenerator.h
  generators/SimplifiersParadiseGenerator.cpp
  generators/SimplifiersParadiseGenerator.h

  verifier/Assignment.cpp
  verifier/Assignment.h
  verifier/BoundedMap.h
  verifier/Clause.cpp
  verifier/Clause.h
  verifier/ClauseImpl.h
  verifier/Propagation.cpp
  verifier/Propagation.h
  verifier/Traits.h
)

target_include_directories(libincmonk PUBLIC ${PROJECT_SOURCE_DIR}/lib)
target_link_libraries(libincmonk PRIVATE deps_gsl deps_cms deps_tomlxx deps_dl deps_hopscotchmap)
set_property(TARGET libincmonk PROPERTY OUTPUT_NAME incmonk)
