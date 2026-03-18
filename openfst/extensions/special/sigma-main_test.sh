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

# This allows fo dynamic loading in `fstcompose` and `fstequal`.
if [[ "$(uname)" == "Darwin" ]]; then
  export DYLD_LIBRARY_PATH="${DYLD_LIBRARY_PATH:-}:${SBIN}"
else
  export LD_LIBRARY_PATH="${LD_LIBRARY_PATH:-}:${SBIN}"
fi

# sigma w/o overwriting.
"${SPBIN}/fstspecial" --fst_type="sigma" --sigma_fst_sigma_label=3 \
  --sigma_fst_rewrite_mode="never" "${DAT}/h.fst" |
"${FBIN}/fstcompose" "${DAT}/g.fst" - > "${TST}/i1.fst"
"${FBIN}/fstequal" "${DAT}/i1.fst" "${TST}/i1.fst"

# sigma w/ overwriting.
"${SPBIN}/fstspecial" --fst_type="sigma" --sigma_fst_sigma_label=3 \
  --sigma_fst_rewrite_mode="always" "${DAT}/h.fst" |
"${FBIN}/fstcompose" "${DAT}/g.fst" - > "${TST}/i2.fst"
"${FBIN}/fstequal" "${DAT}/i2.fst" "${TST}/i2.fst"

# input_sigma w/o overwriting.
"${SPBIN}/fstspecial" --fst_type="input_sigma" --sigma_fst_sigma_label=3 \
  --sigma_fst_rewrite_mode="never" "${DAT}/h.fst" |
"${FBIN}/fstcompose" "${DAT}/g.fst" - > "${TST}/i1.fst"
"${FBIN}/fstequal" "${DAT}/i1.fst" "${TST}/i1.fst"

# input_sigma w/ overwriting.
"${SPBIN}/fstspecial" --fst_type="input_sigma" --sigma_fst_sigma_label=3 \
  --sigma_fst_rewrite_mode="always" "${DAT}/h.fst" |
"${FBIN}/fstcompose" "${DAT}/g.fst" - > "${TST}/i2.fst"
"${FBIN}/fstequal" "${DAT}/i2.fst" "${TST}/i2.fst"
