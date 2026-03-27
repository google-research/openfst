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

#include <memory>
#include <string>
#include <vector>

#include "openfst/compat/file_path.h"
#include "gtest/gtest.h"
#include "absl/log/log.h"
#include "absl/types/span.h"
#include "openfst/lib/arc-map.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/arcfilter.h"
#include "openfst/lib/equal.h"
#include "openfst/lib/float-weight.h"
#include "openfst/lib/fst-decl.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/queue.h"
#include "openfst/lib/reweight.h"
#include "openfst/lib/shortest-distance.h"
#include "openfst/lib/shortest-path.h"
#include "openfst/lib/topsort.h"
#include "openfst/lib/vector-fst.h"

namespace fst {
namespace {

using ::testing::Test;

using Arc = StdArc;
using StateId = typename Arc::StateId;
using Weight = typename Arc::Weight;

class AStarTest : public Test {
 protected:
  void SetUp() final {
    const std::string path =
        JoinPath(std::string("."),
                       "openfst/test/testdata/a-star");
    const std::string dfa_name = JoinPath(path, "dfa.fst");
    const std::string shortest_name = JoinPath(path, "shortest.fst");

    dfa_.reset(VectorFst<Arc>::Read(dfa_name));
    ASSERT_TRUE(dfa_) << "Failed to read " << dfa_name;
    shortest_.reset(VectorFst<Arc>::Read(shortest_name));
    ASSERT_TRUE(shortest_) << "Failed to read " << shortest_name;
  }

  std::unique_ptr<const VectorFst<StdArc>> dfa_;
  std::unique_ptr<const VectorFst<StdArc>> shortest_;
};

StateId CountExploredStates(absl::Span<const Weight> distance) {
  StateId n = 0;
  for (const auto& w : distance) {
    if (w != Weight::Zero()) ++n;
  }
  return n;
}

TEST_F(AStarTest, ExactAStar) {
  // "Exact" A*, i.e., when the estimated future cost equals the true future
  // cost.
  std::vector<Weight> beta;
  ShortestDistance(*dfa_, &beta, /*reverse=*/true);

  using MyEstimate = NaturalAStarEstimate<StateId, Weight>;
  using MyQueue = NaturalAStarQueue<StateId, Weight, MyEstimate>;
  using MyArcFilter = AnyArcFilter<Arc>;
  using MyShortestPathOptions = ShortestPathOptions<Arc, MyQueue, MyArcFilter>;
  const MyEstimate estimate(beta);
  std::vector<Weight> alpha;
  MyQueue queue(alpha, estimate);
  const MyArcFilter filter;
  const MyShortestPathOptions opts(&queue, filter,
                                   /*nshortest=*/1,
                                   /*unique=*/false,  // Already deterministic.
                                   /*has_distance=*/true,
                                   /*delta=*/kShortestDelta,
                                   /*first_path=*/true);
  VectorFst<Arc> shortest;
  ShortestPath(*dfa_, &shortest, &alpha, opts);
  LOG(INFO) << "# states explored: " << CountExploredStates(alpha);

  // Cleans it up.
  TopSort(&shortest);
  ArcMap(&shortest, RmWeightMapper<Arc>());

  EXPECT_TRUE(Equal(*shortest_, shortest));
}

TEST_F(AStarTest, LogAStar) {
  // "Log" A*, i.e., when the estimated future cost equals the neglog sum
  // of all paths from considered state to a final state.
  VectorFst<LogArc> log_dfa;
  Cast(*dfa_, &log_dfa);
  std::vector<LogWeight> log_beta;
  ShortestDistance(log_dfa, &log_beta, /*reverse=*/true);

  using MyEstimate = NaturalAStarEstimate<StateId, Weight>;
  using MyQueue = NaturalAStarQueue<StateId, Weight, MyEstimate>;
  using MyArcFilter = AnyArcFilter<Arc>;
  using MyShortestPathOptions = ShortestPathOptions<Arc, MyQueue, MyArcFilter>;
  const MyEstimate estimate(
      reinterpret_cast<const std::vector<Weight>&>(log_beta));
  std::vector<Weight> alpha;
  MyQueue queue(alpha, estimate);
  const MyArcFilter filter;
  const MyShortestPathOptions opts(&queue, filter,
                                   /*nshortest=*/1,
                                   /*unique=*/false,  // Already deterministic.
                                   /*has_distance=*/true,
                                   /*delta=*/kShortestDelta,
                                   /*first_path=*/true);
  VectorFst<Arc> shortest;
  ShortestPath(*dfa_, &shortest, &alpha, opts);
  LOG(INFO) << "# states explored: " << CountExploredStates(alpha);

  // Cleans it up.
  TopSort(&shortest);
  ArcMap(&shortest, RmWeightMapper<Arc>());

  EXPECT_TRUE(Equal(*shortest_, shortest));
}

TEST_F(AStarTest, ExactReweightedAStar) {
  // "Exact" A*, i.e., when the estimated future cost equals the true future
  // cost, simulating A* by first reweighting using the estimate and then
  // searching using the Dijkstra algorithm.
  VectorFst<Arc> dfa(*dfa_);
  std::vector<Weight> beta;
  ShortestDistance(dfa, &beta, /*reverse=*/true);
  Reweight(&dfa, beta, REWEIGHT_TO_INITIAL);

  using MyQueue = NaturalShortestFirstQueue<StateId, Weight>;
  using MyArcFilter = AnyArcFilter<Arc>;
  using MyShortestPathOptions = ShortestPathOptions<Arc, MyQueue, MyArcFilter>;
  std::vector<Weight> alpha;
  MyQueue queue(alpha);
  const MyArcFilter filter;
  const MyShortestPathOptions opts(&queue, filter,
                                   /*nshortest=*/1,
                                   /*unique=*/false,  // Already deterministic.
                                   /*has_distance=*/true,
                                   /*delta=*/kShortestDelta,
                                   /*first_path=*/true);
  VectorFst<Arc> shortest;
  ShortestPath(dfa, &shortest, &alpha, opts);
  LOG(INFO) << "# states explored: " << CountExploredStates(alpha);

  // Cleans it up.
  TopSort(&shortest);
  ArcMap(&shortest, RmWeightMapper<Arc>());

  EXPECT_TRUE(Equal(*shortest_, shortest));
}

TEST_F(AStarTest, LogReweightedAStar) {
  // "Log" A*, i.e., when the estimated future cost equals the neglog sum
  // of all paths from considered state to a final state, simulating A* by
  // reweighting using the estimate and then searching using the Dijkstra
  // algorithm.
  VectorFst<Arc> dfa(*dfa_);
  VectorFst<LogArc> log_dfa;
  Cast(dfa, &log_dfa);
  std::vector<LogWeight> log_beta;
  ShortestDistance(log_dfa, &log_beta, /*reverse=*/true);
  Reweight(&dfa, reinterpret_cast<const std::vector<Weight>&>(log_beta),
           REWEIGHT_TO_INITIAL);

  using MyQueue = NaturalShortestFirstQueue<StateId, Weight>;
  using MyArcFilter = AnyArcFilter<Arc>;
  using MyShortestPathOptions = ShortestPathOptions<Arc, MyQueue, MyArcFilter>;
  std::vector<Weight> alpha;
  MyQueue queue(alpha);
  const MyArcFilter filter;
  const MyShortestPathOptions opts(&queue, filter,
                                   /*nshortest=*/1,
                                   /*unique=*/false,  // Already deterministic.
                                   /*has_distance=*/true,
                                   /*delta=*/kShortestDelta,
                                   /*first_path=*/true);
  VectorFst<Arc> shortest;
  ShortestPath(dfa, &shortest, &alpha, opts);
  LOG(INFO) << "# states explored: " << CountExploredStates(alpha);

  // Cleans it up.
  TopSort(&shortest);
  ArcMap(&shortest, RmWeightMapper<Arc>());

  EXPECT_TRUE(Equal(*shortest_, shortest));
}

TEST_F(AStarTest, Dijkstra) {
  // Dijkstra, equivalent to A* when the estimated future cost equals
  // Weight::One().
  using MyQueue = NaturalShortestFirstQueue<StateId, Weight>;
  using MyArcFilter = AnyArcFilter<Arc>;
  using MyShortestPathOptions = ShortestPathOptions<Arc, MyQueue, MyArcFilter>;
  std::vector<Weight> alpha;
  MyQueue queue(alpha);
  const MyArcFilter filter;
  const MyShortestPathOptions opts(&queue, filter,
                                   /*nshortest=*/1,
                                   /*unique=*/false,  // Already deterministic.
                                   /*has_distance=*/true,
                                   /*delta=*/kShortestDelta,
                                   /*first_path=*/true);
  VectorFst<Arc> shortest;
  ShortestPath(*dfa_, &shortest, &alpha, opts);
  LOG(INFO) << "# states explored: " << CountExploredStates(alpha);

  // Cleans it up.
  TopSort(&shortest);
  ArcMap(&shortest, RmWeightMapper<Arc>());

  EXPECT_TRUE(Equal(*shortest_, shortest));
}

}  // namespace
}  // namespace fst
