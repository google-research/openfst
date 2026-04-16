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

#include "openfst/extensions/special/sigma-fst.h"

#include <cstdint>
#include <string>

#include "absl/flags/flag.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/register.h"

ABSL_FLAG(int64_t, sigma_fst_sigma_label, 0,
          "Label of transitions to be interpreted as sigma ('any') "
          "transitions");
ABSL_FLAG(std::string, sigma_fst_rewrite_mode, "auto",
          "Rewrite both sides when matching? One of:"
          " \"auto\" (rewrite iff acceptor), \"always\", \"never\"");

namespace fst {

REGISTER_FST(SigmaFst, StdArc);
REGISTER_FST(SigmaFst, LogArc);
REGISTER_FST(SigmaFst, Log64Arc);

REGISTER_FST(InputSigmaFst, StdArc);
REGISTER_FST(InputSigmaFst, LogArc);
REGISTER_FST(InputSigmaFst, Log64Arc);

REGISTER_FST(OutputSigmaFst, StdArc);
REGISTER_FST(OutputSigmaFst, LogArc);
REGISTER_FST(OutputSigmaFst, Log64Arc);

}  // namespace fst
