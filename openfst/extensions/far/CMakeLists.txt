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

add_library(
  fstfar
  stlist.cc
  sttable.cc
  far.cc
  getters.cc
)
set_target_properties(
  fstfar
  PROPERTIES SOVERSION ${OPENFST_SOVERSION}
)

target_include_directories(
  fstfar PUBLIC $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
                $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_link_libraries(
  fstfar
  PUBLIC fst
         absl::base
         absl::core_headers
         absl::log
         absl::memory
         absl::status
         absl::statusor
         absl::strings
         absl::span
)

add_library(
  fstfarscript
  compile-strings.cc
  far-class.cc
  farscript.cc
  script-impl.cc
)
set_target_properties(
  fstfarscript
  PROPERTIES SOVERSION ${OPENFST_SOVERSION}
)

target_include_directories(
  fstfarscript PUBLIC $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
                      $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_link_libraries(
  fstfarscript
  PUBLIC fstfar
         fstscript
         absl::base
         absl::log
         absl::status
         absl::statusor
         absl::strings
)

openfst_install(
  TARGETS fstfar fstfarscript
  EXPORT OpenFstTargets
  DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

set(FAR_HEADERS
    compile-strings.h
    convert.h
    create.h
    encode.h
    equal.h
    extract.h
    far-class.h
    far-reader.h
    far-type.h
    far-writer.h
    far.h
    farlib.h
    farscript.h
    fst-far-reader.h
    fst-far-writer.h
    fst-reader.h
    fst-writer.h
    getters.h
    info.h
    isomorphic.h
    map-reduce.h
    print-strings.h
    script-impl.h
    stlist-far-reader.h
    stlist-far-writer.h
    stlist.h
    sttable-far-reader.h
    sttable-far-writer.h
    sttable.h
)

openfst_install(FILES ${FAR_HEADERS}
                DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/openfst/extensions/far
)

set(FAR_BINS
    farcreate
    farcompilestrings
    farconvert
    farencode
    farequal
    farextract
    farinfo
    farisomorphic
    farprintstrings
)

foreach(BIN ${FAR_BINS})

  add_library(${BIN}-main ${BIN}-main.cc)

  target_link_libraries(
    ${BIN}-main
    PUBLIC fstfarscript
           absl::flags
           absl::log
           absl::status
           absl::statusor
  )

  add_executable(${BIN} ${BIN}.cc)

  target_link_libraries(${BIN} PRIVATE ${BIN}-main absl::status absl::statusor)

  openfst_install(TARGETS ${BIN} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

endforeach()
