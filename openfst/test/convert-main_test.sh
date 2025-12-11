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
# Unit test for fstconvert.

TEST_SRCDIR=.

set -eou pipefail

TST="$TEST_TMPDIR"
FST="$TEST_SRCDIR/openfst"

BIN="$FST/bin"
DAT="$FST/test/testdata/convert"

source "$BIN"/setup.sh

# convert to nonexistent format, which should produce return code 1 (other
# non-zero return codes probably indicate a segfault)
"$BIN"/fstconvert -fst_error_fatal=false -fst_type=spam "$DAT"/c1.fst "$TST"/spam.fst || RC=$?
check_eq $RC 1

# convert from vector to const
"$BIN"/fstconvert -fst_type=const "$DAT"/c1.fst "$TST"/const.fst
"$BIN"/fstequal -v=1 "$DAT"/c1.fst "$TST"/const.fst

# convert from const to vector
"$BIN"/fstconvert -fst_type=vector "$TST"/const.fst "$TST"/vector.fst
"$BIN"/fstequal -v=1 "$DAT"/c1.fst "$TST"/vector.fst

# convert from vector to compact_acceptor
"$BIN"/fstproject "$TST"/vector.fst "$TST"/acceptor.fst
"$BIN"/fstconvert -fst_type=compact_acceptor "$TST"/acceptor.fst "$TST"/compact.fst
"$BIN"/fstequal -v=1 "$TST"/acceptor.fst "$TST"/compact.fst

# convert from compact_acceptor to const
"$BIN"/fstconvert -fst_type=const "$TST"/compact.fst "$TST"/const.fst
"$BIN"/fstequal -v=1 "$TST"/const.fst "$TST"/acceptor.fst

# convert from compact_acceptor to vector
"$BIN"/fstconvert -fst_type=vector "$TST"/compact.fst "$TST"/vector.fst
"$BIN"/fstequal -v=1 "$TST"/vector.fst "$TST"/acceptor.fst

# convert from vector to compact_unweighted
"$BIN"/fstmap -map_type=rmweight "$TST"/vector.fst "$TST"/unweighted.fst
"$BIN"/fstconvert -fst_type=compact_unweighted "$TST"/unweighted.fst \
 "$TST"/compact.fst
"$BIN"/fstequal -v=1 "$TST"/unweighted.fst "$TST"/compact.fst

# convert from compact_unweighted to const
"$BIN"/fstconvert -fst_type=const "$TST"/compact.fst "$TST"/const.fst
"$BIN"/fstequal -v=1 "$TST"/const.fst "$TST"/unweighted.fst

# convert from compact_unweighted to vector
"$BIN"/fstconvert -fst_type=vector "$TST"/compact.fst "$TST"/vector.fst
"$BIN"/fstequal -v=1 "$TST"/vector.fst "$TST"/unweighted.fst

# convert from vector to compact_unweighted_acceptor
"$BIN"/fstmap -map_type=rmweight "$TST"/acceptor.fst "$TST"/unweighted_acceptor.fst
"$BIN"/fstconvert -fst_type=compact_unweighted_acceptor \
 "$TST"/unweighted_acceptor.fst "$TST"/compact.fst
"$BIN"/fstequal -v=1 "$TST"/unweighted_acceptor.fst "$TST"/compact.fst

# convert from compact_unweighted_acceptor to const
"$BIN"/fstconvert -fst_type=const "$TST"/compact.fst "$TST"/const.fst
"$BIN"/fstequal -v=1 "$TST"/const.fst "$TST"/unweighted_acceptor.fst

# convert from compact_unweighted_acceptor to vector
"$BIN"/fstconvert -fst_type=vector "$TST"/compact.fst "$TST"/vector.fst
"$BIN"/fstequal -v=1 "$TST"/vector.fst "$TST"/unweighted_acceptor.fst

# convert from vector to compact_weighted_string
"$BIN"/fstproject "$DAT"/c2.fst "$TST"/weighted_string.fst
"$BIN"/fstconvert -fst_type=compact_weighted_string "$TST"/weighted_string.fst \
 "$TST"/compact.fst

# convert from compact_weighted_string to const
"$BIN"/fstconvert -fst_type=const "$TST"/compact.fst "$TST"/const.fst
"$BIN"/fstequal -v=1 "$TST"/const.fst "$TST"/weighted_string.fst

# convert from compact_unweighted_acceptor to vector
"$BIN"/fstconvert -fst_type=vector "$TST"/compact.fst "$TST"/vector.fst
"$BIN"/fstequal -v=1 "$TST"/vector.fst "$TST"/weighted_string.fst

# convert from vector to compact_string
"$BIN"/fstmap --map_type=rmweight  "$TST"/weighted_string.fst "$TST"/string.fst
"$BIN"/fstconvert -fst_type=compact_string "$TST"/string.fst "$TST"/compact.fst

# convert from compact_string to const
"$BIN"/fstconvert -fst_type=const "$TST"/compact.fst "$TST"/const.fst
"$BIN"/fstequal -v=1 "$TST"/const.fst "$TST"/string.fst

# convert from compact_unweighted_acceptor to vector
"$BIN"/fstconvert -fst_type=vector "$TST"/compact.fst "$TST"/vector.fst
"$BIN"/fstequal -v=1 "$TST"/vector.fst "$TST"/string.fst

# convert stdin to stdout
"$BIN"/fstconvert -fst_type=vector < "$TST"/compact.fst > "$TST"/vector.fst
"$BIN"/fstequal -v=1 "$TST"/vector.fst "$TST"/string.fst

# convert file to stdout
"$BIN"/fstconvert -fst_type=vector "$TST"/compact.fst > "$TST"/vector.fst
"$BIN"/fstequal -v=1 "$TST"/vector.fst "$TST"/string.fst
