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

#include "openfst/extensions/linear/linearscript.h"

#include <cctype>
#include <cstddef>
#include <functional>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "absl/flags/flag.h"
#include "absl/log/log.h"
#include "openfst/lib/file-util.h"
#include "openfst/script/script-impl.h"

ABSL_FLAG(std::string, delimiter, "|",
          "Single non-white-space character delimiter inside sequences of "
          "feature symbols and output symbols");
ABSL_FLAG(std::string, empty_symbol, "<empty>",
          "Special symbol that designates an empty sequence");

ABSL_FLAG(std::string, start_symbol, "<s>", "Start of sentence symbol");
ABSL_FLAG(std::string, end_symbol, "</s>", "End of sentence symbol");

ABSL_FLAG(bool, classifier, false,
          "Treat input model as a classifier instead of a tagger");

namespace fst {
namespace script {

bool ValidateDelimiter() {
  return absl::GetFlag(FLAGS_delimiter).size() == 1 &&
         !std::isspace(absl::GetFlag(FLAGS_delimiter)[0]);
}

bool ValidateEmptySymbol() {
  bool okay = !absl::GetFlag(FLAGS_empty_symbol).empty();
  for (size_t i = 0; i < absl::GetFlag(FLAGS_empty_symbol).size(); ++i) {
    char c = absl::GetFlag(FLAGS_empty_symbol)[i];
    if (std::isspace(c)) okay = false;
  }
  return okay;
}

void LinearCompile(const std::string& arc_type,
                   const std::string& epsilon_symbol,
                   const std::string& unknown_symbol, const std::string& vocab,
                   char** models, int models_len, const std::string& out,
                   const std::string& save_isymbols,
                   const std::string& save_fsymbols,
                   const std::string& save_osymbols) {
  LinearCompileArgs args(epsilon_symbol, unknown_symbol, vocab, models,
                         models_len, out, save_isymbols, save_fsymbols,
                         save_osymbols);
  Apply<Operation<LinearCompileArgs>>("LinearCompileTpl", arc_type, &args);
}

REGISTER_FST_OPERATION_3ARCS(LinearCompileTpl, LinearCompileArgs);

void SplitByWhitespace(const std::string& str, std::vector<std::string>* out) {
  out->clear();
  std::istringstream strm(str);
  std::string buf;
  while (strm >> buf) out->push_back(buf);
}

int ScanNumClasses(char** models, int models_len) {
  std::set<std::string, std::less<>> preds;
  for (int i = 0; i < models_len; ++i) {
    file::FileInStream in(models[i]);
    if (!in) LOG(FATAL) << "Failed to open " << models[i];  // Crash OK.
    std::string line;
    std::getline(in, line);
    size_t num_line = 1;
    while (std::getline(in, line)) {
      ++num_line;
      std::vector<std::string> fields;
      SplitByWhitespace(line, &fields);
      if (fields.size() != 3) {
        LOG(FATAL)  // Crash OK.
            << "Wrong number of fields in source " << models[i] << ", line "
            << num_line;
      }
      preds.insert(fields[1]);
    }
  }
  return preds.size();
}

}  // namespace script
}  // namespace fst
