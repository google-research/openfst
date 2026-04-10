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
#include <string_view>
#include <utility>

#include "absl/strings/match.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "absl/strings/strip.h"

namespace fst {
namespace {

constexpr std::string_view kPathSeparator = "/";

std::pair<std::string_view, std::string_view> SplitPath(std::string_view path) {
  size_t pos = path.find_last_of(kPathSeparator[0]);

  // Handle the case with no '/' in 'path'.
  if (pos == std::string_view::npos) {
    return std::make_pair(path.substr(0, 0), path);
  }

  // Handle the case with a single leading '/' in 'path'.
  if (pos == 0) {
    return std::make_pair(path.substr(0, 1), absl::ClippedSubstr(path, 1));
  }

  return std::make_pair(path.substr(0, pos),
                        absl::ClippedSubstr(path, pos + 1));
}

}  // namespace

std::string JoinPath(std::string_view path1, std::string_view path2) {
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

std::string JoinPath(std::string_view path1, std::string_view path2,
                     std::string_view path3) {
  return JoinPath(JoinPath(path1, path2), path3);
}

std::string JoinPathRespectAbsolute(std::string_view path1,
                                    std::string_view path2) {
  if (path1.empty()) return std::string(path2);
  if (absl::StartsWith(path2, kPathSeparator)) {
    return std::string(path2);
  }
  return JoinPath(path1, path2);
}

std::string_view Basename(std::string_view path) {
  return SplitPath(path).second;
}

}  // namespace fst
