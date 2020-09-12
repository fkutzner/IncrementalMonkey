
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

add_faulty_ipasir_lib(havoc-supporting-ipasir-solver)
target_compile_definitions(havoc-supporting-ipasir-solver PRIVATE ENABLE_HAVOC_INTERFACE)

add_faulty_ipasir_lib(knowngood-ipasir-solver)


function(add_ipasir_faults_test)
  set(options)
  set(oneValueArgs NAME LIB_TARGET FAIL_REGEX PASS_REGEX)
  set(multiValueArgs MONKEY_CLI_ARGS)
  cmake_parse_arguments(ARGP "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  add_test(
    NAME "${ARGP_NAME}" COMMAND monkey ${ARGP_MONKEY_CLI_ARGS} --config "${CMAKE_CURRENT_LIST_DIR}/easy-problems.cfg"  $<TARGET_FILE:${ARGP_LIB_TARGET}>
  )
  
  if (ARGP_FAIL_REGEX)
    set_tests_properties("${ARGP_NAME}" PROPERTIES FAIL_REGULAR_EXPRESSION "${ARGP_FAIL_REGEX}")
  endif()
  if (ARGP_PASS_REGEX)
    set_tests_properties("${ARGP_NAME}" PROPERTIES PASS_REGULAR_EXPRESSION "${ARGP_PASS_REGEX}")
  endif()
endfunction()


add_ipasir_faults_test(
  NAME incmonktests.monkey.acceptance.ipasir-faults.no_traces_generated_for_known_good_solver
  MONKEY_CLI_ARGS fuzz --rounds=10 --seed=10 --no-havoc
  LIB_TARGET havoc-supporting-ipasir-solver
  PASS_REGEX "Generated error traces: 0"
)

add_ipasir_faults_test(
  NAME incmonktests.monkey.acceptance.ipasir-faults.traces_generated_for_crashing_solver
  MONKEY_CLI_ARGS fuzz --rounds=10 --seed=10 --no-havoc
  LIB_TARGET crashing-ipasir-solver
  FAIL_REGEX "Detected crashes: 0"
)

add_ipasir_faults_test(
  NAME incmonktests.monkey.acceptance.ipasir-faults.traces_generated_for_incorrect_solver
  MONKEY_CLI_ARGS fuzz --rounds=10 --seed=10 --no-havoc
  LIB_TARGET incorrect-ipasir-solver
  FAIL_REGEX "Detected correctness failures: 0"
)

add_ipasir_faults_test(
  NAME incmonktests.monkey.acceptance.ipasir-faults.havoc_disabled_on_plain_ipasir
  MONKEY_CLI_ARGS fuzz --rounds=1
  LIB_TARGET knowngood-ipasir-solver
  FAIL_REGEX "Havoc: enabled"
)

add_ipasir_faults_test(
  NAME incmonktests.monkey.acceptance.ipasir-faults.havoc_enabled_by_default_on_extended_solver
  MONKEY_CLI_ARGS fuzz --rounds=1
  LIB_TARGET havoc-supporting-ipasir-solver
  FAIL_REGEX "Havoc: disabled"
)

add_ipasir_faults_test(
  NAME incmonktests.monkey.acceptance.ipasir-faults.havoc_disabled_by_nohavoc_option
  MONKEY_CLI_ARGS fuzz --rounds=1 --no-havoc
  LIB_TARGET havoc-supporting-ipasir-solver
  FAIL_REGEX "Havoc: enabled"
)


