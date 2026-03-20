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
  fst
  ../compat/compat.cc
  ../compat/file_path.cc
  ../compat/init.cc
  encode.cc
  file-util.cc
  fst.cc
  fst-types.cc
  generic-register.cc
  mapped-file.cc
  properties.cc
  symbol-table.cc
  symbol-table-ops.cc
  util.cc
  weight.cc
)

set_target_properties(
  fst
  PROPERTIES SOVERSION ${OPENFST_SOVERSION}
)

target_include_directories(
  fst PUBLIC $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
             $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(
  fst
  PUBLIC absl::base
         absl::core_headers
         absl::flat_hash_map
         absl::flat_hash_set
         absl::node_hash_set
         absl::flags
         absl::flags_parse
         absl::flags_usage
         absl::log
         absl::log_initialize
         absl::check
         absl::memory
         absl::strings
         absl::synchronization
         absl::span
         absl::status
)

openfst_install(
  TARGETS fst
  EXPORT OpenFstTargets
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

set(LIB_HEADERS
    accumulator.h
    add-on.h
    arc-arena.h
    arc-map.h
    arc.h
    arcfilter.h
    arcsort.h
    bi-table.h
    cache.h
    cc-visitors.h
    closure.h
    compact-fst.h
    compat-util.h
    complement.h
    compose-filter.h
    compose.h
    concat.h
    connect.h
    const-fst.h
    determinize.h
    dfs-visit.h
    difference.h
    disambiguate.h
    edit-fst.h
    encode.h
    epsnormalize.h
    equal.h
    equivalent.h
    error-weight.h
    expanded-fst.h
    expander-cache.h
    expander-fst.h
    expectation-weight.h
    factor-weight.h
    file-stream-status.h
    file-util.h
    filter-state.h
    float-weight.h
    fst-decl.h
    fst.h
    fstlib.h
    generic-register.h
    heap.h
    icu.h
    impl-to-fst.h
    intersect.h
    interval-set.h
    invert.h
    isomorphic.h
    label-reachable.h
    lexicographic-weight.h
    lookahead-filter.h
    lookahead-matcher.h
    mapped-file.h
    matcher-fst.h
    matcher.h
    memory.h
    minimize.h
    mutable-fst.h
    pair-weight.h
    partition.h
    port.h
    power-weight-mappers.h
    power-weight.h
    product-weight.h
    project.h
    properties.h
    prune.h
    push.h
    queue.h
    randequivalent.h
    randgen.h
    rational.h
    register.h
    relabel.h
    replace-util.h
    replace.h
    reverse.h
    reweight.h
    rmepsilon.h
    rmfinalepsilon.h
    set-weight.h
    shortest-distance.h
    shortest-path.h
    signed-log-weight.h
    sparse-power-weight.h
    sparse-tuple-weight.h
    state-map.h
    state-reachable.h
    state-table.h
    statesort.h
    string-weight.h
    string.h
    symbol-table-ops.h
    symbol-table.h
    synchronize.h
    test-properties.h
    topsort.h
    tuple-weight.h
    union-find.h
    union-weight.h
    union.h
    util.h
    vector-fst.h
    verify.h
    visit.h
    weight.h
    windows_defs.inc
)

openfst_install(FILES ${LIB_HEADERS}
                DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/openfst/lib
)
