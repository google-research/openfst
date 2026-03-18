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
# Unit test for fstprint.

source googletest.sh || exit

TEST_SRCDIR="$TEST_SRCDIR/$TEST_WORKSPACE"

set -eou pipefail

TST="$TEST_TMPDIR"
FST="$TEST_SRCDIR/openfst"

BIN="$FST/bin"
DAT="$FST/test/testdata/print"

source "$BIN"/setup.sh

## No symbol tables.

"$BIN"/fstprint "$DAT"/p1.fst "$TST"/p1-nosyms.txt
cmp "$DAT"/p1-nosyms.txt "$TST"/p1-nosyms.txt

"$BIN"/fstprint "$DAT"/p1.fst > "$TST"/p1-nosyms.txt
cmp "$DAT"/p1-nosyms.txt "$TST"/p1-nosyms.txt

"$BIN"/fstprint - "$TST"/p1-nosyms.txt < "$DAT"/p1.fst
cmp "$DAT"/p1-nosyms.txt "$TST"/p1-nosyms.txt

## With symbol tables.

declare -a SYMARGS
SYMARGS=( --isymbols=$DAT/p1.map --osymbols=$DAT/p1.map)

"$BIN"/fstprint "${SYMARGS[@]}" "$DAT"/p1.fst "$TST"/p1-syms.txt
cmp "$DAT"/p1-syms.txt "$TST"/p1-syms.txt

"$BIN"/fstprint "${SYMARGS[@]}" "$DAT"/p1.fst > "$TST"/p1-syms.txt
cmp "$DAT"/p1-syms.txt "$TST"/p1-syms.txt

"$BIN"/fstprint "${SYMARGS[@]}" - "$TST"/p1-syms.txt < "$DAT"/p1.fst
cmp "$DAT"/p1-syms.txt "$TST"/p1-syms.txt

## With an attached symbol table.

"$BIN"/fstprint "$DAT"/p2.fst "$TST"/p2.txt
cmp "$DAT"/p2.txt "$TST"/p2.txt

"$BIN"/fstprint "$DAT"/p2.fst > "$TST"/p2.txt
cmp "$DAT"/p2.txt "$TST"/p2.txt

"$BIN"/fstprint - "$TST"/p2.txt < "$DAT"/p2.fst
cmp "$DAT"/p2.txt "$TST"/p2.txt
