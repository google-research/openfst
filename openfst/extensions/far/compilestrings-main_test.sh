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
# Unit test for farcompilestrings.

TEST_SRCDIR="$TEST_SRCDIR/$TEST_WORKSPACE"

set -eou pipefail

TST="$TEST_TMPDIR"
FST="$TEST_SRCDIR/openfst"

FSTBIN="$FST/bin"
FARBIN="$FST/extensions/far"
DAT="$FST/extensions/far/testdata"

source "$FSTBIN"/setup.sh

cd "$TST"

# Token type: symbols, entry type: one fst per line
"$FARBIN"/farcompilestrings --token_type=symbol --symbols="$DAT"/syms.map \
 --entry_type=line "$DAT"/strings1 "$DAT"/strings2 "$TST"/strings.far

"$FARBIN"/farextract --filename_suffix=".fst" "$TST"/strings.far
"$FSTBIN"/fstequal "$DAT"/strings1-1-symbol.fst "$TST"/strings1-1.fst
"$FSTBIN"/fstequal "$DAT"/strings1-2-symbol.fst "$TST"/strings1-2.fst
"$FSTBIN"/fstequal "$DAT"/strings1-3-symbol.fst "$TST"/strings1-3.fst
"$FSTBIN"/fstequal "$DAT"/strings2-1-symbol.fst "$TST"/strings2-1.fst
"$FSTBIN"/fstequal "$DAT"/strings2-2-symbol.fst "$TST"/strings2-2.fst
[[ "$("$FARBIN"/farinfo --list_fsts strings.far  | wc -l)" -eq 6 ]] || die

# Token type: byte, entry type: one fst per line
"$FARBIN"/farcompilestrings --token_type=byte \
 --entry_type=line "$DAT"/strings1 "$DAT"/strings2 "$TST"/strings.far

"$FARBIN"/farextract --filename_suffix=".fst" "$TST"/strings.far
"$FSTBIN"/fstequal "$DAT"/strings1-1-byte.fst "$TST"/strings1-1.fst
"$FSTBIN"/fstequal "$DAT"/strings1-2-byte.fst "$TST"/strings1-2.fst
"$FSTBIN"/fstequal "$DAT"/strings1-3-byte.fst "$TST"/strings1-3.fst
"$FSTBIN"/fstequal "$DAT"/strings2-1-byte.fst "$TST"/strings2-1.fst
"$FSTBIN"/fstequal "$DAT"/strings2-2-byte.fst "$TST"/strings2-2.fst

[[ "$("$FARBIN"/farinfo --list_fsts strings.far  | wc -l)" -eq 6 ]] || die

# Token type: UTF8, entry type: one fst per line
"$FARBIN"/farcompilestrings --token_type=utf8 \
 --entry_type=line "$DAT"/strings1 "$DAT"/strings2 "$TST"/strings.far

"$FARBIN"/farextract --filename_suffix=".fst" "$TST"/strings.far
"$FSTBIN"/fstequal "$DAT"/strings1-1-byte.fst "$TST"/strings1-1.fst
"$FSTBIN"/fstequal "$DAT"/strings1-2-byte.fst "$TST"/strings1-2.fst
"$FSTBIN"/fstequal "$DAT"/strings1-3-byte.fst "$TST"/strings1-3.fst
"$FSTBIN"/fstequal "$DAT"/strings2-1-byte.fst "$TST"/strings2-1.fst
"$FSTBIN"/fstequal "$DAT"/strings2-2-byte.fst "$TST"/strings2-2.fst

[[ "$("$FARBIN"/farinfo --list_fsts strings.far  | wc -l)" -eq 6 ]] || die

# Token type: symbols, entry type: one fst per file
"$FARBIN"/farcompilestrings --token_type=symbol --symbols="$DAT"/syms.map \
 --entry_type=file "$DAT"/strings1 "$DAT"/strings2 "$TST"/strings.far

"$FARBIN"/farextract --filename_suffix=".fst" "$TST"/strings.far
"$FSTBIN"/fstequal "$DAT"/strings1-symbol.fst "$TST"/strings1.fst
"$FSTBIN"/fstequal "$DAT"/strings2-symbol.fst "$TST"/strings2.fst

[[ "$("$FARBIN"/farinfo --list_fsts strings.far  | wc -l)" -eq 3 ]] || die

# Token type: byte, entry type: one fst per file
"$FARBIN"/farcompilestrings --token_type=byte \
 --entry_type=file "$DAT"/strings1 "$DAT"/strings2 "$TST"/strings.far

"$FARBIN"/farextract --filename_suffix=".fst" "$TST"/strings.far
"$FSTBIN"/fstequal "$DAT"/strings1-byte.fst "$TST"/strings1.fst
"$FSTBIN"/fstequal "$DAT"/strings2-byte.fst "$TST"/strings2.fst

[[ "$("$FARBIN"/farinfo --list_fsts strings.far  | wc -l)" -eq 3 ]] || die

# Token type: UTF8, entry type: one fst per file
"$FARBIN"/farcompilestrings --token_type=utf8 \
 --entry_type=file "$DAT"/strings1 "$DAT"/strings2 "$TST"/strings.far

"$FARBIN"/farextract --filename_suffix=".fst" "$TST"/strings.far
"$FSTBIN"/fstequal "$DAT"/strings1-byte.fst "$TST"/strings1.fst
"$FSTBIN"/fstequal "$DAT"/strings2-byte.fst "$TST"/strings2.fst

[[ "$("$FARBIN"/farinfo --list_fsts strings.far  | wc -l)" -eq 3 ]] || die

# Token type: symbols with unknown, entry type: one fst per line
"$FARBIN"/farcompilestrings --token_type=symbol --symbols="$DAT"/syms.map \
 --entry_type=line --unknown_symbol="a" "$DAT"/strings1-unk "$TST"/strings.far

"$FARBIN"/farextract --filename_suffix=".fst" "$TST"/strings.far
"$FSTBIN"/fstequal "$DAT"/strings1-1-symbol.fst "$TST"/strings1-unk-1.fst
"$FSTBIN"/fstequal "$DAT"/strings1-2-symbol.fst "$TST"/strings1-unk-2.fst
"$FSTBIN"/fstequal "$DAT"/strings1-3-symbol.fst "$TST"/strings1-unk-3.fst

[[ "$("$FARBIN"/farinfo --list_fsts strings.far  | wc -l)" -eq 4 ]] || die

# Writing to stdout, token type: symbols, entry type: one fst per line
"$FARBIN"/farcompilestrings --token_type=symbol --symbols="$DAT"/syms.map \
 --entry_type=line --far_type=stlist "$DAT"/strings1 > "$TST"/strings.far

"$FARBIN"/farextract --filename_suffix=".fst" "$TST"/strings.far
"$FSTBIN"/fstequal "$DAT"/strings1-1-symbol.fst "$TST"/strings1-1.fst
"$FSTBIN"/fstequal "$DAT"/strings1-2-symbol.fst "$TST"/strings1-2.fst
"$FSTBIN"/fstequal "$DAT"/strings1-3-symbol.fst "$TST"/strings1-3.fst

[[ "$("$FARBIN"/farinfo --list_fsts strings.far  | wc -l)" -eq 4 ]] || die

# Reading from stdin, writing to stdout,
# token type: symbols, entry type: one fst per line
cat "$DAT"/strings1 |
"$FARBIN"/farcompilestrings --token_type=symbol --symbols="$DAT"/syms.map \
 --entry_type=line --far_type=stlist --generate_keys=1 \
 --key_prefix="strings1-" - > "$TST"/strings.far

"$FARBIN"/farextract --filename_suffix=".fst" "$TST"/strings.far
"$FSTBIN"/fstequal "$DAT"/strings1-1-symbol.fst "$TST"/strings1-1.fst
"$FSTBIN"/fstequal "$DAT"/strings1-2-symbol.fst "$TST"/strings1-2.fst
"$FSTBIN"/fstequal "$DAT"/strings1-3-symbol.fst "$TST"/strings1-3.fst

[[ "$("$FARBIN"/farinfo --list_fsts strings.far  | wc -l)" -eq 4 ]] || die

cat "$DAT"/strings1 |
"$FARBIN"/farcompilestrings --token_type=symbol --symbols="$DAT"/syms.map \
 --entry_type=line --far_type=stlist --generate_keys=1 \
 --key_prefix="strings1-"- - > "$TST"/strings.far

"$FARBIN"/farextract --filename_suffix=".fst" "$TST"/strings.far
"$FSTBIN"/fstequal "$DAT"/strings1-1-symbol.fst "$TST"/strings1-1.fst
"$FSTBIN"/fstequal "$DAT"/strings1-2-symbol.fst "$TST"/strings1-2.fst
"$FSTBIN"/fstequal "$DAT"/strings1-3-symbol.fst "$TST"/strings1-3.fst

[[ "$("$FARBIN"/farinfo --list_fsts strings.far  | wc -l)" -eq 4 ]] || die

# Reading from file list input.
# Token type: symbols, entry type: one fst per line
FILELIST=$TST/filelist
ls "$DAT"/strings[12] > "$FILELIST"
"$FARBIN"/farcompilestrings --token_type=symbol --symbols="$DAT"/syms.map \
 --entry_type=line --file_list_input "$FILELIST" "$TST"/strings.far

"$FARBIN"/farextract --filename_suffix=".fst" "$TST"/strings.far
"$FSTBIN"/fstequal "$DAT"/strings1-1-symbol.fst "$TST"/strings1-1.fst
"$FSTBIN"/fstequal "$DAT"/strings1-2-symbol.fst "$TST"/strings1-2.fst
"$FSTBIN"/fstequal "$DAT"/strings1-3-symbol.fst "$TST"/strings1-3.fst
"$FSTBIN"/fstequal "$DAT"/strings2-1-symbol.fst "$TST"/strings2-1.fst
"$FSTBIN"/fstequal "$DAT"/strings2-2-symbol.fst "$TST"/strings2-2.fst

[[ "$("$FARBIN"/farinfo --list_fsts strings.far  | wc -l)" -eq 6 ]] || die

# Reading from empty file list input.
# Token type: symbols, entry type: one fst per line
"$FARBIN"/farcompilestrings --token_type=symbol --symbols="$DAT"/syms.map \
 --entry_type=line --file_list_input /dev/null "$TST"/empty.far

"$FARBIN"/farextract --filename_suffix=".fst" "$TST"/empty.far

[[ "$("$FARBIN"/farinfo --list_fsts empty.far  | wc -l)" -eq 1 ]] || die
