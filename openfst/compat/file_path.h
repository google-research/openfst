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

// Operations on file paths.

#ifndef OPENFST_COMPAT_FILE_PATH_H_
#define OPENFST_COMPAT_FILE_PATH_H_

#include <string>
#include <string_view>

namespace fst {

// Joins paths, inserting a path separator if necessary.
std::string JoinPath(std::string_view path1, std::string_view path2);
std::string JoinPath(std::string_view path1, std::string_view path2,
                     std::string_view path3);

// Joins paths, inserting a path separator if necessary. If `path2` is an
// absolute path, the result is `path2`.
std::string JoinPathRespectAbsolute(std::string_view path1,
                                    std::string_view path2);

// Returns the part of the path after the final "/".  If there is no
// "/" in the path, the result is the same as the input.
std::string_view Basename(std::string_view path);

}  // namespace fst

#endif  // OPENFST_COMPAT_FILE_PATH_H_
