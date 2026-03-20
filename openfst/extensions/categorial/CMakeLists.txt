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

set(CATEGORIAL_HEADERS categorial-weight.h)

openfst_install(
  FILES ${CATEGORIAL_HEADERS}
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/openfst/extensions/categorial
)

if(OPENFST_BUILD_TESTS)
  include(GoogleTest)
  add_executable(categorial_weight_test weight_test.cc)
  target_link_directories(categorial_weight_test
                          PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}"
  )
  target_include_directories(categorial_weight_test
                             PRIVATE $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
                                     $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/test>
  )
  target_link_libraries(
    categorial_weight_test
    PRIVATE fst
            GTest::gtest
            GTest::gmock
            absl::flags
            absl::log
  )
  gtest_discover_tests(
    categorial_weight_test
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
    DISCOVERY_TIMEOUT 60
  )
endif()
