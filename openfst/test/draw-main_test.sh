# Copyright 2025 The OpenFst Authors.
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
# Unit test for fstdraw.

TEST_SRCDIR=.

set -eou pipefail

TST="$TEST_TMPDIR"
FST="$TEST_SRCDIR/openfst"

BIN="$FST/bin"
DAT="$FST/test/testdata/draw"

source "$BIN"/setup.sh

# From file.
"$BIN"/fstdraw --precision=2 "$DAT"/draw.fst "$TST"/draw-2p.dot
cmp "$DAT"/draw-2p.dot "$TST"/draw-2p.dot
"$BIN"/fstdraw --precision=5 "$DAT"/draw.fst "$TST"/draw-5p.dot
cmp "$DAT"/draw-5p.dot "$TST"/draw-5p.dot
/bin/rm -f "$TST"/draw-[25]p.dot

# To stdout.
"$BIN"/fstdraw --precision=2 "$DAT"/draw.fst > "$TST"/draw-2p.dot
cmp "$DAT"/draw-2p.dot "$TST"/draw-2p.dot
"$BIN"/fstdraw --precision=5 "$DAT"/draw.fst > "$TST"/draw-5p.dot
cmp "$DAT"/draw-5p.dot "$TST"/draw-5p.dot
/bin/rm -f "$TST"/draw-[25]p.dot

# From stdin to stdout.
"$BIN"/fstdraw --precision=2 < "$DAT"/draw.fst > "$TST"/draw-2p.dot
cmp "$DAT"/draw-2p.dot "$TST"/draw-2p.dot
"$BIN"/fstdraw --precision=5 < "$DAT"/draw.fst > "$TST"/draw-5p.dot
cmp "$DAT"/draw-5p.dot "$TST"/draw-5p.dot
