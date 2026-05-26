// Copyright 2026 The OpenFst Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "openfst/compat/file_path.h"

#include <cstddef>
#include <string>
#include <utility>

#include "absl/strings/match.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "absl/strings/strip.h"

namespace fst {
namespace {

constexpr absl::string_view kPathSeparator = "/";

std::pair<absl::string_view, absl::string_view> SplitPath(
    absl::string_view path) {
  size_t pos = path.find_last_of(kPathSeparator[0]);

  // Handle the case with no '/' in 'path'.
  if (pos == absl::string_view::npos) {
    return std::make_pair(path.substr(0, 0), path);
  }

  // Handle the case with a single leading '/' in 'path'.
  if (pos == 0) {
    return std::make_pair(path.substr(0, 1), absl::ClippedSubstr(path, 1));
  }

  return std::make_pair(path.substr(0, pos),
                        absl::ClippedSubstr(path, pos + 1));
}

// Return the parts of the basename of path, split on the final ".".
// If there is no "." in the basename or "." is the final character in the
// basename, the second value will be empty.
std::pair<absl::string_view, absl::string_view> SplitBasename(
    absl::string_view path) {
  path = Basename(path);

  const size_t pos = path.find_last_of('.');
  if (pos == absl::string_view::npos)
    return std::make_pair(path, absl::ClippedSubstr(path, path.size(), 0));
  return std::make_pair(path.substr(0, pos),
                        absl::ClippedSubstr(path, pos + 1));
}

}  // namespace

std::string JoinPath(absl::string_view path1, absl::string_view path2) {
  if (path1.empty()) return std::string(path2);
  if (path2.empty()) return std::string(path1);

  if (absl::EndsWith(path1, kPathSeparator)) {
    return absl::StrCat(path1, absl::StripPrefix(path2, kPathSeparator));
  } else {
    if (absl::StartsWith(path2, kPathSeparator)) {
      return absl::StrCat(path1, path2);
    } else {
      return absl::StrCat(path1, kPathSeparator, path2);
    }
  }
}

std::string JoinPath(absl::string_view path1, absl::string_view path2,
                     absl::string_view path3) {
  return JoinPath(JoinPath(path1, path2), path3);
}

std::string JoinPathRespectAbsolute(absl::string_view path1,
                                    absl::string_view path2) {
  if (path1.empty()) return std::string(path2);
  if (absl::StartsWith(path2, kPathSeparator)) {
    return std::string(path2);
  }
  return JoinPath(path1, path2);
}

absl::string_view Basename(absl::string_view path) {
  return SplitPath(path).second;
}

absl::string_view Dirname(absl::string_view path) {
  return SplitPath(path).first;
}

absl::string_view Extension(absl::string_view path) {
  return SplitBasename(path).second;
}

}  // namespace fst
