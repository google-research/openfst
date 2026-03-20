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

# Testing with rho transitions.

# rho on a match.
"${SPBIN}/fstspecial" --fst_type="rho" --rho_fst_rho_label=2 \
  "${DAT}/e.fst" |
"${FBIN}/fstcompose" "${DAT}/d1.fst" - > "${TST}/f1.fst"
"${FBIN}/fstequal" "${DAT}/f1.fst" "${TST}/f1.fst"

# rho on a non-match w/ overwriting.
"${SPBIN}/fstspecial" --fst_type="rho" --rho_fst_rho_label=2 \
  --rho_fst_rewrite_mode="never" "${DAT}/e.fst" |
"${FBIN}/fstcompose" "${DAT}/d2.fst" - > "${TST}/f2.fst"
"${FBIN}/fstequal" "${DAT}/f2.fst" "${TST}/f2.fst"

# rho on a non-match w/o overwriting.
"${SPBIN}/fstspecial" --fst_type="rho" --rho_fst_rho_label=2 \
  --rho_fst_rewrite_mode="always" "${DAT}/e.fst" |
"${FBIN}/fstcompose" "${DAT}/d2.fst" - > "${TST}/d2.fst"
"${FBIN}/fstequal" "${DAT}/d2.fst" "${TST}/d2.fst"

# input_rho on a match.
"${SPBIN}/fstspecial" --fst_type="input_rho" --rho_fst_rho_label=2 \
  "${DAT}/e.fst" |
"${FBIN}/fstcompose" "${DAT}/d1.fst" - > "${TST}/f1.fst"
"${FBIN}/fstequal" "${DAT}/f1.fst" "${TST}/f1.fst"

# input_rho on a non-match w/ overwriting.
"${SPBIN}/fstspecial" --fst_type="input_rho" --rho_fst_rho_label=2 \
  --rho_fst_rewrite_mode="never" "${DAT}/e.fst" |
"${FBIN}/fstcompose" "${DAT}/d2.fst" - > "${TST}/f2.fst"
"${FBIN}/fstequal" "${DAT}/f2.fst" "${TST}/f2.fst"

# input_rho on a non-match w/o overwriting.
"${SPBIN}/fstspecial" --fst_type="input_rho" --rho_fst_rho_label=2 \
  --rho_fst_rewrite_mode="always" "${DAT}/e.fst" |
"${FBIN}/fstcompose" "${DAT}/d2.fst" - > "${TST}/d2.fst"
"${FBIN}/fstequal" "${DAT}/d2.fst" "${TST}/d2.fst"
