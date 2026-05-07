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
  fstscript
  arciterator-class.cc
  arcsort.cc
  closure.cc
  compile.cc
  compose.cc
  concat.cc
  connect.cc
  convert.cc
  decode.cc
  determinize.cc
  difference.cc
  disambiguate.cc
  draw.cc
  encode.cc
  encodemapper-class.cc
  epsnormalize.cc
  equal.cc
  equivalent.cc
  fst-class.cc
  getters.cc
  info-impl.cc
  info.cc
  intersect.cc
  invert.cc
  isomorphic.cc
  map.cc
  minimize.cc
  print.cc
  project.cc
  prune.cc
  push.cc
  randequivalent.cc
  randgen.cc
  relabel.cc
  replace.cc
  reverse.cc
  reweight.cc
  rmepsilon.cc
  shortest-distance.cc
  shortest-path.cc
  stateiterator-class.cc
  synchronize.cc
  text-io.cc
  topsort.cc
  union.cc
  verify.cc
  weight-class.cc
)

set_target_properties(
  fstscript
  PROPERTIES SOVERSION ${OPENFST_SOVERSION}
)

target_include_directories(
  fstscript PUBLIC $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
                   $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(
  fstscript
  PUBLIC fst
         absl::base
         absl::core_headers
         absl::flat_hash_map
         absl::flags
         absl::log
         absl::memory
         absl::strings
         absl::span
         absl::random_random
)

openfst_install(
  TARGETS fstscript
  EXPORT OpenFstTargets
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

set(SCRIPT_HEADERS
    arc-class.h
    arcfilter-impl.h
    arciterator-class.h
    arcsort.h
    arg-packs.h
    closure.h
    compile-impl.h
    compile.h
    compose.h
    concat.h
    connect.h
    convert.h
    decode.h
    determinize.h
    difference.h
    disambiguate.h
    draw-impl.h
    draw.h
    encode.h
    encodemapper-class.h
    epsnormalize.h
    equal.h
    equivalent.h
    fst-class.h
    fstscript.h
    getters.h
    info-impl.h
    info.h
    intersect.h
    invert.h
    isomorphic.h
    map.h
    minimize.h
    print-impl.h
    print.h
    project.h
    prune.h
    push.h
    randequivalent.h
    randgen.h
    relabel.h
    replace.h
    reverse.h
    reweight.h
    rmepsilon.h
    script-impl.h
    shortest-distance.h
    shortest-path.h
    stateiterator-class.h
    synchronize.h
    text-io.h
    topsort.h
    union.h
    verify.h
    weight-class.h
)
openfst_install(FILES ${SCRIPT_HEADERS}
                DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/openfst/script
)
