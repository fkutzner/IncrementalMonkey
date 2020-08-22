
function(add_faulty_ipasir_lib NAME)
  add_library(${NAME} SHARED FaultyIPASIRSolver.cpp)
  target_link_libraries(${NAME} deps_ipasir deps_cms)
  target_compile_definitions(${NAME} PRIVATE IPASIR_SHARED_LIB BUILDING_IPASIR_SHARED_LIB)
  target_compile_options(${NAME} PRIVATE -fvisibility=hidden)
endfunction()


add_faulty_ipasir_lib(incorrect-ipasir-solver)
target_compile_definitions(incorrect-ipasir-solver PRIVATE INJECT_CORRECTNESS_FAULT)
add_faulty_ipasir_lib(crashing-ipasir-solver)
target_compile_definitions(crashing-ipasir-solver PRIVATE INJECT_SEG_FAULT)

