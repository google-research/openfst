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

#include "openfst/script/info-impl.h"

#include <cstdint>
#include <ios>
#include <iostream>
#include <ostream>
#include <string>

#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/properties.h"
#include "openfst/lib/util.h"
#include "openfst/script/arcfilter-impl.h"

namespace fst {

void FstInfo::Info() const {
  std::ostream& ostrm = std::cout;
  const auto old = ostrm.setf(std::ios::left);
  PrintField(ostrm, "fst type", FstType());
  PrintField(ostrm, "arc type", ArcType());
  PrintField(ostrm, "input symbol table", InputSymbols());
  PrintField(ostrm, "output symbol table", OutputSymbols());
  if (!LongInfo()) {
    ostrm.setf(old);
    return;
  }
  PrintField(ostrm, "# of states", NumStates());
  PrintField(ostrm, "# of arcs", NumArcs());
  PrintField(ostrm, "initial state", Start());
  PrintField(ostrm, "# of final states", NumFinal());
  PrintField(ostrm, "# of input/output epsilons", NumEpsilons());
  PrintField(ostrm, "# of input epsilons", NumInputEpsilons());
  PrintField(ostrm, "# of output epsilons", NumOutputEpsilons());
  PrintField(ostrm, "input label multiplicity", InputLabelMultiplicity());
  PrintField(ostrm, "output label multiplicity", OutputLabelMultiplicity());
  std::string arc_type = "";
  switch (ArcFilterType()) {
    case script::ArcFilterType::ANY:
      break;
    case script::ArcFilterType::EPSILON: {
      arc_type = "epsilon ";
      break;
    }
    case script::ArcFilterType::INPUT_EPSILON: {
      arc_type = "input-epsilon ";
      break;
    }
    case script::ArcFilterType::OUTPUT_EPSILON: {
      arc_type = "output-epsilon ";
      break;
    }
  }
  PrintField(ostrm, absl::StrCat("# of ", arc_type, "accessible states"),
             NumAccessible());
  PrintField(ostrm, absl::StrCat("# of ", arc_type, "coaccessible states"),
             NumCoAccessible());
  PrintField(ostrm, absl::StrCat("# of ", arc_type, "connected states"),
             NumConnected());
  PrintField(ostrm, absl::StrCat("# of ", arc_type, "connected components"),
             NumCc());
  PrintField(ostrm, absl::StrCat("# of ", arc_type, "strongly conn components"),
             NumScc());
  PrintField(ostrm, "input matcher",
             (InputMatchType() == MATCH_INPUT  ? 'y'
              : InputMatchType() == MATCH_NONE ? 'n'
                                               : '?'));
  PrintField(ostrm, "output matcher",
             (OutputMatchType() == MATCH_OUTPUT ? 'y'
              : OutputMatchType() == MATCH_NONE ? 'n'
                                                : '?'));
  PrintField(ostrm, "input lookahead", (InputLookAhead() ? 'y' : 'n'));
  PrintField(ostrm, "output lookahead", (OutputLookAhead() ? 'y' : 'n'));
  PrintProperties(ostrm, Properties());
  ostrm.setf(old);
}

void PrintProperties(std::ostream& ostrm, const uint64_t properties) {
  uint64_t prop = 1;
  for (auto i = 0; i < 64; ++i, prop <<= 1) {
    if (prop & kBinaryProperties) {
      const char value = properties & prop ? 'y' : 'n';
      PrintField(ostrm, internal::PropertyNames[i], value);
    } else if (prop & kPosTrinaryProperties) {
      char value = '?';
      if (properties & prop) {
        value = 'y';
      } else if (properties & prop << 1) {
        value = 'n';
      }
      PrintField(ostrm, internal::PropertyNames[i], value);
    }
  }
}

void PrintHeader(std::ostream& ostrm, const FstHeader& header) {
  const auto old = ostrm.setf(std::ios::left);
  PrintField(ostrm, "fst type", header.FstType());
  PrintField(ostrm, "arc type", header.ArcType());
  PrintField(ostrm, "version", header.Version());
  // Flags.
  const auto flags = header.GetFlags();
  PrintField(ostrm, "input symbol table",
             (flags & FstHeader::HAS_ISYMBOLS ? 'y' : 'n'));
  PrintField(ostrm, "output symbol table",
             (flags & FstHeader::HAS_OSYMBOLS ? 'y' : 'n'));
  PrintField(ostrm, "aligned", (flags & FstHeader::IS_ALIGNED ? 'y' : 'n'));
  PrintField(ostrm, "initial state", header.Start());
  PrintField(ostrm, "# of states", header.NumStates());
  PrintField(ostrm, "# of arcs", header.NumArcs());
  PrintProperties(ostrm, header.Properties());
  ostrm.setf(old);
}

}  // namespace fst
