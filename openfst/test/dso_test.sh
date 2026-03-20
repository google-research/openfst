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
# Unit test for DSO loading of Fst and arc type extensions.

TEST_SRCDIR="$TEST_SRCDIR/$TEST_WORKSPACE"

set -eou pipefail

TST="$TEST_TMPDIR"
FST="$TEST_SRCDIR/openfst"

BIN="$FST/bin"
LIB="$FST/test"
DAT="$FST/test/testdata/dso"

source "$BIN"/setup.sh

# Adds path to shared objects used below.
export DYLD_LIBRARY_PATH="${DYLD_LIBRARY_PATH:+$DYLD_LIBRARY_PATH:}${LIB}"
export LD_LIBRARY_PATH="${LD_LIBRARY_PATH:+$LD_LIBRARY_PATH:}${LIB}"

# Tests Fst type extension defined in basic-fst.so.
"$BIN"/fstcompile -fst_type=basic "$DAT"/basic.txt "$TST"/basic.fst
"$BIN"/fstprint "$TST"/basic.fst "$TST"/basic.txt
cmp "$DAT"/basic.txt "$TST"/basic.txt

# Tests Fst arc type extension defined in pair-arc.so.
"$BIN"/fstcompile -arc_type=pair "$DAT"/pair.txt "$TST"/pair.fst
"$BIN"/fstprint "$TST"/pair.fst "$TST"/pair.txt
cmp "$DAT"/pair.txt "$TST"/pair.txt

# Tests if Fst weight type is properly parsed.
"$BIN"/fstcompile -arc_type=pair "$DAT"/pair.txt |
"$BIN"/fstmap -map_type=times -weight="0,0" - "$TST"/pair.fst
"$BIN"/fstprint "$TST"/pair.fst "$TST"/pair.txt
cmp "$DAT"/pair.txt "$TST"/pair.txt
