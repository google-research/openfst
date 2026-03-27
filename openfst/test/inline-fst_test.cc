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

#include "openfst/test/inline-fst.h"

#include <memory>
#include <string>

#include "gtest/gtest.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/compose.h"
#include "openfst/lib/equal.h"
#include "openfst/lib/float-weight.h"
#include "openfst/lib/fst-decl.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/lexicographic-weight.h"
#include "openfst/lib/vector-fst.h"

namespace fst {
namespace {

TEST(InlineFstTest, TestStdInline) {
  // Note that this example uses C++11 raw string literals.
  std::unique_ptr<const StdFst> f1(InlineFst<StdArc>(
      R"(0 1 2 3 2.0
1 2 3 4 1.0
2
)"));
  ASSERT_TRUE(f1 != nullptr);
  std::unique_ptr<const StdFst> f2(InlineFst<StdArc>(
      R"(0 1 3 1
1 2 4 2
2
)"));
  ASSERT_TRUE(f2 != nullptr);
  StdVectorFst f3;
  Compose(*f1, *f2, &f3);
  std::unique_ptr<const StdFst> f4(InlineFst<StdArc>(
      R"(0 1 2 1 2
1 2 3 2 1
2
)"));
  ASSERT_TRUE(f4 != nullptr);
  ASSERT_TRUE(Equal(f3, *f4));
}

TEST(InlineFstTest, TestLexicographicInline) {
  using Weight = LexicographicWeight<TropicalWeight, TropicalWeight>;
  using LexArc = ArcTpl<Weight>;
  std::unique_ptr<const Fst<LexArc>> f1(
      InlineFst<LexArc>("0 1 2 3 0.1,0.2\n"
                        "1 0.9,0.3\n"));
  ASSERT_TRUE(f1 != nullptr);
  ASSERT_EQ(f1->Final(0), Weight::Zero());
  ASSERT_EQ(f1->Final(1), Weight(TropicalWeight(0.9), TropicalWeight(0.3)));
}

TEST(InlineFstTest, TestBadFst) {
  EXPECT_DEATH(InlineFst<StdArc>("0 1 2 3 2.0\n"
                                 "1 2 3 hey wait a sec this isn't an fst\n"
                                 "2\n"),
               "Bad number of columns.");
}

}  // namespace
}  // namespace fst
