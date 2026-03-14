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

set(BINARIES
    fstarcsort
    fstclosure
    fstcompile
    fstcompose
    fstconcat
    fstconnect
    fstconvert
    fstdeterminize
    fstdifference
    fstdisambiguate
    fstdraw
    fstencode
    fstepsnormalize
    fstequal
    fstequivalent
    fstinfo
    fstintersect
    fstinvert
    fstisomorphic
    fstmap
    fstminimize
    fstprint
    fstproject
    fstprune
    fstpush
    fstrandgen
    fstrelabel
    fstreplace
    fstreverse
    fstreweight
    fstrmepsilon
    fstshortestdistance
    fstshortestpath
    fstsymbols
    fstsynchronize
    fsttopsort
    fstunion
)

foreach(TOOL_NAME ${BINARIES})
  # Create the main library (e.g., fstarcsort-main)
  add_library(${TOOL_NAME}-main ${TOOL_NAME}-main.cc)

  target_link_libraries(
    ${TOOL_NAME}-main
    PUBLIC fstscript
           fst
           absl::flags
           absl::flags_parse
           absl::flags_usage
           absl::log
  )

  # Create the executable (e.g., fstarcsort)
  add_executable(${TOOL_NAME} ${TOOL_NAME}.cc)
  target_link_libraries(${TOOL_NAME} PRIVATE ${TOOL_NAME}-main absl::flags)

  openfst_install(TARGETS ${TOOL_NAME} RUNTIME DESTINATION
                                               ${CMAKE_INSTALL_BINDIR}
  )

endforeach()
