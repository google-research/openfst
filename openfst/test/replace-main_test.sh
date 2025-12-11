# Copyright 2025 The OpenFst Authors.
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
# Unit test for fstreplace.

TEST_SRCDIR=.

set -eou pipefail

TST="$TEST_TMPDIR"
FST="$TEST_SRCDIR/openfst"

BIN="$FST/bin"
DAT="$FST/test/testdata/replace"

source "$BIN"/setup.sh

"$BIN"/fstreplace --call_arc_labeling="neither" "$DAT"/g1.fst 1 "$DAT"/g2.fst 2 \
  "$DAT"/g3.fst 3 "$DAT"/g4.fst 4 "$TST"/g_out.fst
"$BIN"/fstequal -v=1 "$DAT"/g_out.fst "$TST"/g_out.fst

"$BIN"/fstreplace --call_arc_labeling="neither" "$DAT"/g1.fst 1 "$DAT"/g2.fst 2 \
  "$DAT"/g3.fst 3 "$DAT"/g4.fst 4 > "$TST"/g_out.fst
"$BIN"/fstequal -v=1 "$DAT"/g_out.fst "$TST"/g_out.fst
