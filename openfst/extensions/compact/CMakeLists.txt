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

set(COMPACT_LIBS
    compact8_acceptor-fst
    compact8_string-fst
    compact8_unweighted-fst
    compact8_unweighted_acceptor-fst
    compact8_weighted_string-fst
    compact16_acceptor-fst
    compact16_string-fst
    compact16_unweighted-fst
    compact16_unweighted_acceptor-fst
    compact16_weighted_string-fst
    compact64_acceptor-fst
    compact64_string-fst
    compact64_unweighted-fst
    compact64_unweighted_acceptor-fst
    compact64_weighted_string-fst
)

foreach(LIB ${COMPACT_LIBS})
  fst_add_library(${LIB} ${LIB}.cc)
  set_target_properties(
    ${LIB}
    PROPERTIES SOVERSION ${OPENFST_SOVERSION}
  )
  target_include_directories(
    ${LIB} PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
                  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  )
  target_link_libraries(${LIB} PUBLIC fst)
  openfst_install(
    TARGETS ${LIB}
    EXPORT OpenFstTargets
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/fst
  )
endforeach()

# Logical interface wrapper for plugin-backed Compact extension
add_library(
  openfst::compact
  INTERFACE
  IMPORTED
  GLOBAL
)
set_target_properties(
  openfst::compact
  PROPERTIES INTERFACE_LINK_LIBRARIES openfst::fst
)
