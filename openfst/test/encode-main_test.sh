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

TEST_SRCDIR="$TEST_SRCDIR/$TEST_WORKSPACE"

set -eou pipefail

TST="$TEST_TMPDIR"
FST="$TEST_SRCDIR/openfst"

BIN="$FST/bin"
DAT="$FST/test/testdata/encode"

source "$FST"/bin/setup.sh

# Encode labels.
"$BIN"/fstencode -encode_labels $DAT/e1.fst "$TST"/codex  "$TST"/e1_c.fst
"$BIN"/fstencode -decode -encode_labels "$TST"/e1_c.fst "$TST"/codex "$TST"/e1_cd.fst
rm "$TST"/codex
"$BIN"/fstequal -v=1 $DAT/e1_cd.fst "$TST"/e1_cd.fst

# Encode weights.
"$BIN"/fstencode -encode_weights $DAT/e1.fst "$TST"/codex  "$TST"/e1_c.fst
"$BIN"/fstencode -decode -encode_weights "$TST"/e1_c.fst "$TST"/codex "$TST"/e1_cd.fst
rm "$TST"/codex
"$BIN"/fstequal -v=1 $DAT/e1_cd.fst "$TST"/e1_cd.fst

# Encode labels and weights.
"$BIN"/fstencode -encode_labels -encode_weights $DAT/e1.fst "$TST"/codex \
    "$TST"/e1_c.fst
"$BIN"/fstencode -decode -encode_labels -encode_weights "$TST"/e1_c.fst "$TST"/codex \
    "$TST"/e1_cd.fst
"$BIN"/fstequal -v=1 $DAT/e1_cd.fst "$TST"/e1_cd.fst

# We keep codex for next test.

# Encode labels and weights using pre-existing codex.
"$BIN"/fstencode -encode_labels -encode_weights -encode_reuse $DAT/e2.fst \
    "$TST"/codex "$TST"/e2_c.fst
"$BIN"/fstequal -v=1 $DAT/e2_c.fst "$TST"/e2_c.fst

# Encode labels using out.
"$BIN"/fstencode -encode_labels $DAT/e1.fst "$TST"/codex > "$TST"/e1_c.fst
"$BIN"/fstencode -decode -encode_labels "$TST"/e1_c.fst "$TST"/codex "$TST"/e1_cd.fst
"$BIN"/fstequal -v=1 $DAT/e1_cd.fst "$TST"/e1_cd.fst

echo "PASS"
