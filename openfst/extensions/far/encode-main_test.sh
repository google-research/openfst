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
# Unit test for farencode.

TEST_SRCDIR="$TEST_SRCDIR/$TEST_WORKSPACE"

set -eou pipefail

readonly FST_DIR="${TEST_SRCDIR}/openfst"
source "${FST_DIR}/bin/setup.sh"

readonly FAR="${FST_DIR}/extensions/far"
readonly TESTDATA="${FAR}/testdata"

"${FAR}/farencode" \
  --encode_labels \
  --encode_weights \
  "${TESTDATA}/test2.stlist.far" \
  "${TEST_TMPDIR}/codex" \
  "${TEST_TMPDIR}/test2_c.far"

"${FAR}/farencode" \
  --decode \
  "${TEST_TMPDIR}/test2_c.far" \
  "${TEST_TMPDIR}/codex" \
  "${TEST_TMPDIR}/test2_cd.far"

rm "${TEST_TMPDIR}/codex"

"${FAR}/farequal" \
  -v=1 \
  "${TESTDATA}/test2.stlist.far" \
  "${TEST_TMPDIR}/test2_cd.far"
