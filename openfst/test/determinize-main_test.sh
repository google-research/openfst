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
# Unit test for fstdeterminize.

TEST_SRCDIR="$TEST_SRCDIR/$TEST_WORKSPACE"

set -eou pipefail

TST="$TEST_TMPDIR"
FST="$TEST_SRCDIR/openfst"

BIN="$FST/bin"
DAT="$FST/test/testdata/determinize"

source "$BIN"/setup.sh

# acceptor determinization
"$BIN"/fstdeterminize "$DAT"/d1.fst "$TST"/d2.fst
"$BIN"/fstequal -v=1 "$DAT"/d2.fst "$TST"/d2.fst

# functional transducer determinization
"$BIN"/fstdeterminize "$DAT"/d3.fst "$TST"/d4.fst
"$BIN"/fstequal -v=1 "$DAT"/d4.fst "$TST"/d4.fst

# nonfunctional transducer determinization
"$BIN"/fstdeterminize -det_type="nonfunctional" "$DAT"/d6.fst "$TST"/d8.fst
"$BIN"/fstequal -v=1 "$DAT"/d8.fst "$TST"/d8.fst

# nonfunctional transducer determinization w/ psubsequential labels
"$BIN"/fstdeterminize \
  -det_type="nonfunctional" \
  -subsequential_label=4 \
  -increment_subsequential_label \
  "$DAT"/d6.fst "$TST"/d9.fst
"$BIN"/fstequal -v=1 "$DAT"/d9.fst "$TST"/d9.fst

# pruned determinization
"$BIN"/fstdeterminize -weight=0.5 -nstate=10 "$DAT"/d1.fst "$TST"/d5.fst
"$BIN"/fstequal -v=1 "$DAT"/d5.fst "$TST"/d5.fst

# min determinization
"$BIN"/fstdeterminize -det_type="disambiguate" "$DAT"/d6.fst "$TST"/d7.fst
"$BIN"/fstequal -v=1 "$DAT"/d7.fst "$TST"/d7.fst

# Check that delta parameter is obeyed
"$BIN"/fstdeterminize "$DAT"/delta_test.fst "$TST"/delta_test_default.fst
"$BIN"/fstequal -v=1 "$DAT"/delta_test_default.fst "$TST"/delta_test_default.fst

"$BIN"/fstdeterminize -delta=100 "$DAT"/delta_test.fst "$TST"/delta_test_100.fst
"$BIN"/fstequal -v=1 "$DAT"/delta_test_100.fst "$TST"/delta_test_100.fst

/bin/rm -f "$TST"/d2.fst

# file > stdout
"$BIN"/fstdeterminize "$DAT"/d1.fst > "$TST"/d2.fst
"$BIN"/fstequal -v=1 "$DAT"/d2.fst "$TST"/d2.fst

/bin/rm -f "$TST"/d2.fst

# stdin > stdout
"$BIN"/fstdeterminize < "$DAT"/d1.fst > "$TST"/d2.fst
"$BIN"/fstequal -v=1 "$DAT"/d2.fst "$TST"/d2.fst
