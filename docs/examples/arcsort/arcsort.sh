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
set -euxo pipefail

FST=$(mktemp)
trap "rm -f $FST" EXIT

fstcompile a.fst $FST
fstdraw --portrait $FST | dot -Tsvg > a.svg
fstarcsort --sort_type=ilabel $FST | fstdraw --portrait | dot -Tsvg > a-ilabel-sorted.svg
fstarcsort --sort_type=olabel $FST | fstdraw --portrait | dot -Tsvg > a-olabel-sorted.svg
