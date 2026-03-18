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
# Unit test for farprintstrings.

source googletest.sh || exit

TEST_SRCDIR="$TEST_SRCDIR/$TEST_WORKSPACE"

set -eou pipefail

TST="$TEST_TMPDIR"
FST="$TEST_SRCDIR/openfst"

FBIN="$FST/bin"
FARBIN="$FST/extensions/far"
DAT="$FST/extensions/far/testdata"

source "$FBIN"/setup.sh

cd "$TST"

# Token type: symbol, entry type: one fst per line
"$FARBIN"/farcreate "$DAT"/strings*-*-symbol.fst "$TST"/strings.far
"$FARBIN"/farprintstrings --token_type=symbol --symbols="$DAT"/syms.map \
  --entry_type=line "$TST"/strings.far > "$TST"/strings
cat "$DAT"/strings1 "$DAT"/strings2 | diff -q - "$TST"/strings
# Same, but from Fsts
"$FARBIN"/farprintstrings --token_type=symbol --symbols="$DAT"/syms.map \
  --entry_type=line "$DAT"/strings*-*-symbol.fst > "$TST"/strings
cat "$DAT"/strings1 "$DAT"/strings2 | diff -q - "$TST"/strings
# Same, but from stdin
"$FARBIN"/farcreate --far_type=stlist "$DAT"/strings*-*-symbol.fst "$TST"/strings.far
"$FARBIN"/farprintstrings --token_type=symbol --symbols="$DAT"/syms.map \
  --entry_type=line < "$TST"/strings.far > "$TST"/strings
cat "$DAT"/strings1 "$DAT"/strings2 | diff -q - "$TST"/strings

# Token type: byte, entry type: one fst per line
"$FARBIN"/farcreate "$DAT"/strings*-*-byte.fst "$TST"/strings.far
"$FARBIN"/farprintstrings --token_type=byte \
  --entry_type=line "$TST"/strings.far > "$TST"/strings
cat "$DAT"/strings1 "$DAT"/strings2 | diff -q - "$TST"/strings

# Token type: utf8, entry type: one fst per line
"$FARBIN"/farcreate "$DAT"/strings*-*-byte.fst "$TST"/strings.far
"$FARBIN"/farprintstrings --token_type=utf8 \
  --entry_type=line "$TST"/strings.far > "$TST"/strings
cat "$DAT"/strings1 "$DAT"/strings2 | diff -q - "$TST"/strings

# Token type: symbol, entry type: one fst per file
"$FARBIN"/farcreate --generate_keys=1 --key_prefix=strings \
  "$DAT"/strings[12]-symbol.fst "$TST"/strings.far
"$FARBIN"/farprintstrings --token_type=symbol --symbols="$DAT"/syms.map \
  --entry_type=file "$TST"/strings.far --filename_prefix="$TST"/
# Explicitly add newline to the output of 'tr' for 'diff' commands below to
# pass on *BSD platforms and avoid using '-B' option which may be too
# permissive. On Linux this is not necessary.
(tr "\n" " " < "$DAT"/strings1 && echo "") | diff -b -q - "$TST"/strings1
(tr "\n" " " < "$DAT"/strings2 && echo "") | diff -b -q - "$TST"/strings2

# Token type: byte, entry type: one fst per file
"$FARBIN"/farcreate --generate_keys=1 --key_prefix=strings \
  "$DAT"/strings[12]-byte.fst "$TST"/strings.far
"$FARBIN"/farprintstrings --token_type=byte \
  --entry_type=file "$TST"/strings.far --filename_prefix="$TST"/
diff -q "$DAT"/strings1 "$TST"/strings1
diff -q "$DAT"/strings2 "$TST"/strings2

# Token type: utf8, entry type: one fst per file
"$FARBIN"/farcreate --generate_keys=1 --key_prefix=strings \
  "$DAT"/strings[12]-byte.fst "$TST"/strings.far
"$FARBIN"/farprintstrings --token_type=utf8 \
  --entry_type=file "$TST"/strings.far --filename_prefix="$TST"/
diff -q "$DAT"/strings1 "$TST"/strings1
diff -q "$DAT"/strings2 "$TST"/strings2

# Empty FAR
[[ "$("$FARBIN"/farprintstrings "$TST"/empty.sttable.far | wc -l)" -eq 0 ]] || die
