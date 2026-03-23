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
# Unit test for fstarcsort and one input FST argument file handling.

TEST_SRCDIR="$TEST_SRCDIR/$TEST_WORKSPACE"

set -eou pipefail

TST="$TEST_TMPDIR"
FST="$TEST_SRCDIR/openfst"

BIN="$FST/bin"
DAT="$FST/test/testdata/arcsort"

source "$BIN"/setup.sh

# input label sort test

# file file
"$BIN"/fstarcsort -sort_type=ilabel "$DAT"/a2.fst "$TST"/a1.fst
"$BIN"/fstequal -v=1 "$DAT"/a1.fst "$TST"/a1.fst

rm -f "$TST"/a1.fst

# stdin("-") file
"$BIN"/fstarcsort -sort_type=ilabel - "$TST"/a1.fst <"$DAT"/a2.fst
"$BIN"/fstequal -v=1 "$DAT"/a1.fst "$TST"/a1.fst

rm -f "$TST"/a1.fst

# file stdout
"$BIN"/fstarcsort -sort_type=ilabel "$DAT"/a2.fst >"$TST"/a1.fst
"$BIN"/fstequal -v=1 "$DAT"/a1.fst "$TST"/a1.fst

rm -f "$TST"/a1.fst

# # stdin("") stdout
"$BIN"/fstarcsort -sort_type=ilabel >"$TST"/a1.fst <"$DAT"/a2.fst
"$BIN"/fstequal -v=1 "$DAT"/a1.fst "$TST"/a1.fst

rm -f "$TST"/a1.fst

# stdin("-") stdout
"$BIN"/fstarcsort -sort_type=ilabel - >"$TST"/a1.fst <"$DAT"/a2.fst
"$BIN"/fstequal -v=1 "$DAT"/a1.fst "$TST"/a1.fst

rm -f "$TST"/a2.fst

# output label sort test
"$BIN"/fstarcsort -sort_type=olabel "$DAT"/a1.fst "$TST"/a2.fst
"$BIN"/fstequal -v=1 "$DAT"/a2.fst "$TST"/a2.fst
