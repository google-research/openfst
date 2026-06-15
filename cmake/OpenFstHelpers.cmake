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

# Canonical OpenFst CMake Helper Macros
# Centralizes library declaration and universal namespaced target aliasing.

function(fst_add_library TARGET_NAME)
  # 1. Flawlessly delegate to built-in add_library supporting exact signature
  add_library(${ARGV})

  # 2. Derive modern lowercase imported target name.
  # Internally, OpenFst prefixes extension modules with 'fst' (e.g., fstfar, fstngram),
  # but external consumers expect prefix-stripped target names (e.g., openfst::far).
  # Below we strip the redundant leading 'fst' while preserving the core 'fst' library.
  set(EXPORT_NAME "")
  if(TARGET_NAME STREQUAL "fst")
    set(EXPORT_NAME "fst")
  elseif(TARGET_NAME STREQUAL "fstscript")
    set(EXPORT_NAME "fstscript")
  elseif(TARGET_NAME MATCHES "^fst(.+)$")
    set(EXPORT_NAME "${CMAKE_MATCH_1}")
  endif()

  # 3. Automatically inject fail-safe ALIAS bridge
  if(NOT EXPORT_NAME STREQUAL "")
    add_library(openfst::${EXPORT_NAME} ALIAS ${TARGET_NAME})
  endif()
endfunction()
