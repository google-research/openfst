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

// Compatibility layer for SeedSequence.

#ifndef OPENFST_COMPAT_SEED_SEQUENCES_H_
#define OPENFST_COMPAT_SEED_SEQUENCES_H_

#include <memory>
#include <random>

#include "absl/strings/string_view.h"

namespace fst {

// A custom SeedSequence wrapper that is movable, allowing it to be returned
// by value from factory functions and passed directly to PRNG constructors.
// This is necessary because std::seed_seq is neither copyable nor movable.
// Additionally, this wrapper allows OpenFst tests to transparently inject
// global test salts (e.g., via FLAGS_seed_seq_salt) to fuzz deterministic
// paths while maintaining reproducibility for golden testing.
class SeedSeq {
 public:
  using result_type = std::seed_seq::result_type;

  explicit SeedSeq(absl::string_view tag);

  template <class InputIt>
  SeedSeq(InputIt begin, InputIt end) {
    seq_ = std::make_unique<std::seed_seq>(begin, end);
  }

  template <class RandomIt>
  void generate(RandomIt begin, RandomIt end) {
    seq_->generate(begin, end);
  }

  size_t size() const { return seq_->size(); }

  template <class OutputIt>
  void param(OutputIt dest) const {
    seq_->param(dest);
  }

 private:
  std::unique_ptr<std::seed_seq> seq_;
};

SeedSeq MakeTaggedSeedSeq(absl::string_view tag);

}  // namespace fst

#endif  // OPENFST_COMPAT_SEED_SEQUENCES_H_
