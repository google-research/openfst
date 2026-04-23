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
# Unit test for farconvert.

TEST_SRCDIR="$TEST_SRCDIR/$TEST_WORKSPACE"

set -eou pipefail

FST="$TEST_SRCDIR/openfst"
FARBIN="$FST/extensions/far"
DAT="$FST/extensions/far/testdata"

FARCONVERT="$FARBIN/farconvert"
FAREQUAL="$FARBIN/farequal"
FARINFO="$FARBIN/farinfo"

source "$FST/bin/setup.sh"

TST="$TEST_TMPDIR"
cd "$TST"

# empty.sttable.far: sttable error ""
# There is no fst_type since error arcs can never be created.
"$FARINFO" "$DAT/empty.sttable.far" | grep '^far type.*sttable$'
"$FARINFO" "$DAT/empty.sttable.far" | grep '^arc type.*error$'
"$FARINFO" "$DAT/empty.sttable.far" | grep '^fst type.* $'

# Convert empty to sttable, not changing arc_type;
# this is probably the most common use case.
"$FARCONVERT" --far_type=sttable \
  "$DAT/empty.sttable.far" empty.sttable.far
"$FAREQUAL" "$DAT/empty.sttable.far" empty.sttable.far
"$FARINFO" empty.sttable.far | grep '^far type.*sttable$'
"$FARINFO" empty.sttable.far | grep '^arc type.*error$'
"$FARINFO" empty.sttable.far | grep '^fst type.* $'

# Convert empty to const, not changing far_type.
"$FARCONVERT" --fst_type=const \
  "$DAT/empty.sttable.far" empty.sttable.const.far
"$FAREQUAL" "$DAT/empty.sttable.far" empty.sttable.const.far
"$FARINFO" empty.sttable.const.far | grep '^far type.*sttable$'
# arc_type remains error, since there are no FSTs.
"$FARINFO" empty.sttable.const.far | grep '^arc type.*error$'
"$FARINFO" empty.sttable.const.far | grep '^fst type.* $'

# test1.far: sttable log vector
"$FARINFO" "$DAT/test1.sttable.far" | grep '^far type.*sttable$'
"$FARINFO" "$DAT/test1.sttable.far" | grep '^arc type.*log$'
"$FARINFO" "$DAT/test1.sttable.far" | grep '^fst type.*vector$'

# Convert test1 to sttable const
"$FARCONVERT" --far_type=sttable --fst_type=const \
  "$DAT/test1.sttable.far" test1.sttable.const.far
"$FAREQUAL" "$DAT/test1.sttable.far" test1.sttable.const.far
"$FARINFO" test1.sttable.const.far | grep '^far type.*sttable$'
"$FARINFO" test1.sttable.const.far | grep '^arc type.*log$'
"$FARINFO" test1.sttable.const.far | grep '^fst type.*const$'

# Convert test1 to sttable vector. The FST type is explicitly
# specified, but the same as the input type.
"$FARCONVERT" --far_type=sttable --fst_type=vector \
  "$DAT/test1.sttable.far" test1.sttable.vector.far
"$FAREQUAL" "$DAT/test1.sttable.far" test1.sttable.vector.far
"$FARINFO" test1.sttable.vector.far | grep '^far type.*sttable$'
"$FARINFO" test1.sttable.vector.far | grep '^arc type.*log$'
"$FARINFO" test1.sttable.vector.far | grep '^fst type.*vector$'

# Test one arg version, read from far, write to stdout.
# sttable and sttable cannot write to stdout.
"$FARCONVERT" --far_type=stlist --fst_type=const \
  "$DAT/test1.sttable.far" > test1.stlist.const.stdout.far
"$FAREQUAL" "$DAT/test1.sttable.far" test1.stlist.const.stdout.far
"$FARINFO" test1.stlist.const.stdout.far | grep '^far type.*stlist$'
"$FARINFO" test1.stlist.const.stdout.far | grep '^arc type.*log$'
"$FARINFO" test1.stlist.const.stdout.far | grep '^fst type.*const$'

# Same thing with "-" for output filename.
"$FARCONVERT" --far_type=stlist --fst_type=const \
  "$DAT/test1.sttable.far" - > test1.stlist.const.stdout2.far
"$FAREQUAL" "$DAT/test1.sttable.far" test1.stlist.const.stdout2.far
"$FARINFO" test1.stlist.const.stdout2.far | grep '^far type.*stlist$'
"$FARINFO" test1.stlist.const.stdout2.far | grep '^arc type.*log$'
"$FARINFO" test1.stlist.const.stdout2.far | grep '^fst type.*const$'

# Same thing with "/dev/stdout" for output filename.
# Under MSYS2 /dev/stdout works in bash, but not in farconvert.
if [[ -e /dev/stdout && "$OSTYPE" != msys* ]]; then
  "$FARCONVERT" --far_type=stlist --fst_type=const \
    "$DAT/test1.sttable.far" /dev/stdout > test1.stlist.const.stdout3.far
  "$FAREQUAL" "$DAT/test1.sttable.far" test1.stlist.const.stdout3.far
  "$FARINFO" test1.stlist.const.stdout3.far | grep '^far type.*stlist$'
  "$FARINFO" test1.stlist.const.stdout3.far | grep '^arc type.*log$'
  "$FARINFO" test1.stlist.const.stdout3.far | grep '^fst type.*const$'
fi

# Reading from stdin is only supported with stlist, so that is tested below.

# test2.far: stlist standard vector
"$FARINFO" "$DAT/test2.stlist.far" | grep '^far type.*stlist$'
"$FARINFO" "$DAT/test2.stlist.far" | grep '^arc type.*standard$'
"$FARINFO" "$DAT/test2.stlist.far" | grep '^fst type.*vector$'

# Convert test2 to const; no far_type means leave the same.
"$FARCONVERT" --fst_type=const "$DAT/test2.stlist.far" test2.const.far
"$FAREQUAL" "$DAT/test2.stlist.far" test2.const.far
"$FARINFO" test2.const.far | grep '^far type.*stlist$'
"$FARINFO" test2.const.far | grep '^arc type.*standard$'
"$FARINFO" test2.const.far | grep '^fst type.*const$'

# Convert test2 to sttable
"$FARCONVERT" --far_type=sttable "$DAT/test2.stlist.far" test2.sttable.far
"$FAREQUAL" "$DAT/test2.stlist.far" test2.sttable.far
"$FARINFO" test2.sttable.far | grep '^far type.*sttable$'
"$FARINFO" test2.sttable.far | grep '^arc type.*standard$'
"$FARINFO" test2.sttable.far | grep '^fst type.*vector$'

# Convert test2 to sttable const
"$FARCONVERT" --far_type=sttable --fst_type=const \
  "$DAT/test2.stlist.far" test2.sttable.const.far
"$FAREQUAL" "$DAT/test2.stlist.far" test2.sttable.const.far
"$FARINFO" test2.sttable.const.far | grep '^far type.*sttable$'
"$FARINFO" test2.sttable.const.far | grep '^arc type.*standard$'
"$FARINFO" test2.sttable.const.far | grep '^fst type.*const$'

# Test zero arg version, read from stdin, write to stdout.
# Only stlist can be read from stdin.
"$FARCONVERT" --far_type=stlist --fst_type=const \
  < "$DAT/test2.stlist.far" > test2.stlist.const.stdinout.far
"$FAREQUAL" "$DAT/test2.stlist.far" test2.stlist.const.stdinout.far
"$FARINFO" test2.stlist.const.stdinout.far | grep '^far type.*stlist$'
"$FARINFO" test2.stlist.const.stdinout.far | grep '^arc type.*standard$'
"$FARINFO" test2.stlist.const.stdinout.far | grep '^fst type.*const$'

# Test "-" for input file, no output file.
"$FARCONVERT" --far_type=stlist --fst_type=const \
  - < "$DAT/test2.stlist.far" > test2.stlist.const.stdinout2.far
"$FAREQUAL" "$DAT/test2.stlist.far" test2.stlist.const.stdinout2.far
"$FARINFO" test2.stlist.const.stdinout2.far | grep '^far type.*stlist$'
"$FARINFO" test2.stlist.const.stdinout2.far | grep '^arc type.*standard$'
"$FARINFO" test2.stlist.const.stdinout2.far | grep '^fst type.*const$'

# Test "-" for input file, with output file.
"$FARCONVERT" --far_type=stlist --fst_type=const \
  - test2.stlist.const.stdinout3.far < "$DAT/test2.stlist.far"
"$FAREQUAL" "$DAT/test2.stlist.far" test2.stlist.const.stdinout3.far
"$FARINFO" test2.stlist.const.stdinout3.far | grep '^far type.*stlist$'
"$FARINFO" test2.stlist.const.stdinout3.far | grep '^arc type.*standard$'
"$FARINFO" test2.stlist.const.stdinout3.far | grep '^fst type.*const$'
