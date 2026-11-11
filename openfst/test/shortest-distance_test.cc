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
// Unit test for ShortestDistance.

#include "openfst/lib/shortest-distance.h"

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <string>
#include <vector>

#include "openfst/compat/file_path.h"
#include "gtest/gtest.h"
#include "absl/log/log.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/arcfilter.h"
#include "openfst/lib/float-weight.h"
#include "openfst/lib/queue.h"
#include "openfst/lib/vector-fst.h"
#include "openfst/lib/weight.h"
#include "openfst/script/fst-class.h"
#include "openfst/script/shortest-distance.h"
#include "openfst/script/weight-class.h"

namespace fst {
namespace {

using Arc = StdArc;
using StateId = Arc::StateId;
using Weight = Arc::Weight;

class ShortestDistanceTest : public testing::Test {
 protected:
  void SetUp() override {
    const std::string path = JoinPath(
        std::string("."),
        "openfst/test/testdata/shortest-distance");
    const std::string shortest_distance1_name = JoinPath(path, "sd1.fst");
    const std::string shortest_distance2_name = JoinPath(path, "sd2.fst");

    sdfst1_.reset(VectorFst<Arc>::Read(shortest_distance1_name));
    sdfst2_.reset(VectorFst<Arc>::Read(shortest_distance2_name));
  }

  std::unique_ptr<const VectorFst<Arc>> sdfst1_;
  std::unique_ptr<const VectorFst<Arc>> sdfst2_;
};

TEST_F(ShortestDistanceTest, ShortestDistance) {
  VectorFst<Arc> nfst;

  std::vector<Weight> distance;

  ShortestDistance(nfst, &distance);
  ASSERT_TRUE(distance.empty());

  // Tests forward.
  ShortestDistance(*sdfst1_, &distance);
  ASSERT_EQ(distance.size(), 4);
  ASSERT_EQ(distance[0], Weight::One());
  ASSERT_EQ(distance[1], Weight(3.0));
  ASSERT_EQ(distance[2], Weight(5.0));
  ASSERT_EQ(distance[3], Weight(7.0));

  // Tests reverse.
  ShortestDistance(*sdfst1_, &distance, true);
  ASSERT_EQ(distance.size(), 4);
  ASSERT_EQ(distance[0], Weight(10.0));
  ASSERT_EQ(distance[1], Weight(7.0));
  ASSERT_EQ(distance[2], Weight(7.0));
  ASSERT_EQ(distance[3], Weight(3.0));

  // Tests forward.
  ShortestDistance(*sdfst2_, &distance);
  ASSERT_EQ(distance.size(), 4);
  ASSERT_EQ(distance[0], Weight::One());
  ASSERT_EQ(distance[1], Weight(3.0));
  ASSERT_EQ(distance[2], Weight(5.0));
  ASSERT_EQ(distance[3], Weight(7.0));

  // Tests reverse.
  ShortestDistance(*sdfst2_, &distance, true);
  ASSERT_EQ(distance.size(), 4);
  ASSERT_EQ(distance[0], Weight(10.0));
  ASSERT_EQ(distance[1], Weight(7.0));
  ASSERT_EQ(distance[2], Weight(7.0));
  ASSERT_EQ(distance[3], Weight(3.0));
}

// Test to verify that the convergence criterion kShortestDelta is small enough.
TEST_F(ShortestDistanceTest, kShortestDelta) {
  VectorFst<LogArc> sdfst;
  const LogArc::StateId start_state = sdfst.AddState();
  sdfst.SetStart(start_state);
  sdfst.SetFinal(start_state, LogArc::Weight::Zero());
  LogArc::StateId previous_state = start_state;
  // Number of arcs in the fst.
  const LogArc::StateId num_states = 5;
  // There are 2500 arcs between every two consecutive states and the
  // probability associated with label j proportational to j and sum of all
  // probabilities of outgoing arcs from a state is 1.0.
  const LogArc::Label num_arcs = 2500;
  for (LogArc::StateId i = 0; i < num_states; ++i) {
    const LogArc::StateId new_state = sdfst.AddState();
    sdfst.SetFinal(new_state, LogArc::Weight::Zero());
    for (LogArc::Label j = 1; j < num_arcs + 1; ++j) {
      const double inverse_weight =
          static_cast<double>(num_arcs * (num_arcs + 1)) / (2 * j);
      sdfst.AddArc(
          previous_state,
          LogArc(j, j, LogArc::Weight(std::log(inverse_weight)), new_state));
    }
    previous_state = new_state;
  }
  sdfst.SetFinal(previous_state, LogArc::Weight::One());
  std::vector<LogArc::Weight> distance;
  ShortestDistance(sdfst, &distance, /*reverse*/ true);
  ASSERT_NEAR(distance[0].Value(), LogArc::Weight::One().Value(), kDelta);
  // A test to demonstrate that convergence criterion kDelta is not sufficient.
  // TODO: remove the shortest distance test with convergence
  // criterion kDelta.
  std::vector<LogArc::Weight> distance_kDelta;
  ShortestDistance(sdfst, &distance_kDelta, /*reverse*/ true, kDelta);
  ASSERT_FALSE(std::abs(distance_kDelta[0].Value() -
                        LogArc::Weight::One().Value()) <= kDelta);
}

TEST_F(ShortestDistanceTest, ShortestDistanceWithOptions) {
  std::vector<Weight> distance;

  AnyArcFilter<Arc> arc_filter;
  std::vector<std::unique_ptr<QueueBase<StateId>>> queues;
  queues.push_back(std::make_unique<FifoQueue<StateId>>());
  queues.push_back(std::make_unique<LifoQueue<StateId>>());
  queues.push_back(
      std::make_unique<NaturalShortestFirstQueue<StateId, Weight>>(distance));
  queues.push_back(
      std::make_unique<AutoQueue<StateId>>(*sdfst1_, &distance, arc_filter));

  for (auto i = 0; i < queues.size(); ++i) {
    VLOG(1) << "Testing sdfst1 with queue #" << i;

    ShortestDistanceOptions<Arc, QueueBase<StateId>, AnyArcFilter<Arc>> opts(
        queues[i].get(), arc_filter);
    ShortestDistance(*sdfst1_, &distance, opts);

    ASSERT_EQ(distance.size(), 4);
    ASSERT_EQ(distance[0], Weight::One());
    ASSERT_EQ(distance[1], Weight(3.0));
    ASSERT_EQ(distance[2], Weight(5.0));
    ASSERT_EQ(distance[3], Weight(7.0));
  }

  queues.clear();
  queues.push_back(std::make_unique<FifoQueue<StateId>>());
  queues.push_back(std::make_unique<LifoQueue<StateId>>());
  queues.push_back(
      std::make_unique<TopOrderQueue<StateId>>(*sdfst2_, arc_filter));
  queues.push_back(std::make_unique<StateOrderQueue<StateId>>());
  queues.push_back(
      std::make_unique<AutoQueue<StateId>>(*sdfst2_, &distance, arc_filter));

  for (auto i = 0; i < queues.size(); ++i) {
    VLOG(1) << "Testing sdfst2 with queue #" << i;

    ShortestDistanceOptions<Arc, QueueBase<StateId>, AnyArcFilter<Arc>> opts(
        queues[i].get(), arc_filter);
    ShortestDistance(*sdfst2_, &distance, opts);

    ASSERT_EQ(distance.size(), 4);
    ASSERT_EQ(distance[0], Weight::One());
    ASSERT_EQ(distance[1], Weight(3.0));
    ASSERT_EQ(distance[2], Weight(5.0));
    ASSERT_EQ(distance[3], Weight(7.0));
  }
}

TEST_F(ShortestDistanceTest, ShortestDistanceFstClass) {
  namespace s = fst::script;

  const s::VectorFstClass sdfst1(*sdfst1_);
  const auto distance = s::ShortestDistance(sdfst1);
  s::WeightClass expected(Weight(10.0));
  ASSERT_EQ(distance, expected);
}

TEST(ShortestDistanceStateTest, RetainGenerationDoesNotWrap) {
  using SmallArc = ArcTpl<TropicalWeight, int, int8_t>;
  VectorFst<SmallArc> fst;
  fst.AddStates(4);
  fst.SetStart(0);
  fst.SetFinal(2, TropicalWeight::One());
  fst.AddArc(0, SmallArc(1, 1, TropicalWeight(1.0), 2));
  fst.AddArc(3, SmallArc(1, 1, TropicalWeight(10.0), 2));

  FifoQueue<int8_t> state_queue;
  const AnyArcFilter<SmallArc> arc_filter;
  const ShortestDistanceOptions<SmallArc, FifoQueue<int8_t>,
                                AnyArcFilter<SmallArc>>
      opts(&state_queue, arc_filter);
  std::vector<TropicalWeight> distance;
  internal::ShortestDistanceState<SmallArc, FifoQueue<int8_t>,
                                  AnyArcFilter<SmallArc>>
      sd_state(fst, &distance, opts, /*retain=*/true,
               std::numeric_limits<std::size_t>::max() - 256);

  // First call records state 2's generation for state 0's search.
  sd_state.ShortestDistance(0);
  ASSERT_FALSE(sd_state.Error());
  ASSERT_GT(distance.size(), 2);
  ASSERT_EQ(distance[2], TropicalWeight(1.0));

  // Execute enough calls on isolated state 1 that an int8_t counter would wrap
  // around to the first call's ID (1 + 255 == 0 mod 256) and the std::size_t
  // counter reaches std::numeric_limits<std::size_t>::max().
  for (int i = 0; i < 255; ++i) {
    sd_state.ShortestDistance(1);
  }
  ASSERT_FALSE(sd_state.Error());

  // Next call wraps generation_ from max() to 1 and zeroes generations_, so
  // state 2 is reset to Zero() before relaxing 3 -> 2 (weight 10).
  sd_state.ShortestDistance(3);
  ASSERT_FALSE(sd_state.Error());
  EXPECT_EQ(distance[2], TropicalWeight(10.0));
}

TEST(ShortestDistanceStateTest, RetainAdvancesGenerationAfterMemberError) {
  // State 0 relaxes 0 -> 2 (valid weight 1.0) and then aborts early on
  // 0 -> 3 (non-member weight). A subsequent call from state 1 must use a
  // fresh generation_ so neither state 2's distance/enqueued bit nor state 3's
  // non-member distance aliases into the new search, while Error() remains
  // sticky across calls (as required by RmEpsilonState::Error()).
  VectorFst<Arc> fst;
  fst.AddStates(4);
  fst.SetStart(0);
  fst.SetFinal(2, Weight::One());
  fst.SetFinal(3, Weight::One());
  fst.AddArc(0, Arc(1, 1, Weight(1.0), 2));
  fst.AddArc(0, Arc(2, 2, Weight::NoWeight(), 3));
  fst.AddArc(1, Arc(1, 1, Weight(5.0), 2));
  fst.AddArc(1, Arc(2, 2, Weight(7.0), 3));

  FifoQueue<StateId> state_queue;
  const AnyArcFilter<Arc> arc_filter;
  const ShortestDistanceOptions<Arc, FifoQueue<StateId>, AnyArcFilter<Arc>>
      opts(&state_queue, arc_filter);
  std::vector<Weight> distance;
  internal::ShortestDistanceState<Arc, FifoQueue<StateId>, AnyArcFilter<Arc>>
      sd_state(fst, &distance, opts, /*retain=*/true);

  sd_state.ShortestDistance(0);
  EXPECT_TRUE(sd_state.Error());
  ASSERT_GT(distance.size(), 3);
  EXPECT_FALSE(distance[3].Member());

  sd_state.ShortestDistance(1);
  EXPECT_TRUE(sd_state.Error());
  EXPECT_EQ(distance[2], Weight(5.0));
  EXPECT_EQ(distance[3], Weight(7.0));
}

}  // namespace
}  // namespace fst
