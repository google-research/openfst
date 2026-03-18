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
# Unit test for fstminimize.

TEST_SRCDIR="$TEST_SRCDIR/$TEST_WORKSPACE"

set -eou pipefail

TST="$TEST_TMPDIR"
FST="$TEST_SRCDIR/openfst"

BIN="$FST/bin"
DAT="$FST/test/testdata/minimize"

source "$BIN"/setup.sh

# acyclic test
"$BIN"/fstminimize "$DAT"/m1.fst "$TST"/acyclic_min.fst
"$BIN"/fstequal -v=1 "$DAT"/acyclic_min.fst "$TST"/acyclic_min.fst

# cyclic test
"$BIN"/fstminimize "$DAT"/m2.fst "$TST"/cyclic_min.fst
"$BIN"/fstequal -v=1 "$DAT"/cyclic_min.fst "$TST"/cyclic_min.fst

# same test, output to stdout
"$BIN"/fstminimize "$DAT"/m2.fst > "$TST"/cyclic_min.fst
"$BIN"/fstequal -v=1 "$DAT"/cyclic_min.fst "$TST"/cyclic_min.fst

# test pipeline-type usage
"$BIN"/fstminimize < "$DAT"/m2.fst > "$TST"/cyclic_min.fst
"$BIN"/fstequal -v=1 "$DAT"/cyclic_min.fst "$TST"/cyclic_min.fst

# weighted acyclic test
"$BIN"/fstminimize "$DAT"/m3.fst "$TST"/weighted_acyclic_min.fst
"$BIN"/fstequal -v=1 "$DAT"/weighted_acyclic_min.fst "$TST"/weighted_acyclic_min.fst

# weighted cyclic test
"$BIN"/fstminimize "$DAT"/m4.fst "$TST"/weighted_cyclic_min.fst
"$BIN"/fstequal -v=1 "$DAT"/weighted_cyclic_min.fst "$TST"/weighted_cyclic_min.fst

# transducer acyclic test (one output)
"$BIN"/fstminimize "$DAT"/m5.fst "$TST"/transducer_acyclic_min.fst
"$BIN"/fstequal -v=1 "$DAT"/transducer_acyclic_min.fst \
  "$TST"/transducer_acyclic_min.fst

# transducer acyclic test (two outputs)
"$BIN"/fstminimize "$DAT"/m5.fst "$TST"/transducer_acyclic_min1.fst \
  "$TST"/transducer_acyclic_min2.fst
"$BIN"/fstequal -v=1 "$DAT"/transducer_acyclic_min1.fst \
  "$TST"/transducer_acyclic_min1.fst
"$BIN"/fstequal -v=1 "$DAT"/transducer_acyclic_min2.fst \
  "$TST"/transducer_acyclic_min2.fst

# same test, but output one of the outputs to standard out
"$BIN"/fstminimize "$DAT"/m5.fst "$TST"/ta_min1.fst "-" > "$TST"/ta_min2.fst
"$BIN"/fstequal -v=1 "$DAT"/transducer_acyclic_min1.fst "$TST"/ta_min1.fst
"$BIN"/fstequal -v=1 "$DAT"/transducer_acyclic_min2.fst "$TST"/ta_min2.fst

# transducer cyclic test
"$BIN"/fstminimize "$DAT"/m6.fst "$TST"/transducer_cyclic_min.fst
"$BIN"/fstequal -v=1 "$DAT"/transducer_cyclic_min.fst \
  "$TST"/transducer_cyclic_min.fst
