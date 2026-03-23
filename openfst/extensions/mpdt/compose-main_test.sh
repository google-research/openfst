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
# Unit test for mpdtcompose.

TEST_SRCDIR="$TEST_SRCDIR/$TEST_WORKSPACE"

set -eou pipefail

TST="$TEST_TMPDIR"
FST="$TEST_SRCDIR/openfst"

FBIN="$FST/bin"
XBIN="$FST/extensions/mpdt"
DAT="$FST/extensions/mpdt/testdata"

source "$FBIN"/setup.sh

"$XBIN"/mpdtcompose -mpdt_parentheses="$DAT"/eparen.triples "$DAT"/c1.fst "$DAT"/c2.fst "$TST"/c3.fst
"$FBIN"/fstequal -v=1 "$DAT"/c3.fst "$TST"/c3.fst

"$XBIN"/mpdtcompose --left_mpdt=false -mpdt_parentheses="$DAT"/eparen.triples "$DAT"/c2.fst "$DAT"/c1.fst "$TST"/c3.fst
"$FBIN"/fstequal -v=1 "$DAT"/c3.fst "$TST"/c3.fst
