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
# Unit test for fstshortestpath.sh

TEST_SRCDIR=.

set -eou pipefail

TST="$TEST_TMPDIR"
FST="$TEST_SRCDIR/openfst"

BIN="$FST/bin"
DAT="$FST/test/testdata/shortest-path"

source "$BIN"/setup.sh

"$BIN"/fstshortestpath "$DAT"/sp1.fst "$TST"/sp2.fst
"$BIN"/fstequal -v=1 "$DAT"/sp2.fst "$TST"/sp2.fst

"$BIN"/fstshortestpath -nshortest=4 "$DAT"/sp1.fst "$TST"/sp3.fst
"$BIN"/fstequal -v=1 "$DAT"/sp3.fst "$TST"/sp3.fst

"$BIN"/fstshortestpath -unique -nshortest=4 "$DAT"/sp1.fst "$TST"/sp4.fst
"$BIN"/fstequal -v=1 "$DAT"/sp4.fst "$TST"/sp4.fst

"$BIN"/fstshortestpath -nshortest=4 -weight=1.0 -nstate=20 "$DAT"/sp5.fst \
  "$TST"/sp9.fst
"$BIN"/fstequal -v=1 "$DAT"/sp9.fst "$TST"/sp9.fst

for q in fifo lifo shortest state top
do
  "$BIN"/fstshortestpath -queue_type=$q "$DAT"/sp5.fst "$TST"/sp6.fst
  "$BIN"/fstequal -v=1 "$DAT"/sp6.fst "$TST"/sp6.fst
 done

# from stdin
"$BIN"/fstshortestpath - "$TST"/sp2.fst < "$DAT"/sp1.fst
"$BIN"/fstequal -v=1 "$DAT"/sp2.fst "$TST"/sp2.fst

# to stdout
"$BIN"/fstshortestpath "$DAT"/sp1.fst > "$TST"/sp2.fst
"$BIN"/fstequal -v=1 "$DAT"/sp2.fst "$TST"/sp2.fst

# pipe
"$BIN"/fstshortestpath < "$DAT"/sp1.fst > "$TST"/sp2.fst
"$BIN"/fstequal -v=1 "$DAT"/sp2.fst "$TST"/sp2.fst
