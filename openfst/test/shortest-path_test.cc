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
//
// Unit test for ShortestPath.

#include "openfst/lib/shortest-path.h"

#include <cstddef>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "openfst/compat/file_path.h"
#include "gtest/gtest.h"
#include "absl/log/log.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/arcfilter.h"
#include "openfst/lib/equal.h"
#include "openfst/lib/float-weight.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/queue.h"
#include "openfst/lib/vector-fst.h"
#include "openfst/lib/verify.h"
#include "openfst/lib/weight.h"

namespace fst {
namespace {

using Arc = StdArc;
using StateId = Arc::StateId;
using Weight = Arc::Weight;

constexpr char kTestDataDir[] =
    "openfst/test/testdata/shortest-path";

class ShortestPathTest : public testing::Test {
 protected:
  void SetUp() override {
    const std::string path = JoinPath(std::string("."), kTestDataDir);
    const std::string shortest_path1_name = JoinPath(path, "sp1.fst");
    const std::string shortest_path2_name = JoinPath(path, "sp2.fst");
    const std::string shortest_path3_name = JoinPath(path, "sp3.fst");
    const std::string shortest_path4_name = JoinPath(path, "sp4.fst");
    const std::string shortest_path5_name = JoinPath(path, "sp5.fst");
    const std::string shortest_path6_name = JoinPath(path, "sp6.fst");
    const std::string shortest_path7_name = JoinPath(path, "sp7.fst");
    const std::string shortest_path8_name = JoinPath(path, "sp8.fst");
    const std::string shortest_path9_name = JoinPath(path, "sp9.fst");

    spfst1_.reset(VectorFst<Arc>::Read(shortest_path1_name));
    spfst2_.reset(VectorFst<Arc>::Read(shortest_path2_name));
    spfst3_.reset(VectorFst<Arc>::Read(shortest_path3_name));
    spfst4_.reset(VectorFst<Arc>::Read(shortest_path4_name));
    spfst5_.reset(VectorFst<Arc>::Read(shortest_path5_name));
    spfst6_.reset(VectorFst<Arc>::Read(shortest_path6_name));
    spfst7_.reset(VectorFst<Arc>::Read(shortest_path7_name));
    spfst8_.reset(VectorFst<Arc>::Read(shortest_path8_name));
    spfst9_.reset(VectorFst<Arc>::Read(shortest_path9_name));
  }

  std::unique_ptr<const VectorFst<Arc>> spfst1_;
  std::unique_ptr<const VectorFst<Arc>> spfst2_;
  std::unique_ptr<const VectorFst<Arc>> spfst3_;
  std::unique_ptr<const VectorFst<Arc>> spfst4_;
  std::unique_ptr<const VectorFst<Arc>> spfst5_;
  std::unique_ptr<const VectorFst<Arc>> spfst6_;
  std::unique_ptr<const VectorFst<Arc>> spfst7_;
  std::unique_ptr<const VectorFst<Arc>> spfst8_;
  std::unique_ptr<const VectorFst<Arc>> spfst9_;
};

TEST_F(ShortestPathTest, ShortestPath) {
  VectorFst<Arc> nfst;
  VectorFst<Arc> paths;

  // 1-best.
  ShortestPath(nfst, &paths);
  ASSERT_TRUE(Verify(paths));
  ASSERT_TRUE(Equal(nfst, paths));

  // 4-best.
  ShortestPath(nfst, &paths, 4);
  ASSERT_TRUE(Verify(paths));
  ASSERT_TRUE(Equal(nfst, paths));

  // Unique 4-best.
  ShortestPath(nfst, &paths, 4, true);
  ASSERT_TRUE(Verify(paths));
  ASSERT_TRUE(Equal(nfst, paths));

  // Pruned 4-best.
  {
    std::vector<Weight> distance;
    AnyArcFilter<Arc> arc_filter;
    AutoQueue<StateId> state_queue(nfst, &distance, arc_filter);

    ShortestPathOptions<Arc, AutoQueue<StateId>, AnyArcFilter<Arc>> sopts(
        &state_queue, arc_filter, 4, false, false, kDelta, false, Weight(1.0),
        1);
    ShortestPath(nfst, &paths, &distance, sopts);
    ASSERT_TRUE(Verify(paths));
    ASSERT_TRUE(Equal(nfst, paths));
  }

  // 1-best.
  ShortestPath(*spfst1_, &paths);
  ASSERT_TRUE(Verify(paths));
  ASSERT_TRUE(Equal(*spfst2_, paths));

  // 4-best.
  ShortestPath(*spfst1_, &paths, 4);
  ASSERT_TRUE(Verify(paths));
  ASSERT_TRUE(Equal(*spfst3_, paths));

  // Unique 4-best.
  ShortestPath(*spfst1_, &paths, 4, true);
  ASSERT_TRUE(Verify(paths));
  ASSERT_TRUE(Equal(*spfst4_, paths));

  // 1-best.
  ShortestPath(*spfst5_, &paths);
  ASSERT_TRUE(Verify(paths));
  ASSERT_TRUE(Equal(*spfst6_, paths));

  // 4-best.
  ShortestPath(*spfst5_, &paths, 4);
  ASSERT_TRUE(Verify(paths));
  ASSERT_TRUE(Equal(*spfst7_, paths));

  // Unique 4-best.
  ShortestPath(*spfst5_, &paths, 4, true);
  ASSERT_TRUE(Verify(paths));
  ASSERT_TRUE(Equal(*spfst8_, paths));

  // Pruned 4-best.
  {
    std::vector<Weight> distance;
    AnyArcFilter<Arc> arc_filter;
    AutoQueue<StateId> state_queue(*spfst5_, &distance, arc_filter);

    ShortestPathOptions<Arc, AutoQueue<StateId>, AnyArcFilter<Arc>> sopts(
        &state_queue, arc_filter, 4, false, false, kDelta, false, Weight(1.0),
        20);
    ShortestPath(*spfst5_, &paths, &distance, sopts);
    ASSERT_TRUE(Verify(paths));
    ASSERT_TRUE(Equal(*spfst9_, paths));
  }
}

TEST_F(ShortestPathTest, ShortestPathWithOptions) {
  using E = TrivialAStarEstimate<StateId, Weight>;
  using C = TrivialStateEquivClass<StateId>;

  std::vector<Weight> distance;
  VectorFst<Arc> paths;

  AnyArcFilter<Arc> arc_filter;
  std::vector<std::unique_ptr<QueueBase<StateId>>> queues;

  queues.push_back(std::make_unique<FifoQueue<StateId>>());
  queues.push_back(std::make_unique<LifoQueue<StateId>>());
  queues.push_back(
      std::make_unique<NaturalShortestFirstQueue<StateId, Weight>>(distance));
  queues.push_back(
      std::make_unique<NaturalAStarQueue<StateId, Weight, E>>(distance, E()));
  queues.push_back(
      std::make_unique<NaturalPruneQueue<FifoQueue<StateId>, Weight, C>>(
          distance, std::make_unique<FifoQueue<StateId>>(), C(),
          Weight::Zero()));
  queues.push_back(
      std::make_unique<AutoQueue<StateId>>(*spfst1_, &distance, arc_filter));
  queues.push_back(
      std::make_unique<PruneNaturalShortestFirstQueue<StateId, Weight>>(
          distance, 5, 1000));

  for (auto i = 0; i < queues.size(); ++i) {
    VLOG(1) << "Testing spfst1 with queue #" << i;

    ShortestPathOptions<Arc, QueueBase<StateId>, AnyArcFilter<Arc>> opts(
        queues[i].get(), arc_filter);
    ShortestPath(*spfst1_, &paths, &distance, opts);
    ASSERT_TRUE(Verify(paths));
    ASSERT_TRUE(Equal(*spfst2_, paths));

    ASSERT_EQ(distance.size(), 5);
    ASSERT_EQ(distance[0], Weight::One());
    ASSERT_EQ(distance[1], Weight(3.0));
    ASSERT_EQ(distance[2], Weight(5.0));
    ASSERT_EQ(distance[3], Weight(7.0));
    ASSERT_EQ(distance[4], Weight(4.0));

    opts.nshortest = 4;
    ShortestPath(*spfst1_, &paths, &distance, opts);
    ASSERT_TRUE(Verify(paths));
    ASSERT_TRUE(Equal(*spfst3_, paths));

    ASSERT_EQ(distance.size(), 5);
    ASSERT_EQ(distance[0], Weight::One());
    ASSERT_EQ(distance[1], Weight(3.0));
    ASSERT_EQ(distance[2], Weight(5.0));
    ASSERT_EQ(distance[3], Weight(7.0));
    ASSERT_EQ(distance[4], Weight(4.0));

    opts.unique = true;
    ShortestPath(*spfst1_, &paths, &distance, opts);
    ASSERT_TRUE(Verify(paths));
    ASSERT_TRUE(Equal(*spfst4_, paths));

    ASSERT_EQ(distance.size(), 5);
    ASSERT_EQ(distance[0], Weight::One());
    ASSERT_EQ(distance[1], Weight(3.0));
    ASSERT_EQ(distance[2], Weight(5.0));
    ASSERT_EQ(distance[3], Weight(7.0));
    ASSERT_EQ(distance[4], Weight(4.0));
  }

  queues.clear();
  queues.push_back(std::make_unique<FifoQueue<StateId>>());
  queues.push_back(std::make_unique<LifoQueue<StateId>>());
  queues.push_back(
      std::make_unique<TopOrderQueue<StateId>>(*spfst5_, arc_filter));
  queues.push_back(std::make_unique<StateOrderQueue<StateId>>());
  queues.push_back(
      std::make_unique<AutoQueue<StateId>>(*spfst5_, &distance, arc_filter));

  for (auto i = 0; i < queues.size(); ++i) {
    VLOG(1) << "Testing spfst5 with queue #" << i;

    ShortestPathOptions<Arc, QueueBase<StateId>, AnyArcFilter<Arc>> opts(
        queues[i].get(), arc_filter);
    ShortestPath(*spfst5_, &paths, &distance, opts);
    ASSERT_TRUE(Verify(paths));
    ASSERT_TRUE(Equal(*spfst6_, paths));

    ASSERT_EQ(distance.size(), 5);
    ASSERT_EQ(distance[0], Weight::One());
    ASSERT_EQ(distance[1], Weight(3.0));
    ASSERT_EQ(distance[2], Weight(5.0));
    ASSERT_EQ(distance[3], Weight(7.0));
    ASSERT_EQ(distance[4], Weight(9.0));

    opts.nshortest = 4;
    ShortestPath(*spfst5_, &paths, &distance, opts);
    ASSERT_TRUE(Verify(paths));
    ASSERT_TRUE(Equal(*spfst7_, paths));

    ASSERT_EQ(distance.size(), 5);
    ASSERT_EQ(distance[0], Weight::One());
    ASSERT_EQ(distance[1], Weight(3.0));
    ASSERT_EQ(distance[2], Weight(5.0));
    ASSERT_EQ(distance[3], Weight(7.0));
    ASSERT_EQ(distance[4], Weight(9.0));

    opts.unique = true;
    ShortestPath(*spfst5_, &paths, &distance, opts);
    ASSERT_TRUE(Verify(paths));
    ASSERT_TRUE(Equal(*spfst8_, paths));

    ASSERT_EQ(distance.size(), 5);
    ASSERT_EQ(distance[0], Weight::One());
    ASSERT_EQ(distance[1], Weight(3.0));
    ASSERT_EQ(distance[2], Weight(5.0));
    ASSERT_EQ(distance[3], Weight(7.0));
    ASSERT_EQ(distance[4], Weight(9.0));
  }
}

TEST_F(ShortestPathTest, ShortestPathTree) {
  std::vector<StdArc::Weight> distance;
  std::vector<std::pair<StdArc::StateId, size_t>> parent;
  AnyArcFilter<Arc> arc_filter;
  AutoQueue<typename Arc::StateId> state_queue(*spfst5_, &distance, arc_filter);
  ShortestPathOptions<StdArc, QueueBase<StateId>, AnyArcFilter<StdArc>> opts(
      &state_queue, arc_filter);
  StateId notused;
  ASSERT_TRUE(internal::SingleShortestPath(*spfst5_, &distance, opts, &notused,
                                           &parent));
}

TEST_F(ShortestPathTest, TreeNotValid) {
  std::unique_ptr<StdFst> fst(StdFst::Read(
      JoinPath(std::string("."), kTestDataDir, "vexing_tree.fst")));
  std::vector<StdArc::Weight> distance;
  std::vector<std::pair<StdArc::StateId, size_t>> parent;
  AnyArcFilter<StdArc> arc_filter;
  AutoQueue<StdArc::StateId> state_queue(*fst, &distance, arc_filter);
  ShortestPathOptions<StdArc, QueueBase<StdArc::StateId>, AnyArcFilter<StdArc>>
      opts(&state_queue, arc_filter);
  StdArc::StateId notused;
  ASSERT_TRUE(
      internal::SingleShortestPath(*fst, &distance, opts, &notused, &parent));
}

std::pair<StdVectorFst, std::vector<StdArc::Weight>> TestFirstPath(
    const StdFst& fst) {
  std::vector<StdArc::Weight> distance;
  NaturalShortestFirstQueue<StdArc::StateId, StdArc::Weight> state_queue(
      distance);
  ShortestPathOptions<
      StdArc, NaturalShortestFirstQueue<StdArc::StateId, StdArc::Weight>,
      AnyArcFilter<StdArc>>
      shortest_path_opts(&state_queue, AnyArcFilter<StdArc>());
  shortest_path_opts.first_path = true;
  StdVectorFst shortest;
  ShortestPath(fst, &shortest, &distance, shortest_path_opts);
  return {shortest, std::move(distance)};
}

TEST_F(ShortestPathTest, FirstPath_Branch) {
  // A situation where two arcs from a state both lead to a final state but one
  // has smaller final weight.
  std::unique_ptr<StdFst> fst(StdFst::Read(JoinPath(
      std::string("."), kTestDataDir, "first_path_branch.fst")));
  auto shortest_distance = TestFirstPath(*fst);
  const auto& shortest = shortest_distance.first;
  // The final weight should be StdArc::Weight::One().
  for (StateIterator<StdVectorFst> siter(shortest); !siter.Done();
       siter.Next()) {
    auto final_weight = shortest.Final(siter.Value());
    if (final_weight != StdArc::Weight::Zero()) {
      EXPECT_EQ(0, final_weight.Value());
    }
  }
}

TEST_F(ShortestPathTest, FirstPath_Continue) {
  // A situation where a final state has an arc that leads to an even shorter
  // path.
  std::unique_ptr<StdFst> fst(StdFst::Read(JoinPath(
      std::string("."), kTestDataDir, "first_path_continue.fst")));
  auto shortest_distance = TestFirstPath(*fst);
  const auto& shortest = shortest_distance.first;
  // The final weight should be StdArc::Weight::One().
  for (StateIterator<StdVectorFst> siter(shortest); !siter.Done();
       siter.Next()) {
    auto final_weight = shortest.Final(siter.Value());
    if (final_weight != StdArc::Weight::Zero()) {
      EXPECT_EQ(0, final_weight.Value());
    }
  }
}

TEST_F(ShortestPathTest, FirstPath_Early) {
  // Checks that we terminate as early as possible.
  std::unique_ptr<StdFst> fst(StdFst::Read(JoinPath(
      std::string("."), kTestDataDir, "first_path_early.fst")));
  auto shortest_distance = TestFirstPath(*fst);
  const auto& distance = shortest_distance.second;
  EXPECT_TRUE(distance.size() < 5 || distance[4] == StdArc::Weight::Zero())
      << "State 4 should not have been visited";
}

}  // namespace
}  // namespace fst
