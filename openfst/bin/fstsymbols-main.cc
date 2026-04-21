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
// Performs operations (set, clear, relabel) on the symbols table attached to an
// input FST.

#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "absl/flags/usage.h"
#include "openfst/compat/init.h"
#include "absl/flags/declare.h"
#include "absl/flags/flag.h"
#include "absl/log/flags.h"
#include "absl/log/log.h"
#include "openfst/lib/symbol-table.h"
#include "openfst/lib/util.h"
#include "openfst/script/fst-class.h"
#include "openfst/script/verify.h"

ABSL_DECLARE_FLAG(std::string, isymbols);
ABSL_DECLARE_FLAG(std::string, osymbols);
ABSL_DECLARE_FLAG(bool, clear_isymbols);
ABSL_DECLARE_FLAG(bool, clear_osymbols);
ABSL_DECLARE_FLAG(std::string, relabel_ipairs);
ABSL_DECLARE_FLAG(std::string, relabel_opairs);
ABSL_DECLARE_FLAG(std::string, save_isymbols);
ABSL_DECLARE_FLAG(std::string, save_osymbols);
ABSL_DECLARE_FLAG(bool, verify);

int fstsymbols_main(int argc, char** argv) {
  namespace s = fst::script;
  using fst::ReadLabelPairs;
  using fst::RelabelSymbolTable;
  using fst::SymbolTable;
  using fst::script::MutableFstClass;

  std::string usage =
      "Performs operations (set, clear, relabel) on the symbol"
      " tables attached to an FST.\n\n  Usage: ";
  usage += argv[0];
  usage += " [in.fst [out.fst]]\n";

  fst::InitOpenFst(usage.c_str(), &argc, &argv, true);
  if (argc > 3) {
    LOG(INFO) << absl::ProgramUsageMessage();
    return 1;
  }

  const std::string in_name =
      argc > 1 && strcmp(argv[1], "-") != 0 ? argv[1] : "";
  const std::string out_name =
      argc > 2 && strcmp(argv[2], "-") != 0 ? argv[2] : "";

  std::unique_ptr<MutableFstClass> fst(MutableFstClass::Read(in_name, true));
  if (!fst) return 1;

  if (!absl::GetFlag(FLAGS_save_isymbols).empty()) {
    const auto* isyms = fst->InputSymbols();
    if (isyms) {
      isyms->WriteText(absl::GetFlag(FLAGS_save_isymbols));
    } else {
      LOG(ERROR) << argv[0]
                 << ": Saving isymbols but there are no input symbols.";
    }
  }

  if (!absl::GetFlag(FLAGS_save_osymbols).empty()) {
    const auto* osyms = fst->OutputSymbols();
    if (osyms) {
      osyms->WriteText(absl::GetFlag(FLAGS_save_osymbols));
    } else {
      LOG(ERROR) << argv[0]
                 << ": Saving osymbols but there are no output symbols.";
    }
  }

  std::unique_ptr<SymbolTable> isyms;
  if (!absl::GetFlag(FLAGS_isymbols).empty()) {
    isyms.reset(
        SymbolTable::ReadText(absl::GetFlag(FLAGS_isymbols),
                              absl::GetFlag(FLAGS_fst_field_separator)));
    fst->SetInputSymbols(isyms.get());
  } else if (absl::GetFlag(FLAGS_clear_isymbols)) {
    fst->SetInputSymbols(nullptr);
  }
  std::unique_ptr<SymbolTable> osyms;
  if (!absl::GetFlag(FLAGS_osymbols).empty()) {
    osyms.reset(
        SymbolTable::ReadText(absl::GetFlag(FLAGS_osymbols),
                              absl::GetFlag(FLAGS_fst_field_separator)));
    fst->SetOutputSymbols(osyms.get());
  } else if (absl::GetFlag(FLAGS_clear_osymbols)) {
    fst->SetOutputSymbols(nullptr);
  }

  using Label = int64_t;
  if (!absl::GetFlag(FLAGS_relabel_ipairs).empty()) {
    std::vector<std::pair<Label, Label>> ipairs;
    ReadLabelPairs(absl::GetFlag(FLAGS_relabel_ipairs), &ipairs);
    std::unique_ptr<SymbolTable> isyms_relabel(
        RelabelSymbolTable<Label>(fst->InputSymbols(), ipairs));
    fst->SetInputSymbols(isyms_relabel.get());
  }
  if (!absl::GetFlag(FLAGS_relabel_opairs).empty()) {
    std::vector<std::pair<Label, Label>> opairs;
    ReadLabelPairs(absl::GetFlag(FLAGS_relabel_opairs), &opairs);
    std::unique_ptr<SymbolTable> osyms_relabel(
        RelabelSymbolTable<Label>(fst->OutputSymbols(), opairs));
    fst->SetOutputSymbols(osyms_relabel.get());
  }

  if (absl::GetFlag(FLAGS_verify) && !s::Verify(*fst)) return 1;

  return !fst->Write(out_name);
}
