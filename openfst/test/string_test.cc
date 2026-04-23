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

#include "openfst/lib/string.h"

#include <cstdint>
#include <sstream>
#include <string>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/flags/flag.h"
#include "absl/log/check.h"
#include "absl/log/log.h"
#include "absl/strings/str_split.h"
#include "absl/strings/string_view.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/compact-fst.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/icu.h"
#include "openfst/lib/shortest-distance.h"
#include "openfst/lib/symbol-table.h"
#include "openfst/lib/vector-fst.h"
#include "openfst/script/compile-impl.h"

namespace fst {
namespace {

using ::testing::ElementsAre;

// Appends a UTF-8 encoded codepoint to the given string without bloated
// dependencies.
void AppendUTF8(char32_t codepoint, std::string& out) {
  if (codepoint <= 0x7F) {
    out.push_back(static_cast<char>(codepoint));
  } else if (codepoint <= 0x7FF) {
    out.push_back(static_cast<char>(0xC0 | (codepoint >> 6)));
    out.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
  } else if (codepoint <= 0xFFFF) {
    out.push_back(static_cast<char>(0xE0 | (codepoint >> 12)));
    out.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
    out.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
  } else {
    ASSERT_LE(codepoint, 0x10FFFF);
    out.push_back(static_cast<char>(0xF0 | (codepoint >> 18)));
    out.push_back(static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F)));
    out.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
    out.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
  }
}

TEST(StringTest, ConvertStringToLabelsSymbols) {
  using Label = StdArc::Label;
  SymbolTable syms("");
  syms.AddSymbol("<epsilon>");
  Label unk = syms.AddSymbol("<UNK>");
  syms.AddSymbol("A");
  syms.AddSymbol("B");
  syms.AddSymbol("C");
  syms.AddSymbol("R");

  const StdStringCompiler sc(TokenType::SYMBOL, &syms, unk);
  StdVectorFst fst;
  EXPECT_TRUE(sc("A B R A <epsilon> C A <epsilon> D A B R A", &fst,
                 StdArc::Weight(2.0F)));
  const StdStringPrinter sp(TokenType::SYMBOL, &syms);
  std::string out;
  EXPECT_TRUE(sp(fst, &out));
  std::vector<absl::string_view> labels =
      absl::StrSplit(out, ' ', absl::SkipEmpty());
  EXPECT_EQ(std::string(labels[0]), "A");
  EXPECT_EQ(std::string(labels[1]), "B");
  EXPECT_EQ(std::string(labels[2]), "R");
  EXPECT_EQ(std::string(labels[3]), "A");
  EXPECT_EQ(std::string(labels[4]), "C");
  EXPECT_EQ(std::string(labels[5]), "A");
  EXPECT_EQ(std::string(labels[6]), "<UNK>");
  EXPECT_EQ(std::string(labels[7]), "A");
  EXPECT_EQ(std::string(labels[8]), "B");
  EXPECT_EQ(std::string(labels[9]), "R");
  EXPECT_EQ(std::string(labels[10]), "A");
  EXPECT_EQ(labels.size(), 11);
  EXPECT_EQ(ShortestDistance(fst), StdArc::Weight(2.0F));
}

TEST(StringTest, ConvertStringToLabelsSymbolsPreservingEpsilons) {
  using Label = StdArc::Label;
  SymbolTable syms("");
  syms.AddSymbol("<epsilon>");
  Label unk = syms.AddSymbol("<UNK>");
  syms.AddSymbol("A");
  syms.AddSymbol("B");
  syms.AddSymbol("C");
  syms.AddSymbol("R");

  const StringCompiler<StdArc> sc(TokenType::SYMBOL, &syms, unk);
  VectorFst<StdArc> fst;
  EXPECT_TRUE(sc("A B R A <epsilon> C A <epsilon> D A B R A", &fst,
                 StdArc::Weight(2.0F)));
  const StringPrinter<StdArc> sp(TokenType::SYMBOL, &syms,
                                 /*omit_epsilon=*/false);
  std::string out;
  EXPECT_TRUE(sp(fst, &out));
  std::vector<absl::string_view> labels =
      absl::StrSplit(out, ' ', absl::SkipEmpty());
  EXPECT_EQ(std::string(labels[0]), "A");
  EXPECT_EQ(std::string(labels[1]), "B");
  EXPECT_EQ(std::string(labels[2]), "R");
  EXPECT_EQ(std::string(labels[3]), "A");
  EXPECT_EQ(std::string(labels[4]), "<epsilon>");
  EXPECT_EQ(std::string(labels[5]), "C");
  EXPECT_EQ(std::string(labels[6]), "A");
  EXPECT_EQ(std::string(labels[7]), "<epsilon>");
  EXPECT_EQ(std::string(labels[8]), "<UNK>");
  EXPECT_EQ(std::string(labels[9]), "A");
  EXPECT_EQ(std::string(labels[10]), "B");
  EXPECT_EQ(std::string(labels[11]), "R");
  EXPECT_EQ(std::string(labels[12]), "A");
  EXPECT_EQ(labels.size(), 13);
  EXPECT_EQ(ShortestDistance(fst), StdArc::Weight(2.0F));
}

// Checks that converting byte-strings to labels results in positive labels.
// To test with a signed char type, run the following:
// $ bazel test --copt=-fsigned-char --copt=-Wno-constant-conversion
// //openfst/test:string_test
TEST(StringTest, ConvertStringToLabelsBytes) {
  std::ostringstream ostr;
  for (int i = 1; i <= 255; ++i) ostr << static_cast<char>(i);
  const std::string& str = ostr.str();
  LOG(INFO) << "char is " << (str.back() < 0 ? "" : "un") << "signed";
  std::vector<int> labels;
  EXPECT_TRUE(internal::ConvertStringToLabels(str, fst::TokenType::BYTE,
                                              nullptr, fst::kNoLabel, &labels));
  for (int i = 1; i <= 255; ++i) {
    EXPECT_EQ(i, labels[i - 1]);
  }
}

TEST(StringTest, ByteLabelsRoundtrip) {
  std::ostringstream ostr;
  for (int i = 1; i <= 255; ++i) ostr << static_cast<char>(i);
  const StringCompiler<StdArc> sc(TokenType::BYTE);
  VectorFst<StdArc> fst;
  EXPECT_TRUE(sc(ostr.str(), &fst));
  const StringPrinter<StdArc> sp(TokenType::BYTE);
  std::string out;
  EXPECT_TRUE(sp(fst, &out));
  std::string ref = ostr.str();
  EXPECT_EQ(ref, out);
}

TEST(StringTest, ConvertStringToLabelsUnicode) {
  // TODO: Replace brute-force test with test of longest/shortest 1/2/3/4-byte
  // sequences, using "\U0010FFFF", etc.
  std::string utf8;
  int num_code_points = 0;
  // Use every third character from the first three planes (BMP, SMP, SIP),
  // skipping over surrogates.
  for (char32_t i = 3; i <= 0x2ffff; i += 3) {
    if (0xD800 <= i && i < 0xE000)  // skip over surrogates
      continue;
    AppendUTF8(i, utf8);
    ++num_code_points;
  }
  EXPECT_EQ(((3 << 16) - (0xE000 - 0xD800)) / 3 - 1, num_code_points);
  // Use every third character from the last three planes (SSP, SPUA-A, SPUA-B),
  // for an additional 2^16 characters.
  int num_high_code_points = 0;
  for (char32_t i = 0xe0000; i <= 0x10ffff; i += 3) {
    AppendUTF8(i, utf8);
    ++num_high_code_points;
  }
  EXPECT_EQ(1 << 16, num_high_code_points);
  num_code_points += num_high_code_points;
  const StringCompiler<StdArc> sc(TokenType::UTF8);
  VectorFst<StdArc> fst;
  EXPECT_TRUE(sc(utf8, &fst));
  EXPECT_EQ(num_code_points + 1, fst.NumStates());
  const StringPrinter<StdArc> sp(TokenType::UTF8);
  std::string out;
  EXPECT_TRUE(sp(fst, &out));
  EXPECT_EQ(utf8, out);
}

TEST(StringTest, ConvertStringToLabelsTruncatedUTF8) {
  const StringCompiler<StdArc> sc(TokenType::UTF8);
  VectorFst<StdArc> fst;
  // A truncated multibyte sequence.
  EXPECT_FALSE(sc("\xf5", &fst));
  // A continuation byte as the first character.
  EXPECT_FALSE(sc("\xbf", &fst));
}

TEST(StringTest, ConvertStringToLabelsEmptyString) {
  for (auto token_type :
       {TokenType::UTF8, TokenType::BYTE, TokenType::SYMBOL}) {
    const StringCompiler<StdArc> sc(token_type);
    VectorFst<StdArc> fst;
    EXPECT_TRUE(sc("", &fst)) << " fails for type `" << token_type << "`";
    EXPECT_EQ(1, fst.NumStates()) << " fails for type `" << token_type << "`";
    EXPECT_EQ(0, fst.Start()) << " fails for type `" << token_type << "`";
  }
}

TEST(StringTest, ConvertStringToLabelsWithNegativeLabels) {
  using StateId = StdArc::StateId;
  VectorFst<StdArc> fst;
  StateId start = fst.AddState();
  fst.SetStart(start);
  StateId end = fst.AddState();
  fst.AddArc(start, StdArc(-24, 0, StdArc::Weight::One(), end));
  const StringPrinter<StdArc> sp(TokenType::UTF8);
  std::string out;
  EXPECT_FALSE(sp(fst, &out));
}

TEST(StringTest, CompileFstAddSymbols) {
  SymbolTable isyms;
  isyms.AddSymbol("<epsilon>", 0);
  std::istringstream fst_file(
      "1 2 a\n"
      "2 3 b\n"
      "3 4 a\n"
      "4");
  FstCompiler<StdArc> fst(fst_file, "", &isyms, &isyms, nullptr,
                          /* acceptor */ true,
                          /* ikeep */ false, /* okeep */ false,
                          /* nkeep */ false, /* addsyms */ true);
  EXPECT_NE(-1, isyms.Find("a"));
  EXPECT_NE(-1, isyms.Find("b"));
  EXPECT_EQ(-1, isyms.Find("c"));
}

TEST(StringTest, CompileEmptyCompactFst) {
  const std::string input = "";
  const StringCompiler<StdArc> sc(TokenType::BYTE);
  StdCompactWeightedStringFst fst;
  EXPECT_TRUE(sc(input, &fst));
  const StringPrinter<StdArc> sp(TokenType::BYTE);
  std::string output;
  EXPECT_TRUE(sp(fst, &output));
  EXPECT_EQ(input, output);
}

TEST(StringTest, FinalStateWithOutgoingArcs) {
  const StringCompiler<StdArc> sc(TokenType::BYTE);
  StdVectorFst fst;
  EXPECT_TRUE(sc("ABRACADABRA", &fst));
  // Makes "ABRA" and "ABRACAD" possible strings.
  fst.SetFinal(4, StdVectorFst::Arc::Weight::One());
  fst.SetFinal(6, StdVectorFst::Arc::Weight::One());
  const StringPrinter<StdArc> sp(TokenType::BYTE);
  std::string out;
  EXPECT_FALSE(sp(fst, &out));
}

TEST(StringTest, ConvertStringToNumericSymbols) {
  const std::string in = "1 2 3";
  const StringCompiler<StdArc> sc(TokenType::SYMBOL);
  VectorFst<StdArc> fst;
  EXPECT_TRUE(sc(in, &fst));
  EXPECT_EQ(fst.NumStates(), 4);
  const StringPrinter<StdArc> sp(TokenType::SYMBOL);
  std::string out;
  EXPECT_TRUE(sp(fst, &out));
  EXPECT_EQ(out, in);
}

TEST(StringTest, ByteStringToLabels) {
  const std::string str = "abcd";
  std::vector<int> labels;
  EXPECT_TRUE(ByteStringToLabels(str, &labels));
  EXPECT_THAT(labels, ElementsAre('a', 'b', 'c', 'd'));
}

// A few tests to verify that for all TokenType options, LabelsToString
// conversion works correctly and epsilons are deleted by default.
TEST(StringTest, LabelsToByteString) {
  std::vector<char> labels{0, 'a', 'b', 0, 'c', 0, 'd', 0};
  std::string str;
  LabelsToString<char>(labels, &str, /*ttype=*/TokenType::BYTE);
  EXPECT_STREQ(str.data(), "abcd");
}

TEST(StringTest, LabelsToUTF8String) {
  std::vector<int32_t> labels{0, U'क', U'ख', 0, U'ग', 0, U'घ', 0};
  std::string str;
  LabelsToString<int32_t>(labels, &str, /*ttype=*/TokenType::UTF8);
  EXPECT_STREQ(str.data(), "कखगघ");
}

TEST(StringTest, LabelsToSymbolString) {
  SymbolTable syms;
  syms.AddSymbol("<epsilon>", 0);
  syms.AddSymbol("a", 1);
  syms.AddSymbol("b", 2);
  syms.AddSymbol("c", 3);
  syms.AddSymbol("d", 4);
  std::vector<int32_t> labels{0, 1, 2, 0, 3, 0, 4, 0};
  std::string str;
  LabelsToString<int32_t>(labels, &str,
                          /*ttype=*/TokenType::SYMBOL,
                          /*syms=*/&syms);
  EXPECT_STREQ(str.data(), "a b c d");
}

TEST(StringTest, LabelsToNumericString) {
  std::vector<int32_t> labels{0, 1, 2, 0, 3, 0, 4, 0};
  std::string str;
  LabelsToString<int32_t>(labels, &str,
                          /*ttype=*/TokenType::SYMBOL,
                          /*syms=*/nullptr);
  EXPECT_STREQ(str.data(), "1 2 3 4");
}

// Two tests to verify that for TokenType::SYMBOL mode, LabelsToString
// conversion works correctly and epsilons are kept.
TEST(StringTest, LabelsToSymbolStringWithEpsilons) {
  SymbolTable syms;
  syms.AddSymbol("<epsilon>", 0);
  syms.AddSymbol("a", 1);
  syms.AddSymbol("b", 2);
  syms.AddSymbol("c", 3);
  syms.AddSymbol("d", 4);
  std::vector<int32_t> labels{0, 1, 2, 0, 3, 0, 4, 0};
  std::string str;
  LabelsToString<int32_t>(labels, &str,
                          /*ttype=*/TokenType::SYMBOL,
                          /*syms=*/&syms,
                          absl::GetFlag(FLAGS_fst_field_separator),
                          /*omit_epsilon=*/false);
  EXPECT_STREQ(str.data(), "<epsilon> a b <epsilon> c <epsilon> d <epsilon>");
}

TEST(StringTest, LabelsToNumericStringWithEpsilons) {
  std::vector<int32_t> labels{0, 1, 2, 0, 3, 0, 4, 0};
  std::string str;
  LabelsToString<int32_t>(labels, &str,
                          /*ttype=*/TokenType::SYMBOL,
                          /*syms=*/nullptr,
                          absl::GetFlag(FLAGS_fst_field_separator),
                          /*omit_epsilon=*/false);
  EXPECT_STREQ(str.data(), "0 1 2 0 3 0 4 0");
}

TEST(StringTest, CustomSeparators) {
  SymbolTable syms;
  syms.AddSymbol("<epsilon>", 0);
  syms.AddSymbol("A", 1);
  syms.AddSymbol("B", 2);
  const std::string in = "AxByAxAxB";
  const StringCompiler<StdArc> sc(TokenType::SYMBOL, &syms);
  StdVectorFst fst;
  EXPECT_FALSE(sc(in, &fst));
  EXPECT_TRUE(sc(in, &fst, "xy"));
  EXPECT_EQ(fst.NumStates(), 6);
  std::string out;
  const StringPrinter<StdArc> sp(TokenType::SYMBOL, &syms);
  EXPECT_TRUE(sp(fst, &out));
  EXPECT_EQ(out, "A B A A B");
  EXPECT_TRUE(sp(fst, &out, "x"));
  EXPECT_EQ(out, "AxBxAxAxB");
}

}  // namespace
}  // namespace fst
