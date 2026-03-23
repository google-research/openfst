#!/bin/bash

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

set -euxo pipefail

echo "--- Running CMake tests ---"

NPROC="$(getconf _NPROCESSORS_ONLN)"

# Using -DOPENFST_ENABLE_FSTS=ON and -DOPENFST_ENABLE_GRM=ON ensures we test
# all extensions, similar to the CI workflow.
cmake -S . -B build \
  -DOPENFST_ENABLE_BIN=ON \
  -DOPENFST_BUILD_TESTS=ON \
  -DOPENFST_ENABLE_FSTS=ON \
  -DOPENFST_ENABLE_GRM=ON \
  -DOPENFST_ENABLE_PYTHON=ON \
  -DBUILD_SHARED_LIBS=ON

cmake --build build -j "$NPROC"
ctest --test-dir build --output-on-failure -j "$NPROC"
