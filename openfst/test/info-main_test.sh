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
# Unit test for fstinfo.

TEST_SRCDIR="$TEST_SRCDIR/$TEST_WORKSPACE"

set -eou pipefail

TST="$TEST_TMPDIR"
FST="$TEST_SRCDIR/openfst"

BIN="$FST/bin"
DAT="$FST/test/testdata/info"

source "$BIN"/setup.sh

"$BIN"/fstinfo -v=1 "$DAT"/i1.fst > "$TST"/i1.info
cmp "$DAT"/i1.info "$TST"/i1.info

"$BIN"/fstinfo -v=1 < "$DAT"/i1.fst > "$TST"/i1.info
cmp "$DAT"/i1.info "$TST"/i1.info
