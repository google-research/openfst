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
# Unit test for fstrelabel.

TEST_SRCDIR=.

set -eou pipefail

TST="$TEST_TMPDIR"
FST="$TEST_SRCDIR/openfst"

BIN="$FST/bin"
DAT="$FST/test/testdata/relabel"

source "$BIN"/setup.sh

"$BIN"/fstrelabel --relabel_isymbols="$DAT"/in2.map "$DAT"/r1.fst "$TST"/r2.fst
"$BIN"/fstequal -v=1 "$DAT"/r2.fst "$TST"/r2.fst

"$BIN"/fstrelabel --relabel_osymbols="$DAT"/out3.map "$DAT"/r1.fst "$TST"/r3.fst
"$BIN"/fstequal -v=1 "$DAT"/r3.fst "$TST"/r3.fst

"$BIN"/fstrelabel --relabel_ipairs="$DAT"/in5.pairs "$DAT"/r4.fst "$TST"/r5.fst
"$BIN"/fstequal -v=1 "$DAT"/r5.fst "$TST"/r5.fst

"$BIN"/fstrelabel --relabel_opairs="$DAT"/out6.pairs "$DAT"/r4.fst "$TST"/r6.fst
"$BIN"/fstequal -v=1 "$DAT"/r6.fst "$TST"/r6.fst
