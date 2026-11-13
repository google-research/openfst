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
// Prints out binary FSTs in simple text format used by AT&T.

#include <cstring>
#include <iostream>
#include <memory>
#include <ostream>
#include <string>

#include "absl/flags/declare.h"
#include "absl/flags/flag.h"
#include "absl/log/flags.h"
#include "absl/log/log.h"
#include "absl/flags/usage.h"
#include "openfst/compat/init.h"
#include "openfst/lib/file-util.h"
#include "openfst/lib/symbol-table.h"
#include "openfst/script/fst-class.h"
#include "openfst/script/print.h"

ABSL_DECLARE_FLAG(bool, acceptor);
ABSL_DECLARE_FLAG(std::string, isymbols);
ABSL_DECLARE_FLAG(std::string, osymbols);
ABSL_DECLARE_FLAG(std::string, ssymbols);
ABSL_DECLARE_FLAG(bool, numeric);
ABSL_DECLARE_FLAG(std::string, save_isymbols);
ABSL_DECLARE_FLAG(std::string, save_osymbols);
ABSL_DECLARE_FLAG(bool, show_weight_one);
ABSL_DECLARE_FLAG(std::string, missing_symbol);

int fstprint_main(int argc, char** argv) {
  namespace s = fst::script;
  using fst::SymbolTable;
  using fst::script::FstClass;

  std::string usage =
      "Prints out binary FSTs in simple text format.\n\n  Usage: ";
  usage += argv[0];
  usage += " [binary.fst [text.fst]]\n";

  fst::InitOpenFst(usage.c_str(), &argc, &argv, true);
  if (argc > 3) {
    LOG(INFO) << absl::ProgramUsageMessage();
    return 1;
  }

  const std::string in_name =
      (argc > 1 && strcmp(argv[1], "-") != 0) ? argv[1] : "";

  std::unique_ptr<FstClass> fst(FstClass::Read(in_name));
  if (!fst) return 1;

  std::string dest = "standard output";
  file::FileOutStream fstrm;
  if (argc == 3) {
    fstrm.open(argv[2]);
    if (!fstrm) {
      LOG(ERROR) << argv[0] << ": Open failed, file = " << argv[2];
      return 1;
    }
    dest = argv[2];
  }
  std::ostream& ostrm = fstrm.is_open() ? fstrm : std::cout;
  ostrm.precision(9);

  std::unique_ptr<const SymbolTable> isyms;
  if (!absl::GetFlag(FLAGS_isymbols).empty() && !absl::GetFlag(FLAGS_numeric)) {
    isyms.reset(
        SymbolTable::ReadText(absl::GetFlag(FLAGS_isymbols),
                              absl::GetFlag(FLAGS_fst_field_separator)));
    if (!isyms) return 1;
  }

  std::unique_ptr<const SymbolTable> osyms;
  if (!absl::GetFlag(FLAGS_osymbols).empty() && !absl::GetFlag(FLAGS_numeric)) {
    osyms.reset(
        SymbolTable::ReadText(absl::GetFlag(FLAGS_osymbols),
                              absl::GetFlag(FLAGS_fst_field_separator)));
    if (!osyms) return 1;
  }

  std::unique_ptr<const SymbolTable> ssyms;
  if (!absl::GetFlag(FLAGS_ssymbols).empty() && !absl::GetFlag(FLAGS_numeric)) {
    ssyms.reset(
        SymbolTable::ReadText(absl::GetFlag(FLAGS_ssymbols),
                              absl::GetFlag(FLAGS_fst_field_separator)));
    if (!ssyms) return 1;
  }

  if (!isyms && !absl::GetFlag(FLAGS_numeric) && fst->InputSymbols()) {
    isyms.reset(fst->InputSymbols()->Copy());
  }

  if (!osyms && !absl::GetFlag(FLAGS_numeric) && fst->OutputSymbols()) {
    osyms.reset(fst->OutputSymbols()->Copy());
  }

  s::Print(*fst, ostrm, dest, isyms.get(), osyms.get(), ssyms.get(),
           absl::GetFlag(FLAGS_acceptor), absl::GetFlag(FLAGS_show_weight_one),
           absl::GetFlag(FLAGS_missing_symbol));

  if (isyms && !absl::GetFlag(FLAGS_save_isymbols).empty()) {
    if (!isyms->WriteText(absl::GetFlag(FLAGS_save_isymbols))) return 1;
  }

  if (osyms && !absl::GetFlag(FLAGS_save_osymbols).empty()) {
    if (!osyms->WriteText(absl::GetFlag(FLAGS_save_osymbols))) return 1;
  }

  return 0;
}
