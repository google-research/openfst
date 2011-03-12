// printstrings-main.h

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
//
// Copyright 2005-2010 Google, Inc.
// Author: allauzen@google.com (Cyril Allauzen)
// Modified by: jpr@google.com (Jake Ratkiewicz)
//
// \file
// Output as strings the string FSTs in a finite-state archive.

#ifndef FST_EXTENSIONS_FAR_PRINT_STRINGS_H__
#define FST_EXTENSIONS_FAR_PRINT_STRINGS_H__

#include <string>
#include <vector>
using std::vector;

#include <fst/extensions/far/far.h>
#include <fst/string.h>

namespace fst {

template <class Arc>
void FarPrintStrings(
    const vector<string> &ifilenames, const FarEntryType entry_type,
    const FarTokenType far_token_type, const string &begin_key,
    const string &end_key, const bool print_key, const string &symbols_fname,
    const int32 generate_filenames, const string &filename_prefix,
    const string &filename_suffix) {

  typename StringPrinter<Arc>::TokenType token_type;
  if (far_token_type == FTT_SYMBOL)
    token_type = StringPrinter<Arc>::SYMBOL;
  else if (far_token_type == FTT_BYTE)
    token_type = StringPrinter<Arc>::BYTE;
  else if (far_token_type == FTT_UTF8)
    token_type = StringPrinter<Arc>::UTF8;
  else
    LOG(FATAL) << "FarPrintStrings:: unknown token type";

  const SymbolTable *syms = 0;
  if (!symbols_fname.empty()) {
    // allow negative flag?
    syms = SymbolTable::ReadText(symbols_fname, true);
    if (!syms) exit(1);
  }

  StringPrinter<Arc> string_printer(token_type, syms);

  FarReader<Arc> *far_reader = FarReader<Arc>::Open(ifilenames);
  if (!far_reader) return;

  if (!begin_key.empty())
    far_reader->Find(begin_key);

  string okey;
  int nrep = 0;
  for (int i = 1; !far_reader->Done(); far_reader->Next(), ++i) {
    string key = far_reader->GetKey();
    if (!end_key.empty() && end_key < key)
      break;
    if (okey == key)
      ++nrep;
    else
      nrep = 0;
    okey = key;

    const Fst<Arc> &fst = far_reader->GetFst();
    string str;
    VLOG(2) << "Handling key: " << key;
    string_printer(fst, &str);

    if (entry_type == FET_LINE) {
      if (print_key)
        cout << key << "\t";
      cout << str << endl;
    } else if (entry_type == FET_FILE) {
      stringstream sstrm;
      if (generate_filenames) {
        sstrm.fill('0');
        sstrm << std::right << setw(generate_filenames) << i;
      } else {
        sstrm << key;
        if (nrep > 0)
          sstrm << "." << nrep;
      }

      string filename;
      filename = filename_prefix +  sstrm.str() + filename_suffix;

      ofstream ostrm(filename.c_str());
      if (!ostrm)
        LOG(FATAL) << "FarPrintStrings: Can't open file:" << filename;
      ostrm << str;
      if (token_type == StringPrinter<Arc>::SYMBOL)
        ostrm << "\n";
    }
  }
}



}  // namespace fst

#endif  // FST_EXTENSIONS_FAR_PRINT_STRINGS_H__
