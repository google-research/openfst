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
#
# Downloads, verifies, builds, and installs the Abseil release pinned in the
# top-level CMakeLists.txt.
#
# Ubuntu's `libabsl-dev` package is older than the pinned Abseil release and
# lacks targets required by OpenFst (such as `absl::status_macros`). Scraping
# the URL and SHA-256 directly from `CMakeLists.txt` keeps CI aligned with the
# pinned `FetchContent_Declare(abseil-cpp ...)` version without duplication.

set -euo pipefail

read_pinned_abseil() {
  local cmake_file="$1"
  python3 - "${cmake_file}" << 'EOF'
import pathlib
import re
import sys

text = pathlib.Path(sys.argv[1]).read_text()
m = re.search(
    r"FetchContent_Declare\(\s*abseil-cpp\b[^)]*"
    r"\bURL\s+(\S+)[^)]*"
    r"\bURL_HASH\s+SHA256=([0-9a-fA-F]{64})",
    text,
)
if not m:
  raise SystemExit("Failed to locate abseil-cpp URL and SHA256 in CMakeLists.txt")
print(m.group(1), m.group(2))
EOF
}

verify_sha256() {
  local expected_sha256="$1"
  local archive_path="$2"
  if command -v sha256sum >/dev/null 2>&1; then
    printf '%s  %s\n' "${expected_sha256}" "${archive_path}" | sha256sum -c -
  else
    printf '%s  %s\n' "${expected_sha256}" "${archive_path}" | shasum -a 256 -c -
  fi
}

ABSL_WORK_DIR=""

cleanup() {
  if [[ -n "${ABSL_WORK_DIR}" ]]; then
    rm -rf "${ABSL_WORK_DIR}"
  fi
}

main() {
  local script_dir
  script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
  local repo_root
  repo_root="$(cd "${script_dir}/.." && pwd)"

  local absl_url absl_sha256
  read -r absl_url absl_sha256 < <(
    read_pinned_abseil "${repo_root}/CMakeLists.txt"
  )

  if [[ "${1:-}" == "--print-sha256" ]]; then
    echo "${absl_sha256}"
    return 0
  fi

  local absl_prefix="${ABSL_PREFIX:?ABSL_PREFIX must be set}"
  local build_shared="${BUILD_SHARED_LIBS:-OFF}"
  local nproc
  nproc="${NPROC:-$(getconf _NPROCESSORS_ONLN)}"
  ABSL_WORK_DIR="$(mktemp -d "${RUNNER_TEMP:-/tmp}/openfst-absl.XXXXXX")"
  trap cleanup EXIT

  local absl_tar="${ABSL_WORK_DIR}/abseil-cpp.tar.gz"
  local absl_src="${ABSL_WORK_DIR}/abseil-src"
  local absl_build="${ABSL_WORK_DIR}/abseil-build"

  curl -fsSL -o "${absl_tar}" "${absl_url}"
  verify_sha256 "${absl_sha256}" "${absl_tar}"

  mkdir -p "${absl_src}"
  tar -xzf "${absl_tar}" --strip-components=1 -C "${absl_src}"

  cmake -S "${absl_src}" -B "${absl_build}" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_STANDARD=17 \
    -DBUILD_SHARED_LIBS="${build_shared}" \
    -DABSL_PROPAGATE_CXX_STD=ON \
    -DABSL_ENABLE_INSTALL=ON \
    -DABSL_BUILD_TESTING=OFF
  cmake --build "${absl_build}" -j "${nproc}"
  cmake --install "${absl_build}" --prefix "${absl_prefix}"
}

main "$@"
