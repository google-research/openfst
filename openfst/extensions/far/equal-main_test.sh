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
# Unit test for farequal.

TEST_SRCDIR="$TEST_SRCDIR/$TEST_WORKSPACE"

set -eou pipefail

source googletest.sh || exit

readonly FST="${TEST_SRCDIR}/openfst"
readonly FARBIN="${FST}/extensions/far"
readonly DAT="${FST}/extensions/far/testdata"

source "${FST}/bin/setup.sh"

readonly FAREQUAL="${FARBIN}/farequal"

"${FAREQUAL}" "${DAT}/test1.sttable.far" "${DAT}/test1.sttable.far"
if "${FAREQUAL}" "${DAT}/test1.sttable.far" "${DAT}/strings.sttable.far"; then
  echo "FAR '${DAT}/test1.sttable.far' equal to '${DAT}/strings.sttable.far'"
  TEST_FAILURE=1
fi

"${FAREQUAL}" "${DAT}/empty.sttable.far" "${DAT}/empty.sttable.far"
if "${FAREQUAL}" "${DAT}/empty.sttable.far" "${DAT}/strings.sttable.far"; then
  echo "FAR '${DAT}/empty.sttable.far' equal to '${DAT}/strings.sttable.far'"
  TEST_FAILURE=1
fi

# macOS still ships with an old version of bash (version 3.2.57) which lacks
# '-v' unary operator. The test below uses an older syntax that should work
# with all versions of bash shells.
if [ "${TEST_FAILURE+x}" ]; then
  echo "Test(s) failed" && exit 1
fi
