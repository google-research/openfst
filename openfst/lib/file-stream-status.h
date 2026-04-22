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

#ifndef OPENFST_LIB_FILE_STREAM_STATUS_H_
#define OPENFST_LIB_FILE_STREAM_STATUS_H_

#include <cerrno>
#include <cstring>
#include <fstream>
#include <iostream>

#include "absl/status/status.h"
#include "openfst/lib/file-util.h"
#include "openfst/lib/util.h"

#define LOG_AND_RETURN_IF_STREAM_FAIL(strm, action, msg) \
  do {                                                   \
    action;                                              \
    if ((strm).fail()) {                                 \
      FSTERROR() << (msg);                               \
      return absl::InternalError(msg);                   \
    }                                                    \
  } while (0)

namespace fst {

inline absl::Status GetFileStreamStatus(const std::ios& stream) {
  if (stream.fail()) {
    return absl::InternalError(std::strerror(errno));
  }
  return absl::OkStatus();
}

inline absl::Status CloseFileStream(std::ifstream& stream) {
  stream.close();
  if (stream.fail()) {
    return absl::InternalError(std::strerror(errno));
  }
  return absl::OkStatus();
}

inline absl::Status CloseFileStream(std::ofstream& stream) {
  stream.close();
  if (stream.fail()) {
    return absl::InternalError(std::strerror(errno));
  }
  return absl::OkStatus();
}

inline absl::Status CloseFileStream(std::fstream& stream) {
  stream.close();
  if (stream.fail()) {
    return absl::InternalError(std::strerror(errno));
  }
  return absl::OkStatus();
}

}  // namespace fst

#endif  // OPENFST_LIB_FILE_STREAM_STATUS_H_
