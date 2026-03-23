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
#
# This script is used as a wrapper for test execution (via CMake's
# `CROSSCOMPILING_EMULATOR`) to ensure that each test process has a unique
# `TEST_TMPDIR`.
#
# Many OpenFst tests use hardcoded filenames in the temporary directory.  When
# running tests in parallel (e.g., with `ctest -jN` and `gtest_discover_tests`),
# these tests can collide and fail. This wrapper isolates each test process by
# giving it its own scratch space.

set -euo pipefail

# Create a unique temporary directory for this process.
export TEST_TMPDIR="$(mktemp -d)"

# Ensure the temporary directory is cleaned up on exit (success or failure).
trap 'rm -rf "${TEST_TMPDIR}"' EXIT

# Execute the test with all provided arguments.
"$@"
