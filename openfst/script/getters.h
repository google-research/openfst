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
// Getters for converting command-line arguments into the appropriate enums
// or bitmasks, with the simplest ones defined as inline.

#ifndef OPENFST_SCRIPT_GETTERS_H_
#define OPENFST_SCRIPT_GETTERS_H_

#include <cstdint>
#include <limits>

#include "absl/strings/string_view.h"
#include "openfst/lib/compose.h"       // For ComposeFilter.
#include "openfst/lib/determinize.h"   // For DeterminizeType.
#include "openfst/lib/encode.h"        // For kEncodeLabels (etc.).
#include "openfst/lib/epsnormalize.h"  // For EpsNormalizeType.
#include "openfst/lib/project.h"       // For ProjectType.
#include "openfst/lib/push.h"          // For kPushWeights (etc.).
#include "openfst/lib/queue.h"         // For QueueType.
#include "openfst/lib/rational.h"      // For ClosureType.
#include "openfst/lib/replace-util.h"
#include "openfst/lib/reweight.h"
#include "openfst/lib/string.h"             // For TokenType.
#include "openfst/script/arcfilter-impl.h"  // For ArcFilterType.
#include "openfst/script/arcsort.h"         // For ArcSortType.
#include "openfst/script/map.h"             // For MapType.
#include "openfst/script/script-impl.h"     // For RandArcSelection.

namespace fst {
namespace script {

inline constexpr uint64_t kDefaultSeed = std::numeric_limits<uint64_t>::max();

bool GetArcFilterType(absl::string_view str, ArcFilterType* arc_filter_type);

bool GetArcSortType(absl::string_view str, ArcSortType* sort_type);

bool GetClosureType(absl::string_view str, ClosureType* closure_type);

bool GetComposeFilter(absl::string_view str, ComposeFilter* compose_filter);

bool GetDeterminizeType(absl::string_view str, DeterminizeType* det_type);

inline uint8_t GetEncodeFlags(bool encode_labels, bool encode_weights) {
  return (encode_labels ? kEncodeLabels : 0) |
         (encode_weights ? kEncodeWeights : 0);
}

bool GetEpsNormalizeType(absl::string_view str,
                         EpsNormalizeType* eps_norm_type);

bool GetMapType(absl::string_view str, MapType* map_type);

bool GetProjectType(absl::string_view str, ProjectType* project_type);

inline uint8_t GetPushFlags(bool push_weights, bool push_labels,
                            bool remove_total_weight,
                            bool remove_common_affix) {
  return ((push_weights ? kPushWeights : 0) | (push_labels ? kPushLabels : 0) |
          (remove_total_weight ? kPushRemoveTotalWeight : 0) |
          (remove_common_affix ? kPushRemoveCommonAffix : 0));
}

bool GetQueueType(absl::string_view str, QueueType* queue_type);

bool GetRandArcSelection(absl::string_view str, RandArcSelection* ras);

bool GetReplaceLabelType(absl::string_view str, bool epsilon_on_replace,
                         ReplaceLabelType* rlt);

bool GetReweightType(absl::string_view str, ReweightType* reweight_type);

uint64_t GetSeed(uint64_t seed);

bool GetTokenType(absl::string_view str, TokenType* token_type);

}  // namespace script
}  // namespace fst

#endif  // OPENFST_SCRIPT_GETTERS_H_
