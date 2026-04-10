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

#include "openfst/extensions/linear/linear-fst.h"

#include <cstddef>
#include <iterator>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "gtest/gtest.h"
#include "openfst/extensions/linear/linear-fst-data-builder.h"
#include "openfst/lib/arc-map.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/cache.h"
#include "openfst/lib/compact-fst.h"
#include "openfst/lib/compose.h"
#include "openfst/lib/determinize.h"
#include "openfst/lib/equal.h"
#include "openfst/lib/equivalent.h"
#include "openfst/lib/float-weight.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/matcher.h"
#include "openfst/lib/minimize.h"
#include "openfst/lib/project.h"
#include "openfst/lib/rmepsilon.h"
#include "openfst/lib/symbol-table.h"
#include "openfst/lib/vector-fst.h"

using ::testing::Test;

namespace fst {
typedef StdArc::Label Label;
typedef StdArc::StateId StateId;
typedef StdArc::Weight Weight;
typedef LinearFstDataBuilder<StdArc> StdLinearFstDataBuilder;
typedef LinearTaggerFst<StdArc> StdLinearTaggerFst;
typedef LinearClassifierFstDataBuilder<StdArc>
    StdLinearClassifierFstDataBuilder;
typedef LinearClassifierFst<StdArc> StdLinearClassifierFst;

class LinearTaggerFstTest : public Test {
 protected:
  void SetUp() override {
    syms_.AddSymbol("<eps>");
    fst_.reset(BuildFst());
  }

  const StdLinearTaggerFst* BuildFst(CacheOptions opts = CacheOptions()) {
    StdLinearFstDataBuilder builder;
    AddDictionary(&builder);
    AddEmissionFeatures(&builder);
    AddEmissionFeaturesWithDelay(1, &builder);
    AddEmissionFeaturesWithDelay(2, &builder);
    AddTransitionFeatures(&builder);
    return new StdLinearTaggerFst(
        builder.Dump(), static_cast<const SymbolTable*>(&syms_),
        static_cast<const SymbolTable*>(&syms_), opts);
  }

  bool IsPossible(Label word, Label output) const {
    return (word + output) % 2;
  }

  void AddDictionary(StdLinearFstDataBuilder* builder) {
    num_combinations_ = 0;
    for (Label word = 1; word <= kNumWords; ++word) {
      std::vector<Label> features(1, word), possible;
      for (Label output = 1; output <= kNumOutput; ++output)
        if (IsPossible(word, output)) possible.push_back(output);
      builder->AddWord(word, features, possible);
      num_combinations_ += possible.size();
    }
  }

  Weight EmissionWeight(Label ilabel, Label olabel) const {
    if ((ilabel + olabel) % 2) return Weight(ilabel - olabel);
    return Weight::One();
  }

  Weight TransitionWeight(Label olabel1, Label olabel2) const {
    if (olabel2 < olabel1) return Weight(olabel2 - olabel1);
    return Weight::One();
  }

  bool InterestingWeight(Weight weight) const {
    return weight != Weight::One();
  }

  void AddEmissionFeaturesWithDelay(size_t delay,
                                    StdLinearFstDataBuilder* builder) {
    int emission_group = builder->AddGroup(delay);
    for (Label ilabel = 1; ilabel <= kNumWords; ++ilabel) {
      std::vector<Label> input(1 + delay, ilabel);
      for (Label olabel = 1; olabel <= kNumOutput; ++olabel) {
        Weight weight = EmissionWeight(ilabel, olabel);
        if (InterestingWeight(weight)) {
          std::vector<Label> output(1, olabel);
          builder->AddWeight(emission_group, input, output, weight);
        }
      }
    }
  }

  void AddEmissionFeatures(StdLinearFstDataBuilder* builder) {
    AddEmissionFeaturesWithDelay(0, builder);
  }

  void AddTransitionFeatures(StdLinearFstDataBuilder* builder) {
    int transition_group = builder->AddGroup(0);
    for (Label olabel1 = 1; olabel1 <= kNumOutput; ++olabel1) {
      for (Label olabel2 = 1; olabel2 <= kNumOutput; ++olabel2) {
        Weight weight = TransitionWeight(olabel1, olabel2);
        if (InterestingWeight(weight)) {
          std::vector<Label> input, output;
          output.push_back(olabel1);
          output.push_back(olabel2);
          builder->AddWeight(transition_group, input, output, weight);
        }
      }
    }
  }

  void CheckBasicProperties(const StdLinearTaggerFst& fst) {
    ASSERT_EQ(0, fst.Start());
    EXPECT_NE(Weight::Zero(), fst.Final(0));
    EXPECT_EQ(num_combinations_, fst.NumArcs(0));
  }

  static constexpr int kNumWords = 2;
  static constexpr int kNumOutput = 2;
  SymbolTable syms_;
  std::unique_ptr<const StdLinearTaggerFst> fst_;
  size_t num_combinations_;
};

TEST_F(LinearTaggerFstTest, BasicProperties) {
  CheckBasicProperties(*fst_);
  StdLinearTaggerFst copy1(*fst_, true);
  std::unique_ptr<StdLinearTaggerFst> copy2(fst_->Copy(true));
  CheckBasicProperties(copy1);
  CheckBasicProperties(*copy2);
  StdVectorFst expanded(*fst_);
  EXPECT_GT(expanded.NumStates(), 0);
}

TEST_F(LinearTaggerFstTest, SymbolTable) {
  ASSERT_TRUE(fst_->InputSymbols());
  EXPECT_EQ(syms_.Find("<eps>"), fst_->InputSymbols()->Find("<eps>"));
  ASSERT_TRUE(fst_->OutputSymbols());
  EXPECT_EQ(syms_.Find("<eps>"), fst_->OutputSymbols()->Find("<eps>"));
}

TEST_F(LinearTaggerFstTest, AcceptsAll) {
  StdVectorFst input_fst;
  {
    ProjectFst<StdArc> projected(*fst_, ProjectType::INPUT);
    ArcMapFst unweighted(projected, RmWeightMapper<StdArc>());
    RmEpsilonFst<StdArc> no_eps(unweighted);
    Determinize(no_eps, &input_fst);
    Minimize(&input_fst);
  }
  EXPECT_EQ(1, input_fst.NumStates());
  int covered = 0;
  for (ArcIterator<StdVectorFst> aiter(input_fst, input_fst.Start());
       !aiter.Done(); aiter.Next())
    ++covered;
  EXPECT_EQ(kNumWords, covered);
}

TEST_F(LinearTaggerFstTest, FlushOnlyOnce) {
  Label input[] = {1, 0, 1, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0};
  StdVectorFst expanded(*fst_);
  StdCompactStringFst input_fst =
      MakeStdCompactStringFst(std::begin(input), std::end(input));
  StdVectorFst composed;
  Compose(input_fst, expanded, &composed);
  for (StateIterator<StdVectorFst> siter(composed); !siter.Done();
       siter.Next()) {
    StateId state = siter.Value();
    if (composed.Final(state) == Weight::Zero()) {
      EXPECT_EQ(1, composed.NumArcs(state));
    } else {
      EXPECT_EQ(0, composed.NumArcs(state));
    }
  }
}

TEST_F(LinearTaggerFstTest, MatcherComposeTest) {
  Label input[] = {1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0};
  StdCompactStringFst input_fst =
      MakeStdCompactStringFst(std::begin(input), std::end(input));
  ComposeFst<StdArc> composed1(
      input_fst, *fst_,
      ComposeFstOptions<StdArc, Matcher<Fst<StdArc>>>(
          CacheOptions(),
          new Matcher<Fst<StdArc>>(input_fst.InitMatcher(MATCH_OUTPUT)),
          new Matcher<Fst<StdArc>>(fst_->InitMatcher(MATCH_INPUT))));
  StdVectorFst expanded(*fst_);
  ComposeFst<StdArc> composed2(input_fst, expanded);
  ProjectFst<StdArc> projected1(composed1, ProjectType::OUTPUT),
      projected2(composed2, ProjectType::OUTPUT);
  StdVectorFst acceptor1(projected1), acceptor2(projected2);
  RmEpsilon(&acceptor1);
  RmEpsilon(&acceptor2);
  EXPECT_TRUE(Equivalent(acceptor1, acceptor2));
}

TEST_F(LinearTaggerFstTest, IOTest) {
  std::ostringstream out;
  ASSERT_TRUE(fst_->Write(out, FstWriteOptions("ostringstream")));
  std::istringstream in(out.str());
  std::unique_ptr<const StdLinearTaggerFst> fst(
      StdLinearTaggerFst::Read(in, FstReadOptions("istringstream")));
  ASSERT_TRUE(fst);
  StdVectorFst vfst1(*fst_), vfst2(*fst);
  EXPECT_TRUE(Equal(vfst1, vfst2));
}

TEST_F(LinearTaggerFstTest, ShortInput) {
  StdVectorFst expanded(*fst_);
  Label input[] = {1};
  StdCompactStringFst input_fst =
      MakeStdCompactStringFst(std::begin(input), std::end(input));
  ComposeFst<StdArc> composed1(input_fst, expanded),
      composed2(
          input_fst, *fst_,
          ComposeFstOptions<StdArc, Matcher<Fst<StdArc>>>(
              CacheOptions(),
              new Matcher<Fst<StdArc>>(input_fst.InitMatcher(MATCH_OUTPUT)),
              new Matcher<Fst<StdArc>>(fst_->InitMatcher(MATCH_INPUT))));
  ProjectFst<StdArc> projected1(composed1, ProjectType::OUTPUT),
      projected2(composed2, ProjectType::OUTPUT);
  StdVectorFst acceptor1(projected1), acceptor2(projected2);
  RmEpsilon(&acceptor1);
  RmEpsilon(&acceptor2);
  EXPECT_TRUE(Equivalent(acceptor1, acceptor2));
}

class LinearClassifierFstTest : public Test {
 protected:
  void SetUp() override {
    syms_.AddSymbol("<eps>");

    // Build a multi-class classifier that assigns score proportional to
    // the number of unigram occurences.
    StdLinearClassifierFstDataBuilder builder(kNumClasses);
    std::vector<Label> feat;
    for (int i = 0; i < kNumGroups; ++i) builder.AddGroup();
    for (Label i = 1; i <= kNumClasses; ++i) {
      feat.clear();
      for (int j = 0; j < kNumGroups; ++j) feat.push_back(i + j * kNumClasses);
      builder.AddWord(i, feat);
      feat.resize(1);
      for (int j = 0; j < kNumGroups; ++j) {
        feat[0] = i + j * kNumClasses;
        builder.AddWeight(j, feat, i, Weight(1));
      }
    }
    fst_ = std::make_unique<const StdLinearClassifierFst>(
        builder.Dump(), kNumClasses, static_cast<const SymbolTable*>(&syms_),
        static_cast<const SymbolTable*>(&syms_));
  }

  static void EncodeAsAcceptor(StdVectorFst* fst) {
    for (StateIterator<StdVectorFst> siter(*fst); !siter.Done(); siter.Next()) {
      for (MutableArcIterator<StdVectorFst> aiter(fst, siter.Value());
           !aiter.Done(); aiter.Next()) {
        StdArc arc = aiter.Value();
        Label label = arc.ilabel * (kNumClasses + 1) + arc.olabel;
        arc.ilabel = arc.olabel = label;
        aiter.SetValue(arc);
      }
    }
  }

  SymbolTable syms_;
  std::unique_ptr<const StdLinearClassifierFst> fst_;
  static constexpr int kNumClasses = 3;
  static constexpr int kNumGroups = 2;
};

TEST_F(LinearClassifierFstTest, SymbolTable) {
  ASSERT_TRUE(fst_->InputSymbols());
  EXPECT_EQ(syms_.Find("<eps>"), fst_->InputSymbols()->Find("<eps>"));
  ASSERT_TRUE(fst_->OutputSymbols());
  EXPECT_EQ(syms_.Find("<eps>"), fst_->OutputSymbols()->Find("<eps>"));
}

TEST_F(LinearClassifierFstTest, CountingClassifier) {
  std::vector<Label> input;
  for (Label pred = 1; pred <= kNumClasses; ++pred)
    for (int i = 0; i < pred; ++i) input.push_back(pred);
  StdCompactStringFst input_fsa =
      MakeStdCompactStringFst(input.begin(), input.end());
  ComposeFst<StdArc> composed(input_fsa, *fst_);
  ProjectFst<StdArc> output_fsa(composed, ProjectType::OUTPUT);
  RmEpsilonFst<StdArc> no_eps_fsa(output_fsa);
  DeterminizeFst<StdArc> det_fsa(no_eps_fsa);
  StdVectorFst final_fsa(det_fsa);
  Minimize(&final_fsa);

  EXPECT_EQ(2, final_fsa.NumStates());

  for (ArcIterator<StdVectorFst> aiter(final_fsa, final_fsa.Start());
       !aiter.Done(); aiter.Next()) {
    const StdArc& arc = aiter.Value();
    StateId nextstate = arc.nextstate;
    EXPECT_EQ(Weight::One(), final_fsa.Final(nextstate));
    EXPECT_EQ(0, final_fsa.NumArcs(nextstate));
    EXPECT_EQ(Weight(arc.ilabel * kNumGroups), arc.weight);
  }
}

TEST_F(LinearClassifierFstTest, ExpandVsMatch) {
  StdVectorFst accept_anything;
  StateId start = accept_anything.AddState();
  accept_anything.SetStart(start);
  accept_anything.SetFinal(start, Weight::One());
  for (Label pred = 1; pred <= kNumClasses; ++pred)
    accept_anything.AddArc(start, StdArc(pred, pred, Weight::One(), start));

  StdVectorFst expand(*fst_), match(ComposeFst<StdArc>(accept_anything, *fst_));
  EncodeAsAcceptor(&expand);
  EncodeAsAcceptor(&match);
  EXPECT_TRUE(Equivalent(expand, match));
}

class LinearFstMatcherTplDeathTest : public LinearTaggerFstTest {};

TEST_F(LinearFstMatcherTplDeathTest, OnlyMatchInput) {
  EXPECT_DEATH(
      { LinearFstMatcherTpl<StdLinearTaggerFst> matcher(*fst_, MATCH_BOTH); },
      "LinearFstMatcherTpl: Bad match type");
  EXPECT_DEATH(
      {
        LinearFstMatcherTpl<StdLinearTaggerFst> matcher(*fst_, MATCH_OUTPUT);
        matcher.SetState(0);
      },
      "LinearFstMatcherTpl: Bad match type");
}

}  // namespace fst
