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

// Basic 32-bit checksums.

#ifndef OPENFST_COMPAT_CHECKSUMMER_H_
#define OPENFST_COMPAT_CHECKSUMMER_H_

#include <string>

#include "absl/strings/string_view.h"

namespace fst {

class CheckSummer {
 public:
  CheckSummer() : count_(0) { check_sum_.resize(kCheckSumLength, '\0'); }

  void Reset() {
    count_ = 0;
    for (int i = 0; i < kCheckSumLength; ++i) check_sum_[i] = '\0';
  }

  void Update(absl::string_view data) {
    for (size_t i = 0; i < data.size(); ++i) {
      check_sum_[(count_++) % kCheckSumLength] ^= data[i];
    }
  }

  std::string Digest() const { return check_sum_; }

 private:
  static constexpr int kCheckSumLength = 32;
  int count_;
  std::string check_sum_;
};

}  // namespace fst

#endif  // OPENFST_COMPAT_CHECKSUMMER_H_
