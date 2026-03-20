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

FBIN="$FST/bin"
SPBIN="$FST/extensions/special"
SBIN="$FST/extensions/so"
DAT="$FST/extensions/special/testdata"

source "$FBIN"/setup.sh

# This allows for dynamic loading in `fstcompose` and `fstequal`.
export DYLD_LIBRARY_PATH="${DYLD_LIBRARY_PATH:+$DYLD_LIBRARY_PATH:}${SBIN}"
export LD_LIBRARY_PATH="${LD_LIBRARY_PATH:+$LD_LIBRARY_PATH:}${SBIN}"

# phi, match on output.
"${SPBIN}/fstspecial" --fst_type="phi" "${DAT}/b1.fst" |
"${FBIN}/fstcompose" - "${DAT}/a.fst" > "${TST}/c.fst"
"${FBIN}/fstequal" "${DAT}/c1.fst" "${TST}/c.fst"

# input_phi, match on input.
"${SPBIN}/fstspecial" --fst_type="input_phi" "${DAT}/b1.fst" |
"${FBIN}/fstcompose" "${DAT}/a.fst" - > "${TST}/c.fst"
"${FBIN}/fstequal" "${DAT}/c1.fst" "${TST}/c.fst"

# iutput_phi, match on output.
"${SPBIN}/fstspecial" --fst_type="output_phi" "${DAT}/b1.fst" |
"${FBIN}/fstcompose" - "${DAT}/a.fst" > "${TST}/c.fst"
"${FBIN}/fstequal" "${DAT}/c1.fst" "${TST}/c.fst"

# Testing with no phi transitions.

# phi, no phi transitions in the FST.
"${SPBIN}/fstspecial" --fst_type="phi" "${DAT}/b2.fst" |
"${FBIN}/fstcompose" "${DAT}/a.fst" - > "${TST}/c.fst"
"${FBIN}/fstequal" "${DAT}/c3.fst" "${TST}/c.fst"

# input_phi, match on output.
"${SPBIN}/fstspecial" --fst_type="input_phi" "${DAT}/b2.fst" |
"${FBIN}/fstcompose" - "${DAT}/a.fst" > "${TST}/c.fst"
"${FBIN}/fstequal" "${DAT}/c3.fst" "${TST}/c.fst"

# type: output_phi, match on input.
"${SPBIN}/fstspecial" --fst_type="output_phi" "${DAT}/b2.fst" |
"${FBIN}/fstcompose" "${DAT}/a.fst" - > "${TST}/c.fst"
"${FBIN}/fstequal" "${DAT}/c3.fst" "${TST}/c.fst"
