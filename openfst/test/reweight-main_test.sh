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
# Unit test for fstreweight.

TEST_SRCDIR="$TEST_SRCDIR/$TEST_WORKSPACE"

set -eou pipefail

TST="$TEST_TMPDIR"
FST="$TEST_SRCDIR/openfst"

BIN="$FST/bin"
DAT="$FST/test/testdata/reweight"

source "$BIN"/setup.sh

"$BIN"/fstreweight -reweight_type=to_initial "$DAT"/r1.fst "$DAT"/r1.pot "$TST"/r2.fst
"$BIN"/fstequal -v=1 "$DAT"/r2.fst "$TST"/r2.fst

"$BIN"/fstreweight -reweight_type=to_final "$DAT"/r1.fst "$DAT"/r1.pot "$TST"/r3.fst
"$BIN"/fstequal -v=1 "$DAT"/r3.fst "$TST"/r3.fst

# to stdout
"$BIN"/fstreweight -reweight_type=to_final "$DAT"/r1.fst "$DAT"/r1.pot > "$TST"/r3.fst
"$BIN"/fstequal -v=1 "$DAT"/r3.fst "$TST"/r3.fst
