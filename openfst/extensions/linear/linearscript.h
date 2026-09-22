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

#ifndef OPENFST_EXTENSIONS_LINEAR_LINEARSCRIPT_H_
#define OPENFST_EXTENSIONS_LINEAR_LINEARSCRIPT_H_

#include <cstddef>
#include <istream>
#include <string>
#include <tuple>
#include <vector>

#include "absl/flags/declare.h"
#include "absl/flags/flag.h"
#include "absl/log/check.h"
#include "absl/log/log.h"
#include "absl/strings/ascii.h"
#include "absl/strings/numbers.h"
#include "absl/strings/str_split.h"
#include "absl/strings/string_view.h"
#include "openfst/extensions/linear/linear-fst-data-builder.h"
#include "openfst/extensions/linear/linear-fst-data.h"
#include "openfst/extensions/linear/linear-fst.h"
#include "openfst/lib/file-util.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/symbol-table.h"
#include "openfst/lib/util.h"

ABSL_DECLARE_FLAG(std::string, delimiter);
ABSL_DECLARE_FLAG(std::string, empty_symbol);
ABSL_DECLARE_FLAG(std::string, start_symbol);
ABSL_DECLARE_FLAG(std::string, end_symbol);
ABSL_DECLARE_FLAG(bool, classifier);

namespace fst {
namespace script {

using LinearCompileArgs =
    std::tuple<const std::string&, const std::string&, const std::string&,
               char**, int, const std::string&, const std::string&,
               const std::string&, const std::string&>;

bool ValidateDelimiter();

bool ValidateEmptySymbol();

// Returns the proper label given the symbol. For symbols other than
// `FLAGS_start_symbol` or `FLAGS_end_symbol`, looks up the symbol
// table to decide the label. Depending on whether
// `FLAGS_start_symbol` and `FLAGS_end_symbol` are identical, it
// either returns `kNoLabel` for later processing or decides the label
// right away.
template <class Arc>
inline typename Arc::Label LookUp(absl::string_view str, SymbolTable* syms) {
  if (str == absl::GetFlag(FLAGS_start_symbol)) {
    return str == absl::GetFlag(FLAGS_end_symbol)
               ? kNoLabel
               : LinearFstData<Arc>::kStartOfSentence;
  } else if (str == absl::GetFlag(FLAGS_end_symbol)) {
    return LinearFstData<Arc>::kEndOfSentence;
  } else {
    return syms->AddSymbol(str);
  }
}

// Splits `str` with `delim` as the delimiter and stores the labels in
// `output`.
template <class Arc>
void SplitAndPush(absl::string_view str, char delim, SymbolTable* syms,
                  std::vector<typename Arc::Label>* output) {
  if (str.empty() || str == absl::GetFlag(FLAGS_empty_symbol)) return;
  for (absl::string_view buf : absl::StrSplit(str, delim)) {
    output->push_back(LookUp<Arc>(buf, syms));
  }
}

// Like `std::replace_copy` but returns the number of modifications
template <class InputIterator, class OutputIterator, class T>
size_t ReplaceCopy(InputIterator first, InputIterator last,
                   OutputIterator result, const T& old_value,
                   const T& new_value) {
  size_t changes = 0;
  while (first != last) {
    if (*first == old_value) {
      *result = new_value;
      ++changes;
    } else {
      *result = *first;
    }
    ++first;
    ++result;
  }
  return changes;
}

template <class Arc>
bool GetVocabRecord(const std::string& vocab, std::istream& strm,
                    SymbolTable* isyms, SymbolTable* fsyms, SymbolTable* osyms,
                    typename Arc::Label* word,
                    std::vector<typename Arc::Label>* feature_labels,
                    std::vector<typename Arc::Label>* possible_labels,
                    size_t* num_line);

template <class Arc>
bool GetModelRecord(const std::string& model, std::istream& strm,
                    SymbolTable* fsyms, SymbolTable* osyms,
                    std::vector<typename Arc::Label>* input_labels,
                    std::vector<typename Arc::Label>* output_labels,
                    typename Arc::Weight* weight, size_t* num_line);

// Reads in vocabulary file. Each line is in the following format
//
//   word <whitespace> features [ <whitespace> possible output ]
//
// where features and possible output are `FLAGS_delimiter`-delimited lists of
// tokens
template <class Arc>
void AddVocab(const std::string& vocab, SymbolTable* isyms, SymbolTable* fsyms,
              SymbolTable* osyms, LinearFstDataBuilder<Arc>* builder) {
  file::FileInStream in(vocab);
  if (!in) LOG(FATAL) << "Can't open file: " << vocab;  // Crash OK.
  size_t num_line = 0, num_added = 0;
  std::vector<std::string> fields;
  std::vector<typename Arc::Label> feature_labels, possible_labels;
  typename Arc::Label word;
  while (GetVocabRecord<Arc>(vocab, in, isyms, fsyms, osyms, &word,
                             &feature_labels, &possible_labels, &num_line)) {
    if (word == kNoLabel) {
      LOG(WARNING) << "Ignored: boundary word: " << fields[0];
      continue;
    }
    if (possible_labels.empty()) {
      num_added += builder->AddWord(word, feature_labels);
    } else {
      num_added += builder->AddWord(word, feature_labels, possible_labels);
    }
  }
  VLOG(1) << "Read " << num_added << " words in " << num_line << " lines from "
          << vocab;
}

template <class Arc>
void AddVocab(const std::string& vocab, SymbolTable* isyms, SymbolTable* fsyms,
              SymbolTable* osyms,
              LinearClassifierFstDataBuilder<Arc>* builder) {
  file::FileInStream in(vocab);
  if (!in) LOG(FATAL) << "Can't open file: " << vocab;  // Crash OK.
  size_t num_line = 0, num_added = 0;
  std::vector<std::string> fields;
  std::vector<typename Arc::Label> feature_labels, possible_labels;
  typename Arc::Label word;
  while (GetVocabRecord<Arc>(vocab, in, isyms, fsyms, osyms, &word,
                             &feature_labels, &possible_labels, &num_line)) {
    if (!possible_labels.empty()) {
      LOG(FATAL)  // Crash OK.
          << "Classifier vocabulary should not have possible output constraint";
    }
    if (word == kNoLabel) {
      LOG(WARNING) << "Ignored: boundary word: " << fields[0];
      continue;
    }
    num_added += builder->AddWord(word, feature_labels);
  }
  VLOG(1) << "Read " << num_added << " words in " << num_line << " lines from "
          << vocab;
}

// Reads in model file. The first line is an integer designating the
// size of future window in the input sequences. After this, each line
// is in the following format
//
//   input sequence <whitespace> output sequence <whitespace> weight
//
// input sequence is a `FLAGS_delimiter`-delimited sequence of feature
// labels (see `AddVocab()`) . output sequence is a
// `FLAGS_delimiter`-delimited sequence of output labels where the
// last label is the output of the feature position before the history
// boundary.
template <class Arc>
void AddModel(const std::string& model, SymbolTable* fsyms, SymbolTable* osyms,
              LinearFstDataBuilder<Arc>* builder) {
  file::FileInStream in(model);
  if (!in) LOG(FATAL) << "Can't open file: " << model;  // Crash OK.
  std::string line;
  std::getline(in, line);
  if (!in) LOG(FATAL) << "Empty file: " << model;  // Crash OK.
  size_t future_size;
  if (!absl::SimpleAtoi(absl::StripAsciiWhitespace(line), &future_size)) {
    LOG(FATAL) << "Can't read future size: " << model;  // Crash OK.
  }
  size_t num_line = 1, num_added = 0;
  const int group = builder->AddGroup(future_size);
  VLOG(1) << "Group " << group << ": from " << model << "; future size is "
          << future_size << ".";
  // Add the rest of lines as a single feature group
  std::vector<typename Arc::Label> input_labels, output_labels;
  typename Arc::Weight weight;
  while (GetModelRecord<Arc>(model, in, fsyms, osyms, &input_labels,
                             &output_labels, &weight, &num_line)) {
    if (output_labels.empty()) {
      LOG(FATAL)  // Crash OK.
          << "Empty output sequence in source " << model << ", line "
          << num_line;
    }

    const typename Arc::Label marks[] = {LinearFstData<Arc>::kStartOfSentence,
                                         LinearFstData<Arc>::kEndOfSentence};

    std::vector<typename Arc::Label> copy_input(input_labels.size()),
        copy_output(output_labels.size());
    for (int i = 0; i < 2; ++i) {
      for (int j = 0; j < 2; ++j) {
        size_t num_input_changes =
            ReplaceCopy(input_labels.begin(), input_labels.end(),
                        copy_input.begin(), kNoLabel, marks[i]);
        size_t num_output_changes =
            ReplaceCopy(output_labels.begin(), output_labels.end(),
                        copy_output.begin(), kNoLabel, marks[j]);
        if ((num_input_changes > 0 || i == 0) &&
            (num_output_changes > 0 || j == 0)) {
          num_added +=
              builder->AddWeight(group, copy_input, copy_output, weight);
        }
      }
    }
  }
  VLOG(1) << "Group " << group << ": read " << num_added << " weight(s) in "
          << num_line << " lines.";
}

template <class Arc>
void AddModel(const std::string& model, SymbolTable* fsyms, SymbolTable* osyms,
              LinearClassifierFstDataBuilder<Arc>* builder) {
  file::FileInStream in(model);
  if (!in) LOG(FATAL) << "Can't open file: " << model;  // Crash OK.
  std::string line;
  std::getline(in, line);
  if (!in) LOG(FATAL) << "Empty file: " << model;  // Crash OK.
  size_t future_size;
  if (!absl::SimpleAtoi(absl::StripAsciiWhitespace(line), &future_size)) {
    LOG(FATAL) << "Can't read future size: " << model;  // Crash OK.
  }
  if (future_size != 0) {
    LOG(FATAL)  // Crash OK.
        << "Classifier model must have future size = 0; got " << future_size
        << " from " << model;
  }
  size_t num_line = 1, num_added = 0;
  const int group = builder->AddGroup();
  VLOG(1) << "Group " << group << ": from " << model << "; future size is "
          << future_size << ".";
  // Add the rest of lines as a single feature group
  std::vector<typename Arc::Label> input_labels, output_labels;
  typename Arc::Weight weight;
  while (GetModelRecord<Arc>(model, in, fsyms, osyms, &input_labels,
                             &output_labels, &weight, &num_line)) {
    if (output_labels.size() != 1) {
      LOG(FATAL)  // Crash OK.
          << "Output not a single label in source " << model << ", line "
          << num_line;
    }

    const typename Arc::Label marks[] = {LinearFstData<Arc>::kStartOfSentence,
                                         LinearFstData<Arc>::kEndOfSentence};

    typename Arc::Label pred = output_labels[0];

    std::vector<typename Arc::Label> copy_input(input_labels.size());
    for (int i = 0; i < 2; ++i) {
      size_t num_input_changes =
          ReplaceCopy(input_labels.begin(), input_labels.end(),
                      copy_input.begin(), kNoLabel, marks[i]);
      if (num_input_changes > 0 || i == 0) {
        num_added += builder->AddWeight(group, copy_input, pred, weight);
      }
    }
  }
  VLOG(1) << "Group " << group << ": read " << num_added << " weight(s) in "
          << num_line << " lines.";
}

void SplitByWhitespace(absl::string_view str, std::vector<std::string>* out);
int ScanNumClasses(char** models, int models_length);

template <class Arc>
void LinearCompileTpl(LinearCompileArgs* args) {
  const std::string& epsilon_symbol = std::get<0>(*args);
  const std::string& unknown_symbol = std::get<1>(*args);
  const std::string& vocab = std::get<2>(*args);
  char** models = std::get<3>(*args);
  const int models_length = std::get<4>(*args);
  const std::string& out = std::get<5>(*args);
  const std::string& save_isymbols = std::get<6>(*args);
  const std::string& save_fsymbols = std::get<7>(*args);
  const std::string& save_osymbols = std::get<8>(*args);

  SymbolTable isyms,  // input (e.g. word tokens)
      osyms,          // output (e.g. tags)
      fsyms;          // feature (e.g. word identity, suffix, etc.)
  isyms.AddSymbol(epsilon_symbol);
  osyms.AddSymbol(epsilon_symbol);
  fsyms.AddSymbol(epsilon_symbol);
  isyms.AddSymbol(unknown_symbol);

  VLOG(1) << "start-of-sentence label is "
          << LinearFstData<Arc>::kStartOfSentence;
  VLOG(1) << "end-of-sentence label is " << LinearFstData<Arc>::kEndOfSentence;

  if (absl::GetFlag(FLAGS_classifier)) {
    int num_classes = ScanNumClasses(models, models_length);
    LinearClassifierFstDataBuilder<Arc> builder(num_classes, &isyms, &fsyms,
                                                &osyms);

    AddVocab(vocab, &isyms, &fsyms, &osyms, &builder);
    for (int i = 0; i < models_length; ++i) {
      AddModel(models[i], &fsyms, &osyms, &builder);
    }
    LinearClassifierFst<Arc> fst(builder.Dump(), num_classes, &isyms, &osyms);
    fst.Write(out);
  } else {
    LinearFstDataBuilder<Arc> builder(&isyms, &fsyms, &osyms);
    AddVocab(vocab, &isyms, &fsyms, &osyms, &builder);
    for (int i = 0; i < models_length; ++i) {
      AddModel(models[i], &fsyms, &osyms, &builder);
    }
    LinearTaggerFst<Arc> fst(builder.Dump(), &isyms, &osyms);
    fst.Write(out);
  }

  if (!save_isymbols.empty()) isyms.WriteText(save_isymbols);
  if (!save_fsymbols.empty()) fsyms.WriteText(save_fsymbols);
  if (!save_osymbols.empty()) osyms.WriteText(save_osymbols);
}

void LinearCompile(const std::string& arc_type,
                   const std::string& epsilon_symbol,
                   const std::string& unknown_symbol, const std::string& vocab,
                   char** models, int models_len, const std::string& out,
                   const std::string& save_isymbols,
                   const std::string& save_fsymbols,
                   const std::string& save_osymbols);

template <class Arc>
bool GetVocabRecord(const std::string& vocab, std::istream& strm,
                    SymbolTable* isyms, SymbolTable* fsyms, SymbolTable* osyms,
                    typename Arc::Label* word,
                    std::vector<typename Arc::Label>* feature_labels,
                    std::vector<typename Arc::Label>* possible_labels,
                    size_t* num_line) {
  std::string line;
  if (!std::getline(strm, line)) return false;
  ++(*num_line);

  std::vector<std::string> fields;
  SplitByWhitespace(line, &fields);
  if (fields.size() != 3) {
    LOG(FATAL)  // Crash OK.
        << "Wrong number of fields in source " << vocab << ", line "
        << num_line;
  }

  feature_labels->clear();
  possible_labels->clear();

  *word = LookUp<Arc>(fields[0], isyms);

  const char delim = absl::GetFlag(FLAGS_delimiter)[0];
  SplitAndPush<Arc>(fields[1], delim, fsyms, feature_labels);
  SplitAndPush<Arc>(fields[2], delim, osyms, possible_labels);

  return true;
}

template <class Arc>
bool GetModelRecord(const std::string& model, std::istream& strm,
                    SymbolTable* fsyms, SymbolTable* osyms,
                    std::vector<typename Arc::Label>* input_labels,
                    std::vector<typename Arc::Label>* output_labels,
                    typename Arc::Weight* weight, size_t* num_line) {
  std::string line;
  if (!std::getline(strm, line)) return false;
  ++(*num_line);

  std::vector<std::string> fields;
  SplitByWhitespace(line, &fields);
  if (fields.size() != 3) {
    LOG(FATAL)  // Crash OK.
        << "Wrong number of fields in source " << model << ", line "
        << num_line;
  }

  input_labels->clear();
  output_labels->clear();

  const char delim = absl::GetFlag(FLAGS_delimiter)[0];
  SplitAndPush<Arc>(fields[0], delim, fsyms, input_labels);
  SplitAndPush<Arc>(fields[1], delim, osyms, output_labels);

  *weight = StrToWeight<typename Arc::Weight>(fields[2]);

  GuessStartOrEnd<Arc>(input_labels, kNoLabel);
  GuessStartOrEnd<Arc>(output_labels, kNoLabel);

  return true;
}

}  // namespace script
}  // namespace fst

#endif  // OPENFST_EXTENSIONS_LINEAR_LINEARSCRIPT_H_
