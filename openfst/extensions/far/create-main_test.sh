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
# Unit test for farcreate.

source googletest.sh || exit

TEST_SRCDIR="$TEST_SRCDIR/$TEST_WORKSPACE"

set -eou pipefail

TST="$TEST_TMPDIR"
FST="$TEST_SRCDIR/openfst"

FSTBIN="$FST/bin"
FARBIN="$FST/extensions/far"
DAT="$FST/extensions/far/testdata"

source "$FSTBIN"/setup.sh

cd "$TST"

# Creates input file list.
FILELIST=filelist.txt
ls "$DAT"/test1-*.fst > $FILELIST

check() {
  "$FARBIN"/farextract "$TST"/test1.far
  "$FSTBIN"/fstequal "$DAT"/test1-01.fst "$TST"/test1-01.fst
  "$FSTBIN"/fstequal "$DAT"/test1-02.fst "$TST"/test1-02.fst
  "$FSTBIN"/fstequal "$DAT"/test1-03.fst "$TST"/test1-03.fst
  if [[ "$FAR_TYPE" = "default" ]]; then
    [[ "$("$FARBIN"/farinfo test1.far | grep sttable | wc -l)" -eq 1 ]] || die
  else
    [[ "$("$FARBIN"/farinfo test1.far | grep "$FAR_TYPE" | wc -l)" -eq 1 ]] || die
  fi
}

check_empty() {
  if [[ "$FAR_TYPE" = "default" ]]; then
    [[ "$("$FARBIN"/farinfo test_empty.far | grep sttable | wc -l)" -eq 1 ]] || die
  else
    [[ "$("$FARBIN"/farinfo test_empty.far | grep "$FAR_TYPE" | wc -l)" -eq 1 ]] || die
  fi
}

for FAR_TYPE in default sttable sttable stlist; do
  # Lists FSTs from argv.
  if [[ $FAR_TYPE = "stlist" ]]; then
    "$FARBIN"/farcreate --far_type=$FAR_TYPE "$DAT"/test1-*.fst - > "$TST"/test1.far
  else
    "$FARBIN"/farcreate --far_type=$FAR_TYPE "$DAT"/test1-*.fst "$TST"/test1.far
  fi
  check

  # Reads FSTs from a file list.
  if [[ $FAR_TYPE = "stlist" ]]; then
    "$FARBIN"/farcreate --far_type=$FAR_TYPE \
                      --file_list_input $FILELIST - > "$TST"/test1.far
  else
    "$FARBIN"/farcreate --far_type=$FAR_TYPE \
                      --file_list_input $FILELIST "$TST"/test1.far
  fi
  check

  # Reads FSTs from an empty file list.
  if [[ $FAR_TYPE = "stlist" ]]; then
    "$FARBIN"/farcreate --far_type=$FAR_TYPE \
                      --file_list_input /dev/null - > "$TST"/test_empty.far
  else
    "$FARBIN"/farcreate --far_type=$FAR_TYPE \
                      --file_list_input /dev/null "$TST"/test_empty.far
  fi
  check_empty
done
