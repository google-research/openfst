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

#include "openfst/extensions/ngram/compact-louds-tree-fst.h"

#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <memory>
#include <string>
#include <vector>

#include "openfst/compat/file_path.h"
#include "gtest/gtest.h"
#include "absl/flags/flag.h"
#include "absl/log/check.h"
#include "absl/random/random.h"
#include "benchmark/benchmark.h"
#include "openfst/extensions/ngram/ngram-fst.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/cache.h"
#include "openfst/lib/compose.h"
#include "openfst/lib/expanded-fst.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/isomorphic.h"
#include "openfst/lib/mutable-fst.h"
#include "openfst/lib/properties.h"
#include "openfst/lib/shortest-distance.h"
#include "openfst/lib/test-properties.h"
#include "openfst/lib/vector-fst.h"

namespace fst {
namespace {

std::string Testfile() {
  return JoinPath(
      std::string("."),
      "openfst/extensions/ngram/testdata/earnest.mod");
}

std::vector<std::vector<StdArc::Label>> GetWords(size_t n, size_t sigma) {
  absl::BitGen bitgen;
  std::vector<std::vector<StdArc::Label>> output;
  for (int i = 0; i < n; i++) {
    std::vector<StdArc::Label> tempvector;
    for (int j = 0; j < absl::Uniform(bitgen, 0, RAND_MAX) % 12 + 1; j++) {
      tempvector.push_back(absl::Uniform(bitgen, 0, RAND_MAX) % sigma + 1);
    }
    output.push_back(tempvector);
  }
  std::sort(output.begin(), output.end());
  return output;
}

StdVectorFst BuildTrie(std::vector<std::vector<StdArc::Label>> words) {
  StdVectorFst fst;
  fst.AddState();
  fst.SetStart(0);
  StdArc::StateId statecount = 1;
  bool found = false;
  for (const auto& word : words) {
    StdArc::StateId cur = 0;
    for (auto c : word) {
      found = false;
      for (ArcIterator<StdVectorFst> aiter(fst, cur); !aiter.Done() && !found;
           aiter.Next()) {
        if (aiter.Value().ilabel == c) {
          cur = aiter.Value().nextstate;
          found = true;
        }
      }
      // no matching transition found so we make a new state/transition
      if (!found) {
        fst.AddState();
        fst.AddArc(cur, StdArc(c, c, StdArc::Weight::One(), statecount));
        cur = statecount;
        statecount++;
      }
    }
    fst.SetFinal(cur, StdArc::Weight::One());
  }
  return fst;
}

// Tests

TEST(CompactLoudsTreeFstTest, HasLoudsTreeStructureTestMultiedge) {
  StdVectorFst vecfst;
  vecfst.AddState();
  vecfst.AddState();
  vecfst.SetStart(0);
  vecfst.AddArc(0, StdArc(1, 1, StdArc::Weight::One(), 1));
  vecfst.AddArc(0, StdArc(2, 2, StdArc::Weight::One(), 1));
  vecfst.SetFinal(0, StdArc::Weight::One());
  vecfst.SetFinal(1, StdArc::Weight::One());

  EXPECT_FALSE(HasLoudsTreeStructure(vecfst));
}

TEST(CompactLoudsTreeFstTest, HasLoudsTreeStructureTestCycle) {
  StdVectorFst vecfst;
  vecfst.AddState();
  vecfst.AddState();
  vecfst.AddState();
  vecfst.SetStart(0);
  vecfst.AddArc(0, StdArc(1, 1, StdArc::Weight::One(), 1));
  vecfst.AddArc(1, StdArc(2, 2, StdArc::Weight::One(), 2));
  vecfst.AddArc(2, StdArc(2, 2, StdArc::Weight::One(), 0));
  vecfst.SetFinal(0, StdArc::Weight::One());
  vecfst.SetFinal(1, StdArc::Weight::One());
  vecfst.SetFinal(2, StdArc::Weight::One());

  EXPECT_FALSE(HasLoudsTreeStructure(vecfst));
}

TEST(CompactLoudsTreeFstTest, HasLoudsTreeStructureTestSink) {
  StdVectorFst vecfst;
  vecfst.AddState();
  vecfst.AddState();
  vecfst.AddState();
  vecfst.SetStart(0);
  vecfst.AddArc(0, StdArc(1, 1, StdArc::Weight::One(), 1));
  vecfst.AddArc(2, StdArc(2, 2, StdArc::Weight::One(), 1));
  vecfst.SetFinal(0, StdArc::Weight::One());
  vecfst.SetFinal(1, StdArc::Weight::One());
  vecfst.SetFinal(2, StdArc::Weight::One());

  EXPECT_FALSE(HasLoudsTreeStructure(vecfst));
}

TEST(CompactLoudsTreeFstTest, IsCompatibleWeighted) {
  StdVectorFst vecfst;
  vecfst.AddState();
  vecfst.AddState();
  vecfst.AddState();
  vecfst.SetStart(0);
  vecfst.AddArc(0, StdArc(1, 1, StdArc::Weight(.2f), 1));
  vecfst.AddArc(1, StdArc(2, 2, StdArc::Weight(.3f), 2));
  vecfst.SetFinal(0, StdArc::Weight(.1f));
  vecfst.SetFinal(1, StdArc::Weight::One());
  vecfst.SetFinal(2, StdArc::Weight::One());

  bool defaultcompactor =
      DefaultLoudsTreeElementCompactor<StdArc>().IsCompatible(vecfst);
  bool unweightedcompactor =
      UnweightedLoudsTreeElementCompactor<StdArc>().IsCompatible(vecfst);
  bool acceptorcompactor =
      AcceptorLoudsTreeElementCompactor<StdArc>().IsCompatible(vecfst);
  bool unweightedacceptorcompactor =
      UnweightedAcceptorLoudsTreeElementCompactor<StdArc>().IsCompatible(
          vecfst);

  EXPECT_TRUE(defaultcompactor);
  EXPECT_FALSE(unweightedcompactor);
  EXPECT_TRUE(acceptorcompactor);
  EXPECT_FALSE(unweightedacceptorcompactor);
}

TEST(CompactLoudsTreeFstTest, IsCompatibleTransducer) {
  StdVectorFst vecfst;
  vecfst.AddState();
  vecfst.AddState();
  vecfst.AddState();
  vecfst.SetStart(0);
  vecfst.AddArc(0, StdArc(1, 3, StdArc::Weight::One(), 1));
  vecfst.AddArc(1, StdArc(2, 4, StdArc::Weight::One(), 2));
  vecfst.SetFinal(0, StdArc::Weight::One());
  vecfst.SetFinal(1, StdArc::Weight::One());
  vecfst.SetFinal(2, StdArc::Weight::One());

  bool defaultcompactor =
      DefaultLoudsTreeElementCompactor<StdArc>().IsCompatible(vecfst);
  bool unweightedcompactor =
      UnweightedLoudsTreeElementCompactor<StdArc>().IsCompatible(vecfst);
  bool acceptorcompactor =
      AcceptorLoudsTreeElementCompactor<StdArc>().IsCompatible(vecfst);
  bool unweightedacceptorcompactor =
      UnweightedAcceptorLoudsTreeElementCompactor<StdArc>().IsCompatible(
          vecfst);

  EXPECT_TRUE(defaultcompactor);
  EXPECT_TRUE(unweightedcompactor);
  EXPECT_FALSE(acceptorcompactor);
  EXPECT_FALSE(unweightedacceptorcompactor);
}

TEST(CompactLoudsTreeFstTest, IsCompatibleWeightedTransducer) {
  StdVectorFst vecfst;
  vecfst.AddState();
  vecfst.AddState();
  vecfst.AddState();
  vecfst.SetStart(0);
  vecfst.AddArc(0, StdArc(1, 3, StdArc::Weight(.1f), 1));
  vecfst.AddArc(1, StdArc(2, 4, StdArc::Weight(.3f), 2));
  vecfst.SetFinal(0, StdArc::Weight(.4f));
  vecfst.SetFinal(1, StdArc::Weight::One());
  vecfst.SetFinal(2, StdArc::Weight::One());

  bool defaultcompactor =
      DefaultLoudsTreeElementCompactor<StdArc>().IsCompatible(vecfst);
  bool unweightedcompactor =
      UnweightedLoudsTreeElementCompactor<StdArc>().IsCompatible(vecfst);
  bool acceptorcompactor =
      AcceptorLoudsTreeElementCompactor<StdArc>().IsCompatible(vecfst);
  bool unweightedacceptorcompactor =
      UnweightedAcceptorLoudsTreeElementCompactor<StdArc>().IsCompatible(
          vecfst);

  EXPECT_TRUE(defaultcompactor);
  EXPECT_FALSE(unweightedcompactor);
  EXPECT_FALSE(acceptorcompactor);
  EXPECT_FALSE(unweightedacceptorcompactor);
}

TEST(CompactLoudsTreeFstTest, InterConversionTest) {
  StdVectorFst vecfst(BuildTrie(GetWords(100, 10)));

  StdCompactLoudsTreeFst def(vecfst);
  StdUnweightedCompactLoudsTreeFst unweighted(def);
  StdAcceptorLoudsTreeFst acceptor(unweighted);
  StdUnweightedAcceptorCompactLoudsTreeFst unweightedacceptor(acceptor);
  StdCompactLoudsTreeFst def2(unweightedacceptor);

  EXPECT_TRUE(Isomorphic(def, unweighted));
  EXPECT_TRUE(Isomorphic(unweighted, acceptor));
  EXPECT_TRUE(Isomorphic(acceptor, unweightedacceptor));
  EXPECT_TRUE(Isomorphic(unweightedacceptor, def2));
  EXPECT_TRUE(Isomorphic(def2, vecfst));
}

// TEMPLATED TESTS

template <typename T>
class CompactorTest : public ::testing::Test {
 public:
  CompactorTest() : vecfst(BuildTrie(GetWords(100, 5))), ltfst(vecfst) {}
  StdVectorFst vecfst;
  T ltfst;

  void SetUp() override {
    // Force Fst::Properties to check cached vs computed properties.
    absl::SetFlag(&FLAGS_fst_verify_properties, true);
  }
};

using FstType =
    ::testing::Types<StdCompactLoudsTreeFst, StdUnweightedCompactLoudsTreeFst,
                     StdAcceptorLoudsTreeFst,
                     StdUnweightedAcceptorCompactLoudsTreeFst>;
TYPED_TEST_SUITE(CompactorTest, FstType);

TYPED_TEST(CompactorTest, BasicCheck) {
  fst::StdVectorFst vecfst;
  vecfst.AddState();
  vecfst.AddState();
  vecfst.AddState();
  vecfst.SetStart(0);
  vecfst.AddArc(0, StdArc(1, 1, StdArc::Weight::One(), 1));
  vecfst.AddArc(0, StdArc(2, 2, StdArc::Weight::One(), 2));

  vecfst.SetFinal(1, StdArc::Weight::One());
  vecfst.SetFinal(2, StdArc::Weight::One());

  TypeParam ltfst(vecfst);
  EXPECT_EQ(vecfst.NumStates(), ltfst.NumStates());

  EXPECT_EQ(3, ltfst.NumStates());
  EXPECT_EQ(2, ltfst.NumArcs(0));
  EXPECT_EQ(0, ltfst.NumArcs(1));
  EXPECT_EQ(0, ltfst.NumArcs(2));

  EXPECT_EQ(0, ltfst.Start());

  for (StateIterator<TypeParam> siter(ltfst); !siter.Done(); siter.Next()) {
    EXPECT_EQ(vecfst.NumArcs(siter.Value()), ltfst.NumArcs(siter.Value()));
    EXPECT_EQ(vecfst.Final(siter.Value()), ltfst.Final(siter.Value()));
  }

  StateIterator<Fst<StdArc>> siter1(vecfst);
  StateIterator<TypeParam> siter2(ltfst);
  while (!siter1.Done() || !siter2.Done()) {
    EXPECT_EQ(siter1.Done(), siter2.Done());
    EXPECT_EQ(siter1.Value(), siter2.Value());
    EXPECT_EQ(vecfst.NumArcs(siter1.Value()), ltfst.NumArcs(siter2.Value()));
    EXPECT_EQ(vecfst.Final(siter1.Value()), ltfst.Final(siter2.Value()));

    ArcIterator<StdVectorFst> aiter1(vecfst, siter1.Value());
    ArcIterator<TypeParam> aiter2(ltfst, siter2.Value());

    while (!aiter1.Done() || !aiter2.Done()) {
      EXPECT_EQ(aiter1.Done(), aiter2.Done());
      StdArc arc1, arc2;
      arc1 = aiter1.Value();
      arc2 = aiter2.Value();
      EXPECT_EQ(arc1.ilabel, arc2.ilabel);
      EXPECT_EQ(arc1.olabel, arc2.olabel);
      EXPECT_EQ(arc1.nextstate, arc2.nextstate);
      EXPECT_EQ(arc1.weight, arc2.weight);
      aiter1.Next();
      aiter2.Next();
    }
    siter1.Next();
    siter2.Next();
  }
}

TYPED_TEST(CompactorTest, EqualsTest) {
  EXPECT_TRUE(Isomorphic(this->vecfst, this->ltfst));
}

TYPED_TEST(CompactorTest, ShortestDistanceTest) {
  auto correct = ShortestDistance(this->vecfst);
  auto test = ShortestDistance(this->ltfst);
  EXPECT_EQ(correct, test);
}

TYPED_TEST(CompactorTest, ConvertBackToVectorTest) {
  fst::StdVectorFst vecfst;
  vecfst.AddState();
  vecfst.AddState();
  vecfst.AddState();
  vecfst.SetStart(0);
  vecfst.AddArc(0, StdArc(1, 1, StdArc::Weight::One(), 1));
  vecfst.AddArc(0, StdArc(2, 2, StdArc::Weight::One(), 2));

  vecfst.SetFinal(1, StdArc::Weight::One());
  vecfst.SetFinal(2, StdArc::Weight::One());

  TypeParam ltfst(vecfst);
  StdVectorFst vecfst2(ltfst);

  EXPECT_TRUE(fst::Isomorphic(vecfst, vecfst2));
  // SetUp sets FLAGS_fst_verify_properties, so this checks the computed
  // properties against the cached ones.
  EXPECT_EQ(vecfst.Properties(kFstProperties, /*test=*/true),
            vecfst2.Properties(kFstProperties, /*test=*/true));
}

// BENCHMARKS

void BM_CountStatesNGramVecFst(benchmark::State& state) {
  auto num_words = state.range(0);
  std::unique_ptr<StdMutableFst> fst(StdMutableFst::Read(Testfile()));
  std::vector<StdArc::StateId> order;
  NGramFst<StdArc> ngram(*fst, &order);

  std::vector<std::vector<StdArc::Label>> words = GetWords(num_words, 35);
  StdVectorFst trie = BuildTrie(words);
  for (auto _ : state) {
    auto compose = ComposeFst<StdArc>(trie, ngram, fst::CacheOptions(true, 0));
    CHECK_GT(fst::CountStates(compose), 0);
  }
}
BENCHMARK(BM_CountStatesNGramVecFst)->Range(100, 100000);

template <class FstType>
void BM_TemplatedCountStates(benchmark::State& state) {
  auto num_words = state.range(0);
  std::unique_ptr<StdMutableFst> fst(StdMutableFst::Read(Testfile()));
  std::vector<StdArc::StateId> order;
  NGramFst<StdArc> ngram(*fst, &order);

  std::vector<std::vector<StdArc::Label>> words = GetWords(num_words, 35);
  StdVectorFst vectrie = BuildTrie(words);
  FstType trie(vectrie);

  for (auto _ : state) {
    auto compose = ComposeFst<StdArc>(trie, ngram, fst::CacheOptions(true, 0));
    CHECK_GT(fst::CountStates(compose), 0);
  }
}

BENCHMARK(BM_TemplatedCountStates<StdCompactLoudsTreeFst>)->Range(100, 100000);
BENCHMARK(BM_TemplatedCountStates<StdUnweightedCompactLoudsTreeFst>)
    ->Range(100, 100000);
BENCHMARK(BM_TemplatedCountStates<StdAcceptorLoudsTreeFst>)->Range(100, 100000);
BENCHMARK(BM_TemplatedCountStates<StdUnweightedAcceptorCompactLoudsTreeFst>)
    ->Range(100, 100000);

}  // namespace
}  // namespace fst
