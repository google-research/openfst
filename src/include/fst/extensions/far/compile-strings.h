
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
// Authors: allauzen@google.com (Cyril Allauzen)
//          ttai@google.com (Terry Tai)
//          jpr@google.com (Jake Ratkiewicz)


#ifndef FST_EXTENSIONS_FAR_COMPILE_STRINGS_H_
#define FST_EXTENSIONS_FAR_COMPILE_STRINGS_H_

#include <libgen.h>
#include <string>
#include <vector>
using std::vector;

#include <fst/extensions/far/far.h>
#include <fst/string.h>

namespace fst {

// Construct a reader that provides FSTs from a file (stream) either on a
// line-by-line basis or on a per-stream basis.  Note that the freshly
// constructed reader is already set to the first input.
//
// Sample Usage:
//   for (StringReader<Arc> reader(...); !reader.Done(); reader.Next()) {
//     Fst *fst = reader.GetVectorFst();
//   }
template <class A>
class StringReader {
 public:
  typedef A Arc;
  typedef typename A::Label Label;
  typedef typename A::Weight Weight;
  typedef typename StringCompiler<A>::TokenType TokenType;

  enum EntryType { LINE = 1, FILE = 2 };

  StringReader(istream &istrm,
               const string &source,
               EntryType entry_type,
               TokenType token_type,
               bool allow_negative_labels,
               const SymbolTable *syms = 0)
      : nline_(0), strm_(istrm), source_(source), entry_type_(entry_type),
        token_type_(token_type), done_(false),
        compiler_(token_type, syms, allow_negative_labels) {
    Next();  // Initialize the reader to the first input.
  }

  bool Done() {
    return done_;
  }

  void Next() {
    VLOG(1) << "Processing source " << source_ << " at line " << nline_;
    if (!strm_) {                    // We're done if we have no more input.
      done_ = true;
      return;
    }
    if (entry_type_ == LINE) {
      getline(strm_, content_);
      ++nline_;
    } else {
      content_.clear();
      string line;
      while (getline(strm_, line)) {
        ++nline_;
        content_.append(line);
        content_.append("\n");
      }
    }
    if (!strm_ && content_.empty())  // We're also done if we read off all the
      done_ = true;                  // whitespace at the end of a file.
  }

  VectorFst<A> *GetVectorFst() {
    VectorFst<A> *fst = new VectorFst<A>;
    if (compiler_(content_, fst)) {
      return fst;
    } else {
      delete fst;
      return NULL;
    }
  }

  CompactFst<A, StringCompactor<A> > *GetCompactFst() {
    CompactFst<A, StringCompactor<A> > *fst =
        new CompactFst<A, StringCompactor<A> >;
    if (compiler_(content_, fst)) {
      return fst;
    } else {
      delete fst;
      return NULL;
    }
  }

 private:
  size_t nline_;
  istream &strm_;
  string source_;
  EntryType entry_type_;
  TokenType token_type_;
  bool done_;
  StringCompiler<A> compiler_;
  string content_;  // The actual content of the input stream's next FST.

  DISALLOW_COPY_AND_ASSIGN(StringReader);
};

// Compute the minimal length required to encode each line number as a decimal
// number.
int KeySize(const char *filename);

template <class Arc>
void FarCompileStrings(const vector<string> &in_fnames,
                       const string &out_fname,
                       const string &fst_type,
                       const FarType &far_type,
                       int32 generate_keys,
                       FarEntryType fet,
                       FarTokenType tt,
                       const string &symbols_fname,
                       bool allow_negative_labels,
                       bool file_list_input,
                       const string &key_prefix,
                       const string &key_suffix) {
  char keybuf[16];
  CHECK(generate_keys < sizeof(keybuf));

  typename StringReader<Arc>::EntryType entry_type;
  if (fet == FET_LINE)
    entry_type = StringReader<Arc>::LINE;
  else if (fet == FET_FILE)
    entry_type = StringReader<Arc>::FILE;
  else
    LOG(FATAL) << "FarCompileStrings: unknown entry type";

  typename StringCompiler<Arc>::TokenType token_type;
  if (tt == FTT_SYMBOL)
    token_type = StringCompiler<Arc>::SYMBOL;
  else if (tt == FTT_BYTE)
    token_type = StringCompiler<Arc>::BYTE;
  else if (tt == FTT_UTF8)
    token_type = StringCompiler<Arc>::UTF8;
  else
    LOG(FATAL) << "FarCompileStrings: unknown token type";

  bool compact;
  if (fst_type.empty() || (fst_type == "vector"))
    compact = false;
  else if (fst_type == "compact")
    compact = true;
  else
    LOG(FATAL) << "FarCompileStrings: unknown fst type: "
               << fst_type;

  const SymbolTable *syms = 0;
  if (!symbols_fname.empty()) {
    syms = SymbolTable::ReadText(symbols_fname,
                                 allow_negative_labels);
    if (!syms) exit(1);
  }

  FarWriter<Arc> *far_writer =
      FarWriter<Arc>::Create(out_fname, far_type);
  if (!far_writer) return;

  vector<string> inputs;
  if (file_list_input) {
    for (int i = 1; i < in_fnames.size(); ++i) {
      ifstream istrm(in_fnames[i].c_str());
      string str;
      while (getline(istrm, str))
        inputs.push_back(str);
    }
  } else {
    inputs = in_fnames;
  }

  for (int i = 0, n = 0; i < inputs.size(); ++i) {
    int key_size = generate_keys ? generate_keys :
        (entry_type == StringReader<Arc>::FILE ? 1 :
         KeySize(inputs[i].c_str()));
    CHECK(key_size < sizeof(keybuf));

    ifstream istrm(inputs[i].c_str());

    for (StringReader<Arc> reader(
             istrm, inputs[i], entry_type, token_type,
             allow_negative_labels, syms);
         !reader.Done();
         reader.Next()) {
      ++n;
      const Fst<Arc> *fst;
      if (compact)
        fst = reader.GetCompactFst();
      else
        fst = reader.GetVectorFst();
      if (!fst) {
        LOG(FATAL) << "FarCompileStrings: compiling string number " << n
                   << " in file " << inputs[i] << " failed with token_type = "
                   << (tt == FTT_BYTE ? "byte" :
                       (tt == FTT_UTF8 ? "utf8" :
                        (tt == FTT_SYMBOL ? "symbol" : "unknown")))
                   << " and entry_type = "
                   << (fet == FET_LINE ? "line" :
                       (fet == FET_FILE ? "file" : "unknown"));
      }
      sprintf(keybuf, "%0*d", key_size, n);
      string key;
      if (generate_keys > 0) {
        key = keybuf;
      } else {
        char* filename = new char[inputs[i].size() + 1];
        strcpy(filename, inputs[i].c_str());
        key = basename(filename);
        if (entry_type != StringReader<Arc>::FILE) {
          key += "-";
          key += keybuf;
        }
        delete[] filename;
      }
      far_writer->Add(key_prefix + key + key_suffix, *fst);
      delete fst;
    }
    if (generate_keys == 0)
      n = 0;
  }

  delete far_writer;
}

}  // namespace fst


#endif  // FST_EXTENSIONS_FAR_COMPILE_STRINGS_H_
