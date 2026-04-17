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
//
// Function to verify an FST's contents.

#ifndef OPENFST_LIB_VERIFY_H_
#define OPENFST_LIB_VERIFY_H_

#include <cstddef>
#include <cstdint>

#include "absl/log/log.h"
#include "absl/status/status.h"
#include "absl/strings/str_cat.h"
#include "openfst/lib/expanded-fst.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/properties.h"
#include "openfst/lib/test-properties.h"

namespace fst {

// Verifies that an Fst's contents are sane.
template <class Arc>
absl::Status VerifyWithStatus(const Fst<Arc>& fst,
                              bool allow_negative_labels = false) {
  const auto start = fst.Start();
  const auto* isyms = fst.InputSymbols();
  const auto* osyms = fst.OutputSymbols();
  const auto ns = CountStates(fst);
  if (start == kNoStateId && ns > 0) {
    return absl::InvalidArgumentError("Verify: FST start state ID not set");
  } else if (start >= ns) {
    return absl::InvalidArgumentError(
        "Verify: FST start state ID exceeds number of states");
  }
  for (StateIterator<Fst<Arc>> siter(fst); !siter.Done(); siter.Next()) {
    auto state = siter.Value();
    size_t na = 0;
    for (ArcIterator<Fst<Arc>> aiter(fst, state); !aiter.Done(); aiter.Next()) {
      const auto& arc = aiter.Value();
      if (!allow_negative_labels && arc.ilabel < 0) {
        return absl::InvalidArgumentError(
            absl::StrCat("Verify: FST input label ID of arc at position ", na,
                         " of state ", state, " is negative"));
      } else if (isyms && !isyms->Member(arc.ilabel)) {
        return absl::InvalidArgumentError(absl::StrCat(
            "Verify: FST input label ID ", arc.ilabel, " of arc at position ",
            na, " of state ", state, " is missing from input symbol table \"",
            isyms->Name(), "\""));
      } else if (!allow_negative_labels && arc.olabel < 0) {
        return absl::InvalidArgumentError(
            absl::StrCat("Verify: FST output label ID of arc at position ", na,
                         " of state ", state, " is negative"));
      } else if (osyms && !osyms->Member(arc.olabel)) {
        return absl::InvalidArgumentError(absl::StrCat(
            "Verify: FST output label ID ", arc.olabel, " of arc at position ",
            na, " of state ", state, " is missing from output symbol table \"",
            osyms->Name(), "\""));
      } else if (!arc.weight.Member()) {
        return absl::InvalidArgumentError(
            absl::StrCat("Verify: FST weight of arc at position ", na,
                         " of state ", state, " is invalid"));
      } else if (arc.nextstate < 0) {
        return absl::InvalidArgumentError(
            absl::StrCat("Verify: FST destination state ID of arc at position ",
                         na, " of state ", state, " is negative"));
      } else if (arc.nextstate >= ns) {
        return absl::InvalidArgumentError(
            absl::StrCat("Verify: FST destination state ID of arc at position ",
                         na, " of state ", state, " exceeds number of states"));
      }
      ++na;
    }
    if (!fst.Final(state).Member()) {
      return absl::InvalidArgumentError(absl::StrCat(
          "Verify: FST final weight of state ", state, " is invalid"));
    }
  }
  const auto fst_props = fst.Properties(kFstProperties, /*test=*/false);
  if (fst_props & kError) {
    return absl::InvalidArgumentError("Verify: FST error property is set");
  }
  uint64_t known_props;
  uint64_t test_props =
      internal::ComputeProperties(fst, kFstProperties, &known_props);
  if (!internal::CompatProperties(fst_props, test_props)) {
    return absl::InvalidArgumentError(
        "Verify: Stored FST properties incorrect "
        "(props1 = stored props, props2 = tested)");
  } else {
    return absl::OkStatus();
  }
}

template <class Arc>
[[deprecated("Use VerifyWithStatus() instead")]] bool Verify(
    const Fst<Arc>& fst, bool allow_negative_labels = false) {
  const absl::Status status = VerifyWithStatus(fst, allow_negative_labels);
  if (!status.ok()) {
    LOG(ERROR) << status.message();
    return false;
  }
  return true;
}

}  // namespace fst

#endif  // OPENFST_LIB_VERIFY_H_
