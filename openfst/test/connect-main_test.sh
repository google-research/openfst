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
# Unit test for fstconnect.

TEST_SRCDIR=.

set -eou pipefail

TST="$TEST_TMPDIR"
FST="$TEST_SRCDIR/openfst"

BIN="$FST/bin"
DAT="$FST/test/testdata/connect"

source "$BIN"/setup.sh

# file -> file
"$BIN"/fstconnect "$DAT"/c1.fst "$TST"/c2.fst
"$BIN"/fstequal -v=1 "$DAT"/c2.fst "$TST"/c2.fst
/bin/rm "$TST"/c2.fst

# stdin -> file
"$BIN"/fstconnect - "$TST"/c2.fst < "$DAT"/c1.fst
"$BIN"/fstequal -v=1 "$DAT"/c2.fst "$TST"/c2.fst
/bin/rm "$TST"/c2.fst

# file -> stdout
"$BIN"/fstconnect "$DAT"/c1.fst > "$TST"/c2.fst
"$BIN"/fstequal -v=1 "$DAT"/c2.fst "$TST"/c2.fst
/bin/rm "$TST"/c2.fst

# stdin -> stdout
"$BIN"/fstconnect < "$DAT"/c1.fst > "$TST"/c2.fst
"$BIN"/fstequal -v=1 "$DAT"/c2.fst "$TST"/c2.fst
