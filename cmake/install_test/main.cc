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
// Smoke test exercising installed `find_package(OpenFst)` CMake targets across
// core (`openfst::fst`), `script` (`openfst::fstscript`), compiled extensions
// (`openfst::far`, `openfst::farscript`), and header-only interface extensions
// (`openfst::pdt`, `openfst::mpdt`, `openfst::compress_elias`).

#include <memory>
#include <utility>
#include <vector>

#include "openfst/extensions/compress/elias.h"
#include "openfst/extensions/far/far-class.h"
#include "openfst/extensions/far/far-type.h"
#include "openfst/extensions/far/getters.h"
#include "openfst/extensions/mpdt/expand.h"
#include "openfst/extensions/pdt/expand.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/symbol-table.h"
#include "openfst/lib/vector-fst.h"
#include "openfst/script/arcsort.h"
#include "openfst/script/fst-class.h"

int main(int argc, char** argv) {
  if (argc != 2) return 1;

  fst::SymbolTable syms;
  syms.AddSymbol("eps", 0);

  fst::StdVectorFst fst;
  const auto s0 = fst.AddState();
  fst.SetStart(s0);
  fst.SetFinal(s0, fst::StdArc::Weight::One());

  fst::script::VectorFstClass vfst(fst);
  fst::script::ArcSort(&vfst, fst::script::ArcSortType::ILABEL);
  if (vfst.NumStates() != 1) return 1;

  fst::FarType far_type = fst::FarType::DEFAULT;
  if (!fst::script::GetFarType("sttable", &far_type) ||
      far_type != fst::FarType::STTABLE) {
    return 1;
  }

  auto writer = fst::script::FarWriterClass::Create(
      argv[1], fst::StdArc::Type(), far_type);
  if (!writer || !writer->Add("k", vfst) || writer->Error()) return 1;

  fst::StdVectorFst pdt_expanded;
  const std::vector<std::pair<fst::StdArc::Label, fst::StdArc::Label>> parens;
  fst::Expand(fst, parens, &pdt_expanded);
  if (pdt_expanded.NumStates() != 1) return 1;

  fst::StdVectorFst mpdt_expanded;
  const std::vector<fst::StdArc::Label> assignments;
  fst::Expand(fst, parens, assignments, &mpdt_expanded);
  if (mpdt_expanded.NumStates() != 1) return 1;

  std::vector<bool> code;
  fst::Elias<int>::DeltaEncode(7, &code);
  std::vector<int> decoded;
  fst::Elias<int>::BatchDecode(code, &decoded);
  if (decoded.size() != 1 || decoded[0] != 7) return 1;

  return 0;
}
