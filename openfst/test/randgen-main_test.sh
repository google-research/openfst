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
# Unit test for fstrangen.

source googletest.sh || exit

TEST_SRCDIR="$TEST_SRCDIR/$TEST_WORKSPACE"

set -eou pipefail

TST="$TEST_TMPDIR"
FST="$TEST_SRCDIR/openfst"

BIN="$FST/bin"
DAT="$FST/test/testdata/randgen"

source "$BIN"/setup.sh

echo "SKIPPING due to b/476053592: Fails due to std::uniform_int_distribution."
exit 0

"$BIN"/fstrandgen --seed 2 "$DAT"/r1.fst "$TST"/r2.fst
"$BIN"/fstequal -v 1 "$DAT"/r2.fst "$TST"/r2.fst

"$BIN"/fstrandgen --seed 2 "$DAT"/r1.fst > "$TST"/r2.fst
"$BIN"/fstequal -v 1 "$DAT"/r2.fst "$TST"/r2.fst

"$BIN"/fstrandgen --seed 2 < "$DAT"/r1.fst > "$TST"/r2.fst
"$BIN"/fstequal -v 1 "$DAT"/r2.fst "$TST"/r2.fst
