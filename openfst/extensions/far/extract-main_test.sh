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
# Unit test for farextract.

TEST_SRCDIR="$TEST_SRCDIR/$TEST_WORKSPACE"

set -eou pipefail

TST="$TEST_TMPDIR"
FST="$TEST_SRCDIR/openfst"

FSTBIN="$FST/bin"
FARBIN="$FST/extensions/far"
DAT="$FST/extensions/far/testdata"

source "$FSTBIN"/setup.sh

cd "$TST"

"$FARBIN"/farextract "$DAT"/test1.sttable.far
"$FSTBIN"/fstequal "$DAT"/test1-01.fst "$TST"/test1-01.fst
"$FSTBIN"/fstequal "$DAT"/test1-02.fst "$TST"/test1-02.fst
"$FSTBIN"/fstequal "$DAT"/test1-03.fst "$TST"/test1-03.fst

rm "$TST"/test1-0[123].fst

cat "$DAT"/test2.stlist.far | "$FARBIN"/farextract
"$FSTBIN"/fstequal "$DAT"/test2-01.fst "$TST"/test2-01.fst
"$FSTBIN"/fstequal "$DAT"/test2-02.fst "$TST"/test2-02.fst
"$FSTBIN"/fstequal "$DAT"/test2-03.fst "$TST"/test2-03.fst

"$FARBIN"/farextract "$DAT"/empty.sttable.far
