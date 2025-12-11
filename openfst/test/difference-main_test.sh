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
# Unit test for fstdifference.

TEST_SRCDIR=.

set -eou pipefail

TST="$TEST_TMPDIR"
FST="$TEST_SRCDIR/openfst"

BIN="$FST/bin"
DAT="$FST/test/testdata/difference"

source "$BIN"/setup.sh

"$BIN"/fstdifference --connect=false "$DAT"/d1.fst "$DAT"/d2.fst "$TST"/d3.fst
"$BIN"/fstequal -v=1 "$DAT"/d3.fst "$TST"/d3.fst

"$BIN"/fstdifference --connect=true "$DAT"/d4.fst "$DAT"/d2.fst "$TST"/d5.fst
"$BIN"/fstequal -v=1 "$DAT"/d5.fst "$TST"/d5.fst

/bin/rm "$TST"/d5.fst

# output to stdout
"$BIN"/fstdifference --connect=true "$DAT"/d4.fst "$DAT"/d2.fst > "$TST"/d5.fst
"$BIN"/fstequal -v=1 "$DAT"/d5.fst "$TST"/d5.fst
