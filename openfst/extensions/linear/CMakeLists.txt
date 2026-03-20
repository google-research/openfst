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

add_library(fstlinearscript linearscript.cc)
set_target_properties(
  fstlinearscript
  PROPERTIES SOVERSION ${OPENFST_SOVERSION}
)

target_include_directories(
  fstlinearscript PUBLIC $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
                         $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_link_libraries(
  fstlinearscript
  PUBLIC fst
         fstscript
         absl::log
         absl::flags
)

add_library(fstlinear_tagger_fst linear-tagger-fst.cc)
set_target_properties(
  fstlinear_tagger_fst
  PROPERTIES SOVERSION ${OPENFST_SOVERSION}
)
target_link_libraries(fstlinear_tagger_fst PUBLIC fst)

add_library(fstlinear_classifier_fst linear-classifier-fst.cc)
set_target_properties(
  fstlinear_classifier_fst
  PROPERTIES SOVERSION ${OPENFST_SOVERSION}
)
target_link_libraries(fstlinear_classifier_fst PUBLIC fst)

add_library(fstlinear-main fstlinear-main.cc)
target_link_libraries(fstlinear-main PUBLIC fstlinearscript absl::flags
                                            absl::log
)

add_executable(fstlinear fstlinear.cc)
target_link_libraries(fstlinear PRIVATE fstlinear-main)

add_library(fstloglinearapply-main fstloglinearapply-main.cc)
target_link_libraries(fstloglinearapply-main PUBLIC fst absl::flags absl::log)

add_executable(fstloglinearapply fstloglinearapply.cc)

target_link_libraries(fstloglinearapply PRIVATE fstloglinearapply-main)

openfst_install(
  TARGETS fstlinearscript fstlinear_tagger_fst fstlinear_classifier_fst
  EXPORT OpenFstTargets
  DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

openfst_install(TARGETS fstlinear fstloglinearapply
                RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

set(LINEAR_HEADERS
    linear-fst-data-builder.h
    linear-fst-data.h
    linear-fst.h
    linearscript.h
    loglinear-apply.h
    trie.h
)

openfst_install(
  FILES ${LINEAR_HEADERS}
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/openfst/extensions/linear
)

if(OPENFST_BUILD_TESTS)
  include(GoogleTest)

  add_executable(linear-fst-data_test linear-fst-data_test.cc)
  set_target_properties(
    linear-fst-data_test
    PROPERTIES
      CROSSCOMPILING_EMULATOR
        "${CMAKE_SOURCE_DIR}/scripts/run_with_test_tmpdir.sh"
  )
  target_link_libraries(linear-fst-data_test PRIVATE fst test_main absl::log)
  gtest_discover_tests(
    linear-fst-data_test
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
  )

  add_executable(linear-fst_test linear-fst_test.cc)
  set_target_properties(
    linear-fst_test
    PROPERTIES
      CROSSCOMPILING_EMULATOR
        "${CMAKE_SOURCE_DIR}/scripts/run_with_test_tmpdir.sh"
  )
  target_link_libraries(
    linear-fst_test
    PRIVATE fstlinear_tagger_fst
            fstlinear_classifier_fst
            fst
            test_main
  )
  gtest_discover_tests(
    linear-fst_test
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
  )

  add_executable(trie_test trie_test.cc)
  set_target_properties(
    trie_test
    PROPERTIES
      CROSSCOMPILING_EMULATOR
        "${CMAKE_SOURCE_DIR}/scripts/run_with_test_tmpdir.sh"
  )
  target_link_libraries(
    trie_test
    PRIVATE fst
            test_main
            absl::strings
            absl::flat_hash_map
  )
  gtest_discover_tests(
    trie_test
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
  )
endif()
