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

add_library(fstphi-fst phi-fst.cc)
set_target_properties(
  fstphi-fst
  PROPERTIES SOVERSION ${OPENFST_SOVERSION}
)
target_include_directories(
  fstphi-fst PUBLIC $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
                    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_link_libraries(
  fstphi-fst
  PUBLIC fst
         absl::log
         absl::memory
         absl::flags
)

add_library(fstrho-fst rho-fst.cc)
set_target_properties(
  fstrho-fst
  PROPERTIES SOVERSION ${OPENFST_SOVERSION}
)
target_include_directories(
  fstrho-fst PUBLIC $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
                    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_link_libraries(
  fstrho-fst
  PUBLIC fst
         absl::log
         absl::memory
         absl::flags
)

add_library(fstsigma-fst sigma-fst.cc)
set_target_properties(
  fstsigma-fst
  PROPERTIES SOVERSION ${OPENFST_SOVERSION}
)
target_include_directories(
  fstsigma-fst PUBLIC $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
                      $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_link_libraries(
  fstsigma-fst
  PUBLIC fst
         absl::log
         absl::memory
         absl::flags
)

add_executable(fstspecial fstspecial.cc)

# It depends on fstconvert-main which is in openfst/bin

target_link_libraries(
  fstspecial
  PRIVATE fstphi-fst
          fstrho-fst
          fstsigma-fst
          fstconvert-main
          absl::flags
          absl::log
)

openfst_install(
  TARGETS fstphi-fst fstrho-fst fstsigma-fst
  EXPORT OpenFstTargets
  DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

openfst_install(TARGETS fstspecial RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

set(SPECIAL_HEADERS
    phi-fst.h
    rho-fst.h
    sigma-fst.h
)

openfst_install(
  FILES ${SPECIAL_HEADERS}
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/openfst/extensions/special
)

if(OPENFST_BUILD_TESTS)
  include(GoogleTest)
  file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/testdata"
       DESTINATION "${CMAKE_CURRENT_BINARY_DIR}"
  )

  add_executable(phi-fst_test phi-fst_test.cc)
  set_target_properties(
    phi-fst_test
    PROPERTIES
      CROSSCOMPILING_EMULATOR
        "${CMAKE_SOURCE_DIR}/scripts/run_with_test_tmpdir.sh"
  )
  target_link_libraries(
    phi-fst_test
    PRIVATE fstphi-fst
            fst
            test_main
            absl::flags
  )
  gtest_discover_tests(
    phi-fst_test
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
  )

  add_executable(rho-fst_test rho-fst_test.cc)
  set_target_properties(
    rho-fst_test
    PROPERTIES
      CROSSCOMPILING_EMULATOR
        "${CMAKE_SOURCE_DIR}/scripts/run_with_test_tmpdir.sh"
  )
  target_link_libraries(
    rho-fst_test
    PRIVATE fstrho-fst
            fst
            test_main
            absl::flags
  )
  gtest_discover_tests(
    rho-fst_test
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
  )

  add_executable(sigma-fst_test sigma-fst_test.cc)
  set_target_properties(
    sigma-fst_test
    PROPERTIES
      CROSSCOMPILING_EMULATOR
        "${CMAKE_SOURCE_DIR}/scripts/run_with_test_tmpdir.sh"
  )
  target_link_libraries(
    sigma-fst_test
    PRIVATE fstsigma-fst
            fst
            test_main
            absl::flags
  )
  gtest_discover_tests(
    sigma-fst_test
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
  )
endif()
