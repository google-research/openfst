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

// This binary is a proxy for all FST tools in
// //openfst/extensions/far.
//
// It can be used by calling
//   far <command> [flags]
// e.g.
//   far printstrings in.far
//
// Alternatively, it can be accessed via symbolic links named like the original
// tool. For example:
//   ln -s far farprintstrings
//   ./farprintstrings in.far

#include <cstdint>
#include <string>
#include <vector>

#include "absl/flags/flag.h"
#include "openfst/lib/weight.h"
#include "openfst/static/command_dispatcher.h"

// Flags are DEFINED in the section for the first tool that uses them.
// See GetFarCommands() for complete tool -> flag mapping.

// farcompilestrings
ABSL_FLAG(std::string, key_prefix, "", "Prefix to append to keys");
ABSL_FLAG(std::string, key_suffix, "", "Suffix to append to keys");
ABSL_FLAG(int32_t, generate_keys, 0,
          "Generate N digit numeric keys (def: use file basenames)");
ABSL_FLAG(std::string, far_type, "default",
          "FAR file format type: one of: \"default\", \"fst\", "
          "\"stlist\", \"sttable\"; "
          "the input FAR type is used if \"default\"");
ABSL_FLAG(bool, allow_negative_labels, false,
          "Allow negative labels (not recommended; may cause conflicts)");
ABSL_FLAG(std::string, arc_type, "standard", "Output arc type");
ABSL_FLAG(std::string, entry_type, "line",
          "Entry type: one of : "
          "\"file\" (one FST per file), \"line\" (one FST per line)");
ABSL_FLAG(std::string, fst_type, "", "Output FST type");
ABSL_FLAG(std::string, token_type, "symbol",
          "Token type: one of : "
          "\"symbol\", \"byte\", \"utf8\"");
ABSL_FLAG(std::string, symbols, "", "Label symbol table");
ABSL_FLAG(std::string, unknown_symbol, "", "");
ABSL_FLAG(bool, file_list_input, false,
          "Each input file contains a list of files to be processed");
ABSL_FLAG(bool, keep_symbols, false, "Store symbol table in the FAR file");
ABSL_FLAG(bool, initial_symbols, true,
          "When keep_symbols is true, stores symbol table only for the first"
          " FST in archive.");

// farcreate
// All flags already defined above.

// farconvert
// All flags already defined above.

// farencode
ABSL_FLAG(bool, decode, false, "Decode labels and/or weights");
ABSL_FLAG(bool, encode_labels, false, "Encode output labels");
ABSL_FLAG(bool, encode_reuse, false, "Re-use existing mapper");
ABSL_FLAG(bool, encode_weights, false, "Encode weights");

// farequal
ABSL_FLAG(double, delta, fst::kDelta, "Comparison/quantization delta");

// farextract
ABSL_FLAG(std::string, filename_prefix, "", "Prefix to append to filenames");
ABSL_FLAG(std::string, filename_suffix, "", "Suffix to append to filenames");
ABSL_FLAG(int32_t, generate_filenames, 0,
          "Generate N digit numeric filenames (def: use keys)");
ABSL_FLAG(std::string, keys, "",
          "Extract set of keys separated by comma (default) "
          "including ranges delimited by dash (default)");
ABSL_FLAG(std::string, key_separator, ",", "Separator for individual keys");
ABSL_FLAG(std::string, range_delimiter, "-", "Delimiter for ranges of keys");

// farinfo
ABSL_FLAG(std::string, begin_key, "",
          "First key to extract (default: first key in archive)");
ABSL_FLAG(std::string, end_key, "",
          "Last key to extract (default: last key in archive)");

ABSL_FLAG(bool, list_fsts, false, "Display FST information for each key");

// farisomorphic
ABSL_FLAG(bool, print_key, false, "Prefix each std::string by its key");
ABSL_FLAG(bool, print_weight, false, "Suffix each std::string by its weight");

int farcompilestrings_main(int argc, char** argv);
int farconvert_main(int argc, char** argv);
int farcreate_main(int argc, char** argv);
int farencode_main(int argc, char** argv);
int farequal_main(int argc, char** argv);
int farextract_main(int argc, char** argv);
int farinfo_main(int argc, char** argv);
int farisomorphic_main(int argc, char** argv);
int farprintstrings_main(int argc, char** argv);

namespace {

using ::fst::Command;
using ::fst::CommandDispatcher;

std::vector<Command> GetFarCommands() {
  return {
      {"farcompilestrings",
       &farcompilestrings_main,
       {"key_prefix", "key_suffix", "generate_keys", "far_type",
        "allow_negative_labels", "arc_type", "entry_type", "fst_type",
        "token_type", "symbols", "unknown_symbol", "file_list_input",
        "keep_symbols", "initial_symbols"}},
      {"farconvert", &farconvert_main, {"far_type", "fst_type"}},
      {"farcreate",
       &farcreate_main,
       {"key_prefix", "key_suffix", "generate_keys", "far_type",
        "file_list_input"}},
      {"farencode",
       &farencode_main,
       {"encode_labels", "encode_weights", "encode_reuse", "decode"}},
      {"farequal", &farequal_main, {"begin_key", "end_key", "delta"}},
      {"farextract",
       &farextract_main,
       {"filename_prefix", "filename_suffix", "generate_filenames", "keys",
        "key_separator", "range_delimiter"}},
      {"farinfo", &farinfo_main, {"begin_key", "end_key", "list_fsts"}},
      {"farisomorphic", &farisomorphic_main, {"begin_key", "end_key", "delta"}},
      {"farprintstrings",
       &farprintstrings_main,
       {"filename_prefix", "filename_suffix", "generate_filenames", "begin_key",
        "end_key", "print_key", "print_weight", "entry_type", "token_type",
        "symbols", "initial_symbols"}},
  };
}

}  // namespace

int main(int argc, char** argv) {
  CommandDispatcher dispatcher("far", "openfst/extensions/far/",
                               GetFarCommands());
  return dispatcher.Main(argc, argv);
}
