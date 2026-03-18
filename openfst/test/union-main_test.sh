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
# Unit test for fstunion.

TEST_SRCDIR="$TEST_SRCDIR/$TEST_WORKSPACE"

set -eou pipefail

TST="$TEST_TMPDIR"
FST="$TEST_SRCDIR/openfst"

BIN="$FST/bin"
DAT="$FST/test/testdata/union"

source "$BIN"/setup.sh

"$BIN"/fstunion "$DAT"/u1.fst "$DAT"/u2.fst "$TST"/u3.fst
"$BIN"/fstequal -v=1 "$DAT"/u3.fst "$TST"/u3.fst

# stdin 1
"$BIN"/fstunion - "$DAT"/u2.fst "$TST"/u3.fst < "$DAT"/u1.fst
"$BIN"/fstequal -v=1 "$DAT"/u3.fst "$TST"/u3.fst

# stdin 2
"$BIN"/fstunion "$DAT"/u1.fst - "$TST"/u3.fst < "$DAT"/u2.fst
"$BIN"/fstequal -v=1 "$DAT"/u3.fst "$TST"/u3.fst

# stdout
"$BIN"/fstunion "$DAT"/u1.fst "$DAT"/u2.fst > "$TST"/u3.fst
"$BIN"/fstequal -v=1 "$DAT"/u3.fst "$TST"/u3.fst

# pipeline
"$BIN"/fstunion - "$DAT"/u2.fst < "$DAT"/u1.fst > "$TST"/u3.fst
"$BIN"/fstequal -v=1 "$DAT"/u3.fst "$TST"/u3.fst
