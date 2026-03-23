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

# macOS still ships with an old version of bash (version 3.2.57) which lacks
# '-v' unary operator. The test below uses an older syntax that should work
# with all versions of bash shells.
if [ ! "${LD_LIBRARY_PATH+x}" ]; then
  export LD_LIBRARY_PATH=.
fi
