# Copyright 2026 The OpenFst Authors.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# Explicitly list test sources

# Copy testdata to the build directory so tests can find it
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/testdata"
     DESTINATION "${CMAKE_CURRENT_BINARY_DIR}"
)

# Common libraries for tests
add_library(test_main STATIC test_main.cc)
target_include_directories(test_main
                           PRIVATE $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
)
target_link_libraries(
  test_main
  PUBLIC fst
         benchmark::benchmark
         GTest::gtest
         GTest::gmock
)

add_library(test_rand_fst INTERFACE)
target_include_directories(
  test_rand_fst INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

set(TESTS
    a-star_test
    accumulator_test
    add-on_test
    arc-arena_test
    arc-map_test
    arcsort_test
    cache_test
    cast_test
    closure_test
    compact_test
    compose_test
    concat_test
    connect_test
    determinize_test
    difference_test
    disambiguate_test
    edit-fst_test
    encode_test
    epsnormalize_test
    equal_test
    equivalent_test
    error_test
    expander-fst_test
    factor-weight_test
    float-weight_test
    fst-file_test
    fst_test
    generic_register_test
    heap_test
    inline-fst_test
    intersect_test
    interval-set_test
    invert_test
    isomorphic_test
    label-reachable_test
    lookahead_test
    mapped-file_test
    matcher_test
    memory_test
    minimize_test
    partition_test
    power-weight-mappers_test
    power-weight_test
    project_test
    prune_test
    push_test
    queue_test
    rand-fst_test
    randgen_test
    relabel_test
    replace_test
    reverse_test
    reweight_test
    rmepsilon_test
    rmfinalepsilon_test
    script_test
    shortest-distance_test
    shortest-path_test
    state-map_test
    state-reachable_test
    state-table_test
    string_test
    symbol-table-ops_test
    symbol-table_test
    synchronize_test
    thread_test
    topsort_test
    union-find_test
    union_test
    util_test
    vector-fst_test
    visit_test
    weight_test
)

foreach(TEST_NAME ${TESTS})
  add_executable(${TEST_NAME} ${TEST_NAME}.cc)
  set_target_properties(
    ${TEST_NAME}
    PROPERTIES
      CROSSCOMPILING_EMULATOR
        "${CMAKE_SOURCE_DIR}/scripts/run_with_test_tmpdir.sh"
  )
  target_include_directories(
    ${TEST_NAME} PRIVATE $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
                         $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  )

  set(TEST_MAIN_LIB test_main)
  if(TEST_NAME STREQUAL "error_test"
     OR TEST_NAME STREQUAL "fst_test"
     OR TEST_NAME STREQUAL "weight_test"
  )
    set(TEST_MAIN_LIB GTest::gtest)
  endif()

  target_link_libraries(
    ${TEST_NAME}
    PRIVATE fst
            fstscript
            ${TEST_MAIN_LIB}
            GTest::gmock
            absl::flags
            absl::flags_parse
            absl::flags_usage
            absl::log
            absl::status
            absl::statusor
            benchmark::benchmark
  )

  if(TEST_NAME STREQUAL "heap_test")
    target_link_libraries(
      ${TEST_NAME}
      PRIVATE absl::random_internal_randen
              absl::random_internal_randen_hwaes
              absl::random_internal_randen_slow
              absl::random_internal_seed_material
              absl::random_seed_sequences
    )
  endif()

  gtest_discover_tests(
    ${TEST_NAME}
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
  )
endforeach()

# algo_test variants
set(ALGO_TEST_WEIGHTS
    tropical
    log
    minmax
    left_string
    right_string
    gallic
    lexicographic
    power
)

foreach(WEIGHT ${ALGO_TEST_WEIGHTS})
  string(TOUPPER ${WEIGHT} WEIGHT_UPPER)
  set(VAR_TEST_NAME "algo_test_${WEIGHT}")
  add_executable(${VAR_TEST_NAME} algo_test.cc)
  set_target_properties(
    ${VAR_TEST_NAME}
    PROPERTIES
      CROSSCOMPILING_EMULATOR
        "${CMAKE_SOURCE_DIR}/scripts/run_with_test_tmpdir.sh"
  )
  target_include_directories(
    ${VAR_TEST_NAME} PRIVATE $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
                             $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  )
  target_compile_definitions(${VAR_TEST_NAME} PRIVATE "TEST_${WEIGHT_UPPER}")
  target_link_libraries(
    ${VAR_TEST_NAME}
    PRIVATE fst
            GTest::gtest
            GTest::gmock
            absl::flags
            absl::flags_parse
            absl::flags_usage
            absl::log
            absl::status
            absl::statusor
            absl::memory
  )
  # This could use `gtest_discover_tests`, but it's about 5s slower (74s vs
  # 69s) since there are hundreds of very small test cases.
  add_test(
    NAME ${VAR_TEST_NAME}
    COMMAND ${VAR_TEST_NAME}
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
  )
endforeach()
