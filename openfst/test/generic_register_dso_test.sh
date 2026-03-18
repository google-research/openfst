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

#!/bin/bash
# Unit test that tests the generic behavior for registries loading DSOs.

source googletest.sh || exit

TEST_SRCDIR="$TEST_SRCDIR/$TEST_WORKSPACE"

set -eou pipefail

FST="$TEST_SRCDIR/openfst"
LIB="$FST/test"

source "$FST/bin/setup.sh"

# Adds path to shared objects used below.
if [[ "$(uname)" == "Darwin" ]]; then
  DYLD_LIBRARY_PATH="${DYLD_LIBRARY_PATH:-}:$LIB"
  export DYLD_LIBRARY_PATH
else
  LD_LIBRARY_PATH="${LD_LIBRARY_PATH:-}:$LIB"
  export LD_LIBRARY_PATH
fi

"$LIB"/generic_register_dso_test_helper
