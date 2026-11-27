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
# Builds, tests, installs, and verifies OpenFst against a pre-installed Abseil
# prefix (`ABSL_PREFIX`), checking installed CLI binaries, shared libraries, and
# the `cmake/install_test` `find_package(OpenFst)` test project.

set -euo pipefail

resolve_lib_dirs() {
  local result="" prefix dir
  for prefix in "$@"; do
    for dir in "${prefix}/lib" "${prefix}/lib64"; do
      if [[ -d "${dir}" ]]; then
        result="${result:+${result}:}${dir}"
      fi
    done
  done
  printf '%s\n' "${result}"
}

verify_linux_shared_deps() {
  local openfst_prefix="$1"
  local ld_path="$2"

  shopt -s nullglob
  local bins=("${openfst_prefix}"/bin/*)
  local libs=(
    "${openfst_prefix}"/lib/lib*.so*
    "${openfst_prefix}"/lib64/lib*.so*
    "${openfst_prefix}"/lib/fst/*.so*
    "${openfst_prefix}"/lib64/fst/*.so*
  )
  shopt -u nullglob

  if (( ${#bins[@]} == 0 )); then
    echo "No installed executables found under ${openfst_prefix}/bin" >&2
    return 1
  fi
  if (( ${#libs[@]} == 0 )); then
    echo "No installed shared libraries found under ${openfst_prefix}" >&2
    return 1
  fi

  local target ldd_out
  for target in "${bins[@]}" "${libs[@]}"; do
    if [[ -L "${target}" ]]; then
      continue
    fi
    ldd_out="$(LD_LIBRARY_PATH="${ld_path}" ldd "${target}")"
    if grep -q 'not found' <<< "${ldd_out}"; then
      echo "Unresolved shared library in ${target}:" >&2
      echo "${ldd_out}" >&2
      return 1
    fi
  done
}

CLEANUP_WORK_DIR=""

cleanup() {
  if [[ -n "${CLEANUP_WORK_DIR}" ]]; then
    rm -rf "${CLEANUP_WORK_DIR}"
  fi
}

main() {
  local script_dir
  script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
  local repo_root
  repo_root="$(cd "${script_dir}/.." && pwd)"

  local absl_prefix="${ABSL_PREFIX:?ABSL_PREFIX must be set}"
  local build_shared="${BUILD_SHARED_LIBS:-OFF}"
  local nproc
  nproc="${NPROC:-$(getconf _NPROCESSORS_ONLN)}"

  local work_dir="${WORK_DIR:-}"
  if [[ -z "${work_dir}" ]]; then
    work_dir="$(mktemp -d "${RUNNER_TEMP:-/tmp}/openfst-install-test.XXXXXX")"
    CLEANUP_WORK_DIR="${work_dir}"
    trap cleanup EXIT
  fi

  local build_dir="${BUILD_DIR:-${work_dir}/build}"
  local openfst_prefix="${OPENFST_PREFIX:-${work_dir}/openfst-install}"
  local install_test_build="${work_dir}/install-test-build"

  local launcher_args=()
  if [[ "${USE_CCACHE:-0}" == "1" ]]; then
    launcher_args=(
      -DCMAKE_C_COMPILER_LAUNCHER=ccache
      -DCMAKE_CXX_COMPILER_LAUNCHER=ccache
    )
  fi

  # `-DOPENFST_BUILD_TESTS=ON` both runs CTest (covering static builds not
  # exercised by `cmake_x64_linux.yml`) and ensures test-only helper targets do
  # not leak into `OpenFstTargets`. `-DOPENFST_ENABLE_PYTHON=OFF` is explicit
  # because the Python extension requires `BUILD_SHARED_LIBS=ON`.
  cmake -S "${repo_root}" -B "${build_dir}" \
    -DCMAKE_BUILD_TYPE=Dev \
    ${launcher_args[@]+"${launcher_args[@]}"} \
    -DCMAKE_PREFIX_PATH="${absl_prefix}" \
    -DBUILD_SHARED_LIBS="${build_shared}" \
    -DOPENFST_ENABLE_INSTALL=ON \
    -DOPENFST_USE_SYSTEM_ABSEIL=ON \
    -DOPENFST_ENABLE_BIN=ON \
    -DOPENFST_BUILD_TESTS=ON \
    -DOPENFST_ENABLE_FSTS=ON \
    -DOPENFST_ENABLE_GRM=ON \
    -DOPENFST_ENABLE_CATEGORIAL=ON \
    -DOPENFST_ENABLE_PYTHON=OFF

  cmake --build "${build_dir}" -j "${nproc}"
  ctest --test-dir "${build_dir}" --output-on-failure -j "${nproc}"
  cmake --install "${build_dir}" --prefix "${openfst_prefix}"

  local runtime_lib_path=""
  if [[ "${build_shared}" == "ON" ]]; then
    # Guard against internal CLI `*-main` helper libraries accidentally being
    # built as uninstalled shared libraries rather than static archives.
    local bad_main_libs
    bad_main_libs="$(
      find "${build_dir}" "${openfst_prefix}" \
        \( -name 'lib*-main.so*' -o -name 'lib*-main*.dylib' \)
    )"
    if [[ -n "${bad_main_libs}" ]]; then
      echo "Unexpected shared *-main libraries found:" >&2
      echo "${bad_main_libs}" >&2
      return 1
    fi

    runtime_lib_path="$(resolve_lib_dirs "${openfst_prefix}" "${absl_prefix}")"

    if [[ "$(uname -s)" == "Linux" ]]; then
      verify_linux_shared_deps "${openfst_prefix}" "${runtime_lib_path}"
    fi
  fi

  # Smoke-test installed CLI executables and verify built-in `vector` and
  # `const` FST type registrations are retained. In static builds, explicitly
  # clear library search paths to ensure binaries have no hidden `.so`/`.dylib`
  # dependencies on `${openfst_prefix}` or `${absl_prefix}`.
  if [[ "${build_shared}" == "ON" ]]; then
    if [[ "$(uname -s)" == "Darwin" ]]; then
      export DYLD_LIBRARY_PATH="${runtime_lib_path}"
    else
      export LD_LIBRARY_PATH="${runtime_lib_path}"
    fi
  else
    unset LD_LIBRARY_PATH DYLD_LIBRARY_PATH
  fi

  printf '0 1 1 2\n1\n' \
    | "${openfst_prefix}/bin/fstcompile" \
    | "${openfst_prefix}/bin/fstconvert" --fst_type=const \
    | "${openfst_prefix}/bin/fstinfo"

  cmake -S "${repo_root}/cmake/install_test" -B "${install_test_build}" \
    -DCMAKE_PREFIX_PATH="${openfst_prefix};${absl_prefix}"
  cmake --build "${install_test_build}" -j "${nproc}"
  "${install_test_build}/install_test" "${work_dir}/install_test.far"
}

main "$@"
