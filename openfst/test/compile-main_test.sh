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
# Unit test for fstcompile.

TEST_SRCDIR="$TEST_SRCDIR/$TEST_WORKSPACE"

set -eou pipefail

TST="$TEST_TMPDIR"
FST="$TEST_SRCDIR/openfst"

BIN="$FST/bin"
DAT="$FST/test/testdata/compile"

source "$BIN"/setup.sh

"$BIN"/fstcompile --isymbols="$DAT"/isyms.map --osymbols "$DAT"/osyms.map \
  --keep_isymbols --keep_osymbols "$DAT"/fst.txt "$TST"/fst.compiled

"$BIN"/fstequal -v=1 "$DAT"/fst.compiled "$TST"/fst.compiled

/bin/rm "$TST"/fst.compiled

# stdin -> stdout
"$BIN"/fstcompile --isymbols="$DAT"/isyms.map --osymbols "$DAT"/osyms.map \
  --keep_isymbols --keep_osymbols < "$DAT"/fst.txt > "$TST"/fst.compiled
"$BIN"/fstequal -v=1 "$DAT"/fst.compiled "$TST"/fst.compiled

/bin/rm "$TST"/fst.compiled

# file -> stdout
"$BIN"/fstcompile --isymbols="$DAT"/isyms.map --osymbols "$DAT"/osyms.map \
  --keep_isymbols --keep_osymbols "$DAT"/fst.txt > "$TST"/fst.compiled
"$BIN"/fstequal -v=1 "$DAT"/fst.compiled "$TST"/fst.compiled
