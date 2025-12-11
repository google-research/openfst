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
# Unit test for fstintersect.

TEST_SRCDIR=.

set -eou pipefail

TST="$TEST_TMPDIR"
FST="$TEST_SRCDIR/openfst"

BIN="$FST/bin"
DAT="$FST/test/testdata/intersect"

source "$BIN"/setup.sh

"$BIN"/fstintersect "$DAT"/i1.fst "$DAT"/i2.fst "$TST"/i3.fst
"$BIN"/fstequal -v=1 "$DAT"/i3.fst "$TST"/i3.fst

"$BIN"/fstintersect --connect=false "$DAT"/i1.fst "$DAT"/i4.fst "$TST"/i5.fst
"$BIN"/fstequal -v=1 "$DAT"/i5.fst "$TST"/i5.fst

# stdout
"$BIN"/fstintersect --connect=false "$DAT"/i1.fst "$DAT"/i4.fst > "$TST"/i5.fst
"$BIN"/fstequal -v=1 "$DAT"/i5.fst "$TST"/i5.fst
