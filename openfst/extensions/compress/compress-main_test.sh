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
# Unit test for fstcompress.

source googletest.sh || exit

TEST_SRCDIR="$TEST_SRCDIR/$TEST_WORKSPACE"

set -eou pipefail

TST="$TEST_TMPDIR"
FST="$TEST_SRCDIR/openfst"

FBIN="$FST/bin"
XBIN="$FST/extensions/compress"
DAT="$XBIN/testdata"

source "$FBIN/setup.sh"

# To and from file handles.
"$XBIN/fstcompress" "$DAT/unweight.fst" "$TST/compress.fst"
"$XBIN/fstcompress" -decode "$TST/compress.fst" "$TST/unweight.fst"
"$FBIN/fstisomorphic" -v=1 "$DAT/unweight.fst" "$TST/unweight.fst"

# To stdout.
"$XBIN/fstcompress" "$DAT/unweight.fst" > "$TST/compress.fst"
"$XBIN/fstcompress" -decode "$TST/compress.fst" > "$TST/unweight.fst"
"$FBIN/fstisomorphic" -v=1 "$DAT/unweight.fst" "$TST/unweight.fst"

# From stdin.
"$XBIN/fstcompress" < "$DAT/unweight.fst" > "$TST/compress.fst"
"$XBIN/fstcompress" -decode  <"$TST/compress.fst" > "$TST/unweight.fst"
"$FBIN/fstisomorphic" -v=1 "$DAT/unweight.fst" "$TST/unweight.fst"
