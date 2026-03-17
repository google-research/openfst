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

// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.

#ifndef OPENFST_LIB_FILE_UTIL_H_
#define OPENFST_LIB_FILE_UTIL_H_

#include <cstdio>

#define OPENFST_USE_PORTABLE_FILE 1

#include <fstream>

namespace file {
using FileInStream = std::ifstream;
using FileOutStream = std::ofstream;
}  // namespace file

namespace fst {

// Sets binary mode on the file given file pointer. This is required for
// platforms where the standard input and output streams support binary mode
// by default.
void SetBinaryMode(std::FILE* fp);

}  // namespace fst

#endif  // OPENFST_LIB_FILE_UTIL_H_
