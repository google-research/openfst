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

# Copy testdata to the build directory so benchmarks can find it
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/testdata"
     DESTINATION "${CMAKE_CURRENT_BINARY_DIR}"
)

set(BENCHMARKS
    compact_test
    compose_test
    connect_test
    determinize_test
    fst_test
    memory_test
    minimize_test
    serialize_test
    shortest_path_test
    symbol-table_test
    weight_test
)

foreach(BENCHMARK_SRC ${BENCHMARKS})
  # CMake target names must be globally unique across the entire project. Since
  # many benchmarks share filenames with the tests in openfst/test (e.g.,
  # compact_test.cc), we append a suffix to the target name.
  set(BENCHMARK_TARGET "${BENCHMARK_SRC}_benchmark")
  add_executable(${BENCHMARK_TARGET} ${BENCHMARK_SRC}.cc benchmain_main.cc)

  # Set the output filename to match the source name (e.g., 'compact_test') so
  # the binary on disk does not have the suffix.
  set_target_properties(
    ${BENCHMARK_TARGET}
    PROPERTIES OUTPUT_NAME ${BENCHMARK_SRC}
  )

  target_include_directories(${BENCHMARK_TARGET}
                             PRIVATE $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
  )

  target_link_libraries(
    ${BENCHMARK_TARGET}
    PRIVATE fst
            benchmark::benchmark
            GTest::gtest
            absl::flags
            absl::flags_usage
            absl::log
            absl::log_initialize
            absl::die_if_null
            absl::check
            absl::memory
            absl::strings
            absl::random_random
  )

  if(BENCHMARK_SRC STREQUAL "fst_test" OR BENCHMARK_SRC STREQUAL "compose_test")
    if(OPENFST_ENABLE_NGRAM_FSTS)
      target_link_libraries(${BENCHMARK_TARGET} PRIVATE fstngram)
    else()
      message(
        STATUS "NGram FSTs disabled, skipping benchmark ${BENCHMARK_TARGET}"
      )
      set_target_properties(
        ${BENCHMARK_TARGET}
        PROPERTIES EXCLUDE_FROM_ALL TRUE
      )
    endif()
  endif()
endforeach()
