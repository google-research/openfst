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
# Unit test for pdtreplace.

TEST_SRCDIR="$TEST_SRCDIR/$TEST_WORKSPACE"

set -eou pipefail

TST="$TEST_TMPDIR"
FST="$TEST_SRCDIR/openfst"

FBIN="$FST/bin"
XBIN="$FST/extensions/pdt"
DAT="$FST/extensions/pdt/testdata"

source "$FBIN"/setup.sh

"$XBIN"/pdtreplace -pdt_parentheses="$TST"/rparen.pairs "$DAT"/r1.fst 3 "$DAT"/r2.fst 4 \
  "$DAT"/r3.fst 5 "$DAT"/r4.fst 6 "$TST"/r5.fst
"$FBIN"/fstequal -v=1 "$DAT"/r5.fst "$TST"/r5.fst
cmp "$DAT"/rparen.pairs "$TST"/rparen.pairs
