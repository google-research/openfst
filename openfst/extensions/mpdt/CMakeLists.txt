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

add_library(fstmpdtscript mpdtscript.cc)
set_target_properties(
  fstmpdtscript
  PROPERTIES SOVERSION ${OPENFST_SOVERSION}
)

target_include_directories(
  fstmpdtscript PUBLIC $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
                       $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_link_libraries(fstmpdtscript PUBLIC fstscript fstpdtscript absl::log)

set(MPDT_BINS
    mpdtcompose
    mpdtexpand
    mpdtinfo
    mpdtreverse
)

foreach(BIN ${MPDT_BINS})

  add_library(${BIN}-main ${BIN}-main.cc)

  target_link_libraries(${BIN}-main PUBLIC fstmpdtscript absl::flags absl::log)

  add_executable(${BIN} ${BIN}.cc)

  target_link_libraries(${BIN} PRIVATE ${BIN}-main)

  openfst_install(TARGETS ${BIN} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

endforeach()

openfst_install(
  TARGETS fstmpdtscript
  EXPORT OpenFstTargets
  DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

set(MPDT_HEADERS
    compose.h
    expand.h
    info.h
    mpdt.h
    mpdtlib.h
    mpdtscript.h
    read_write_utils.h
    reverse.h
)

openfst_install(FILES ${MPDT_HEADERS}
                DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/openfst/extensions/mpdt
)

if(OPENFST_BUILD_TESTS)
  include(GoogleTest)

  file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/testdata"
       DESTINATION "${CMAKE_CURRENT_BINARY_DIR}"
  )

  set(MPDT_TESTS
      compose_test
      expand_test
      mpdt_test
      reverse_test
  )

  foreach(TEST_NAME ${MPDT_TESTS})
    add_executable(mpdt_${TEST_NAME} ${TEST_NAME}.cc)
    set_target_properties(
      mpdt_${TEST_NAME}
      PROPERTIES
        CROSSCOMPILING_EMULATOR
          "${CMAKE_SOURCE_DIR}/scripts/run_with_test_tmpdir.sh"
    )
    target_include_directories(
      mpdt_${TEST_NAME} PRIVATE $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
                                $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    )
    target_link_libraries(
      mpdt_${TEST_NAME}
      PRIVATE fst
              test_main
              GTest::gtest
              GTest::gmock
              absl::flags
              absl::log
    )
    gtest_discover_tests(
      mpdt_${TEST_NAME}
      WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
    )
  endforeach()
endif()
