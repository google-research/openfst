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

set(COMPAT_HEADERS
    checksummer.h
    compat.h
    compat_memory.h
    file_path.h
    init.h
    seed_sequences.h
    status_macros.h
    unaligned.h
)
openfst_install(FILES ${COMPAT_HEADERS}
                DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/openfst/compat
)

if(OPENFST_BUILD_TESTS)
  include(GoogleTest)

  set(COMPAT_TESTS
      compat_memory_test
      file_path_test
      status_macros_test
      init_test
      unaligned_test
  )

  foreach(TEST_NAME ${COMPAT_TESTS})
    add_executable(${TEST_NAME} ${TEST_NAME}.cc)
    set_target_properties(
      ${TEST_NAME}
      PROPERTIES
        CROSSCOMPILING_EMULATOR
          "${CMAKE_SOURCE_DIR}/scripts/run_with_test_tmpdir.sh"
    )
    target_link_libraries(
      ${TEST_NAME}
      PRIVATE fst
              test_main
              GTest::gtest
              GTest::gmock
              absl::strings
              absl::status
              absl::statusor
              absl::log
              absl::base
    )
    gtest_discover_tests(
      ${TEST_NAME}
      WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
    )
  endforeach()
endif()
