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
// Unit test for various FST state queues.

#include "openfst/lib/queue.h"

#include <vector>

#include "gtest/gtest.h"
#include "absl/log/log.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/arcfilter.h"
#include "openfst/lib/float-weight.h"
#include "openfst/lib/fst-decl.h"
#include "openfst/lib/properties.h"
#include "openfst/lib/vector-fst.h"

namespace fst {
namespace {

TEST(QueueTester, TrivialTest) {
  TrivialQueue<int> queue;
  queue.Enqueue(1);

  ASSERT_EQ(queue.Head(), 1);
  queue.Dequeue();

  queue.Enqueue(2);

  ASSERT_EQ(queue.Head(), 2);
  queue.Dequeue();
}

TEST(QueueTester, FifoTest) {
  FifoQueue<int> queue;

  queue.Enqueue(5);
  queue.Enqueue(6);
  queue.Enqueue(7);
  queue.Enqueue(1);
  queue.Enqueue(2);

  ASSERT_EQ(queue.Head(), 5);
  queue.Dequeue();

  ASSERT_EQ(queue.Head(), 6);
  queue.Dequeue();

  ASSERT_EQ(queue.Head(), 7);
  queue.Dequeue();

  ASSERT_EQ(queue.Head(), 1);
  queue.Dequeue();

  ASSERT_EQ(queue.Head(), 2);
  queue.Dequeue();
}

TEST(QueueTester, LifoTest) {
  LifoQueue<int> queue;

  queue.Enqueue(5);
  queue.Enqueue(6);
  queue.Enqueue(7);
  queue.Enqueue(1);
  queue.Enqueue(2);

  ASSERT_EQ(queue.Head(), 2);
  queue.Dequeue();

  ASSERT_EQ(queue.Head(), 1);
  queue.Dequeue();

  ASSERT_EQ(queue.Head(), 7);
  queue.Dequeue();

  ASSERT_EQ(queue.Head(), 6);
  queue.Dequeue();

  ASSERT_EQ(queue.Head(), 5);
  queue.Dequeue();
}

class Compare {
 public:
  explicit Compare(const std::vector<float>* weights) : weights_(weights) {}

  bool operator()(const int x, const int y) const {
    return (*weights_)[x] < (*weights_)[y];
  }

 private:
  const std::vector<float>* weights_;
};

TEST(QueueTester, ShortestFirstTest) {
  std::vector<float> weights(5);
  weights[0] = 1.0;
  weights[1] = 2.0;
  weights[2] = 3.0;
  weights[3] = 4.0;
  weights[4] = 5.0;

  Compare compare(&weights);
  ShortestFirstQueue<int, Compare> queue(compare);

  queue.Enqueue(4);
  queue.Enqueue(2);
  queue.Enqueue(3);
  queue.Enqueue(0);
  queue.Enqueue(1);

  weights[3] = 6.0;
  queue.Update(3);

  ASSERT_EQ(queue.Head(), 0);
  queue.Dequeue();

  ASSERT_EQ(queue.Head(), 1);
  queue.Dequeue();

  ASSERT_EQ(queue.Head(), 2);
  queue.Dequeue();

  ASSERT_EQ(queue.Head(), 4);
  queue.Dequeue();

  ASSERT_EQ(queue.Head(), 3);
  queue.Dequeue();
}

TEST(QueueTester, PruneNaturalShortestFirstTest) {
  std::vector<TropicalWeight> distance;
  for (int i = 0; i < 52; i++) {
    distance.push_back(TropicalWeight(i));
  }

  PruneNaturalShortestFirstQueue<int, TropicalWeight> queue(distance, 2, 1000);

  // Enqueue 0, it's the start.
  queue.Enqueue(0);
  ASSERT_EQ(0, queue.Head());

  // Enqueue 1 and 50 (50 is a very expensive path and will not be expanded
  // for a long time).
  queue.Dequeue();
  queue.Enqueue(1);
  queue.Enqueue(50);

  // Follow a path of 1, 2, 3, 4, 5, ...
  for (int i = 1; i < 10; i++) {
    ASSERT_EQ(i, queue.Head());
    queue.Dequeue();
    queue.Enqueue(i + 1);
  }

  ASSERT_EQ(10, queue.Head());
  queue.Dequeue();

  ASSERT_EQ(50, queue.Head());
  queue.Dequeue();

  // 51 will have a step length of 3 (0 -> 50 -> 51) but the max step length
  // is 11, so this enqueue should be pruned.
  queue.Enqueue(51);
  ASSERT_TRUE(queue.Empty());
}

TEST(QueueTester, PruneNaturalShortestFirstStateLimitTest) {
  std::vector<TropicalWeight> distance;
  for (int i = 0; i < 5000; i++) {
    distance.push_back(TropicalWeight(i));
  }

  const int kThreshold = 20;
  const int kStateLimit = 3;
  PruneNaturalShortestFirstQueue<int, TropicalWeight> queue(
      distance, kThreshold, kStateLimit);

  // Enqueue 0, it's the start.
  queue.Enqueue(0);
  ASSERT_EQ(0, queue.Head());

  queue.Dequeue();

  // This path is cheap.
  queue.Enqueue(5);

  // This path is not so cheap.
  queue.Enqueue(23);

  // This makes the queue huge so that the state limit kicks in.
  for (int i = 0; i < 25; i++) {
    queue.Enqueue(100 + i);
  }

  // Now we'll expand some paths.
  // Since we expanded a lot of paths before we should already
  // we above the state limit.
  while (true) {
    int state = queue.Head();
    LOG(INFO) << state;
    queue.Dequeue();
    // Don't enqueue number 23 (we already did that).
    if (state + 1 != 23) {
      queue.Enqueue(state + 1);
    }

    if (state == 23) {
      // We expect that the queue.Enqueue(24) call was just pruned.
      // That's because the path length is short (length 3) compared
      // with the current best length (19). The difference (19-3) is less than
      // the default threshold (20), but the state limit should have reduced the
      // threshold to 11, causing this state to get pruned.
      break;
    }
  }

  // If no state limit is set, these are 26 for queue.Size() and 24 for
  // queue.Head().
  EXPECT_EQ(25, queue.Size());
  EXPECT_EQ(100, queue.Head());
}

TEST(QueueTester, PruneNaturalShortestFirstClearAllTest) {
  std::vector<TropicalWeight> distance;
  for (int i = 0; i < 5000; i++) {
    distance.push_back(TropicalWeight(i));
  }

  const int kThreshold = 20;
  const int kStateLimit = 3;
  PruneNaturalShortestFirstQueue<int, TropicalWeight> queue(
      distance, kThreshold, kStateLimit);

  // Enqueue 0, it's the start.
  queue.Enqueue(0);
  ASSERT_EQ(0, queue.Head());

  queue.Dequeue();

  // This path is cheap. State 5 has distance 3.
  queue.Enqueue(5);

  // This path is not so cheap. State 23 has distance 3.
  queue.Enqueue(23);

  // We expand state 5, so state 6 has distance 4.
  EXPECT_EQ(5, queue.Head());
  queue.Dequeue();
  queue.Enqueue(6);

  // State 30 has distance 5. It's the best path.
  EXPECT_EQ(6, queue.Head());
  queue.Dequeue();
  queue.Enqueue(30);

  // State 23 still has distance 3. It's the cheapest path but it's not
  // as long as state 30.
  EXPECT_EQ(23, queue.Head());

  // This makes the queue huge to encourage the queue to clear itself.
  for (int i = 0; i < 250; i++) {
    queue.Enqueue(100 + i);
  }

  // State 30 is still the best path so we haven't cleared anything.
  // However, once we got to size=57 (19*3), we stopped adding new states.
  EXPECT_EQ(57, queue.Size());

  // Now we're expanding the longest path.
  queue.Dequeue();
  EXPECT_EQ(30, queue.Head());

  // This uses the last spot in the queue.
  queue.Enqueue(31);

  // This clears everything else.
  queue.Enqueue(32);

  // We expect the queue to only contain the thing we just enqueued.
  EXPECT_EQ(1, queue.Size());
}

TEST(QueueTester, TopOrderTest) {
  StdVectorFst fst;
  fst.AddState();
  fst.AddState();
  fst.AddState();
  fst.SetStart(2);
  fst.SetFinal(0, StdArc::Weight::One());
  fst.AddArc(1, StdArc(1, 1, StdArc::Weight::One(), 0));
  fst.AddArc(2, StdArc(1, 1, StdArc::Weight::One(), 1));
  fst.AddArc(2, StdArc(1, 1, StdArc::Weight::One(), 0));

  TopOrderQueue<int> queue(fst, AnyArcFilter<StdArc>());
  queue.Enqueue(0);
  queue.Enqueue(1);
  queue.Enqueue(2);

  ASSERT_EQ(queue.Head(), 2);
  queue.Dequeue();

  ASSERT_EQ(queue.Head(), 1);
  queue.Dequeue();

  ASSERT_EQ(queue.Head(), 0);
  queue.Dequeue();
}

TEST(QueueTester, StateOrderTest) {
  StateOrderQueue<int> queue;
  queue.Enqueue(3);
  queue.Enqueue(2);
  queue.Enqueue(1);

  ASSERT_EQ(queue.Head(), 1);
  queue.Dequeue();

  ASSERT_EQ(queue.Head(), 2);
  queue.Dequeue();

  ASSERT_EQ(queue.Head(), 3);
  queue.Dequeue();
}

TEST(QueueTester, AutoQueueUnweightedAcyclicTest) {
  StdVectorFst fst;
  fst.AddState();
  fst.AddState();
  fst.AddState();
  fst.SetStart(2);
  fst.SetFinal(0, StdArc::Weight::One());
  fst.AddArc(1, StdArc(1, 1, StdArc::Weight::One(), 0));
  fst.AddArc(2, StdArc(1, 1, StdArc::Weight::One(), 1));
  fst.AddArc(2, StdArc(1, 1, StdArc::Weight::One(), 0));

  constexpr uint64_t kExpectedProps = kAcyclic | kUnweighted | kNotTopSorted;
  ASSERT_EQ(fst.Properties(kExpectedProps, true), kExpectedProps);

  // Enqueuing (0, 2, 1) distinguishes LIFO (1, 2, 0), topological (2, 1, 0),
  // state-order (0, 1, 2), and FIFO (0, 2, 1).
  AutoQueue<int> queue(fst, /*distance=*/nullptr, AnyArcFilter<StdArc>());
  queue.Enqueue(0);
  queue.Enqueue(2);
  queue.Enqueue(1);

  // Unweighted acyclic FST over an idempotent semiring uses LifoQueue.
  EXPECT_EQ(queue.Head(), 1);
  queue.Dequeue();
  EXPECT_EQ(queue.Head(), 2);
  queue.Dequeue();
  EXPECT_EQ(queue.Head(), 0);
  queue.Dequeue();
  EXPECT_TRUE(queue.Empty());
}

TEST(QueueTester, AutoQueueWeightedAcyclicTest) {
  StdVectorFst fst;
  fst.AddState();
  fst.AddState();
  fst.AddState();
  fst.SetStart(2);
  fst.SetFinal(0, StdArc::Weight::One());
  fst.AddArc(1, StdArc(1, 1, StdArc::Weight::One(), 0));
  fst.AddArc(2, StdArc(1, 1, StdArc::Weight(2.0), 1));
  fst.AddArc(2, StdArc(1, 1, StdArc::Weight::One(), 0));

  constexpr uint64_t kExpectedProps = kAcyclic | kWeighted | kNotTopSorted;
  ASSERT_EQ(fst.Properties(kExpectedProps, true), kExpectedProps);

  // Enqueuing (0, 2, 1) distinguishes LIFO (1, 2, 0), topological (2, 1, 0),
  // state-order (0, 1, 2), and FIFO (0, 2, 1).
  AutoQueue<int> queue(fst, /*distance=*/nullptr, AnyArcFilter<StdArc>());
  queue.Enqueue(0);
  queue.Enqueue(2);
  queue.Enqueue(1);

  // Weighted acyclic FST uses TopOrderQueue.
  EXPECT_EQ(queue.Head(), 2);
  queue.Dequeue();
  EXPECT_EQ(queue.Head(), 1);
  queue.Dequeue();
  EXPECT_EQ(queue.Head(), 0);
  queue.Dequeue();
  EXPECT_TRUE(queue.Empty());
}

}  // namespace
}  // namespace fst
