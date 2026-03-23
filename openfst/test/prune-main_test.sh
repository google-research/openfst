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
# Unit test for fstprune.

TEST_SRCDIR="$TEST_SRCDIR/$TEST_WORKSPACE"

set -eou pipefail

TST="$TEST_TMPDIR"
FST="$TEST_SRCDIR/openfst"

BIN="$FST/bin"
DAT="$FST/test/testdata/prune"

source "$BIN"/setup.sh

"$BIN"/fstprune -weight=0.5  "$DAT"/p1.fst "$TST"/p2.fst
"$BIN"/fstequal -v=1 "$DAT"/p2.fst "$TST"/p2.fst

"$BIN"/fstprune -nstate 2 "$DAT"/p3.fst > "$TST"/p4.fst
"$BIN"/fstequal -v=1 "$DAT"/p4.fst "$TST"/p4.fst

"$BIN"/fstprune -nstate 2 < "$DAT"/p3.fst > "$TST"/p4.fst
"$BIN"/fstequal -v=1 "$DAT"/p4.fst "$TST"/p4.fst
