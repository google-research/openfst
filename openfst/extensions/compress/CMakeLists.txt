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

add_library(fstcompress_elias INTERFACE)
target_include_directories(
  fstcompress_elias INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

add_library(fstcompress_compress INTERFACE)
target_include_directories(
  fstcompress_compress INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)
target_link_libraries(
  fstcompress_compress
  INTERFACE fst
            fstcompress_elias
            absl::log
            absl::memory
            absl::strings
            absl::span
)

add_library(fstcompressscript compressscript.cc)
set_target_properties(
  fstcompressscript
  PROPERTIES SOVERSION ${OPENFST_SOVERSION}
)

target_include_directories(
  fstcompressscript PUBLIC $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
                           $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_link_libraries(fstcompressscript PUBLIC fstscript fstcompress_compress)

add_library(fstcompress-main fstcompress-main.cc)
target_link_libraries(fstcompress-main PUBLIC fstcompressscript absl::flags
                                              absl::log
)

add_executable(fstcompress fstcompress.cc)
target_link_libraries(fstcompress PRIVATE fstcompress-main)

add_executable(fstrandmod fstrandmod.cc)
target_link_libraries(
  fstrandmod
  PRIVATE fst
          absl::flags
          absl::flags_parse
          absl::flags_usage
          absl::log
)

add_library(fstquantize_weights quantize_weights.cc)
set_target_properties(
  fstquantize_weights
  PROPERTIES SOVERSION ${OPENFST_SOVERSION}
)

target_include_directories(
  fstquantize_weights PUBLIC $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
                             $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_link_libraries(fstquantize_weights PUBLIC fst absl::log absl::span)

add_executable(fstquantize fstquantize.cc)
target_link_libraries(
  fstquantize
  PRIVATE fstquantize_weights
          fst
          absl::flags
          absl::log
)

openfst_install(
  TARGETS fstcompress_elias
          fstcompress_compress
          fstcompressscript
          fstquantize_weights
  EXPORT OpenFstTargets
  DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
openfst_install(TARGETS fstcompress fstrandmod fstquantize
                RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

set(COMPRESS_HEADERS
    compress.h
    compressscript.h
    elias.h
    quantize_weights.h
    randmod.h
)
openfst_install(
  FILES ${COMPRESS_HEADERS}
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/openfst/extensions/compress
)

if(OPENFST_BUILD_TESTS)
  include(GoogleTest)
  file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/testdata"
       DESTINATION "${CMAKE_CURRENT_BINARY_DIR}"
  )

  add_executable(compress_test compress_test.cc)
  set_target_properties(
    compress_test
    PROPERTIES
      CROSSCOMPILING_EMULATOR
        "${CMAKE_SOURCE_DIR}/scripts/run_with_test_tmpdir.sh"
  )
  target_link_libraries(
    compress_test
    PRIVATE fstcompress_compress
            test_main
            absl::flags
            absl::log
            fst
  )
  gtest_discover_tests(
    compress_test
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
  )

  add_executable(randmod_test randmod_test.cc)
  set_target_properties(
    randmod_test
    PROPERTIES
      CROSSCOMPILING_EMULATOR
        "${CMAKE_SOURCE_DIR}/scripts/run_with_test_tmpdir.sh"
  )
  target_link_libraries(
    randmod_test
    PRIVATE fstcompress_compress
            test_main
            absl::flags
            absl::log
            fst
  )
  gtest_discover_tests(
    randmod_test
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
  )

  add_executable(quantize_weights_test quantize_weights_test.cc)
  set_target_properties(
    quantize_weights_test
    PROPERTIES
      CROSSCOMPILING_EMULATOR
        "${CMAKE_SOURCE_DIR}/scripts/run_with_test_tmpdir.sh"
  )
  target_link_libraries(
    quantize_weights_test
    PRIVATE fstquantize_weights
            test_main
            absl::flags
            absl::log
            fst
  )
  gtest_discover_tests(
    quantize_weights_test
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
  )
endif()
