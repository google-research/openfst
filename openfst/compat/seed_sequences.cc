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

#include "openfst/compat/seed_sequences.h"

#include <cstdint>
#include <iostream>
#include <memory>
#include <optional>
#include <random>
#include <string>

#include "absl/flags/flag.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"

ABSL_FLAG(std::optional<uint64_t>, seed_seq_salt, std::nullopt,
          "Salt for fst::MakeTaggedSeedSeq");

namespace fst {

SeedSeq::SeedSeq(absl::string_view tag) {
  uint64_t salt;
  std::optional<uint64_t> flag_salt = absl::GetFlag(FLAGS_seed_seq_salt);
  if (flag_salt.has_value()) {
    salt = *flag_salt;
  } else {
    std::random_device rd;
    const auto hi = rd(), lo = rd();
    salt = (uint64_t{hi} << 32) + lo;
    std::cerr << "fst::MakeTaggedSeedSeq: generated salt " << salt
              << " for tag '" << tag << "'\n";
  }

  std::string data = absl::StrCat(tag, "_", salt);
  seq_ = std::make_unique<std::seed_seq>(data.begin(), data.end());
}

SeedSeq MakeTaggedSeedSeq(absl::string_view tag) { return SeedSeq(tag); }

}  // namespace fst
