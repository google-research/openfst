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
// Unit test for FST memory utilities.

#include "openfst/lib/memory.h"

#include <cstddef>
#include <functional>
#include <memory>
#include <set>
#include <utility>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace fst {
namespace {

using ::testing::ElementsAre;

class MemoryTest : public testing::Test {
 protected:
  template <class Allocator>
  void AllocatorSeqTest() {
    using S = std::vector<int, Allocator>;

    const int n = 16;
    S s;
    for (int j = 0; j < 1; ++j) {
      for (int i = 0; i < n; ++i) {
        s.push_back(i);
      }

      for (int i = n - 1; i >= 0; --i) {
        ASSERT_EQ(s.back(), i);
        s.pop_back();
      }
    }
  }

  template <class Allocator>
  void AllocatorAssocTest() {
    using S = std::set<int, std::less<int>, Allocator>;

    const int n = 16;
    S s;
    for (int j = 0; j < 1; ++j) {
      for (int i = 0; i < n; ++i) {
        s.insert(i);
      }

      for (int i = 0; i < n; ++i) {
        auto it = s.find(i);
        ASSERT_TRUE(it != s.end());
        s.erase(it);
      }
    }
  }
};

// Tests basic pool allocation
TEST_F(MemoryTest, MemoryPoolTest) {
  class A {
   public:
    explicit A(int i) : i_(i) {}

    int Value() const { return i_; }

    void* operator new(size_t size, MemoryPool<A>* pool) {
      return pool->Allocate();
    }

    static void Destroy(A* a, MemoryPool<A>* pool) {
      if (a) {
        a->~A();
        pool->Free(a);
      }
    }

   private:
    int i_;
  };

  MemoryPool<A> pool(4);
  const int n = 16;

  for (int j = 0; j < 2; ++j) {
    std::vector<A*> aptrs;
    for (int i = 0; i < n; ++i) aptrs.push_back(new (&pool) A(i));

    for (int i = 0; i < n; ++i) {
      ASSERT_EQ(aptrs[i]->Value(), i);
      A::Destroy(aptrs[i], &pool);
    }
  }
}

// Tests STL block allocation with sequence containers
TEST_F(MemoryTest, BlockAllocatorSeqTest) {
  AllocatorSeqTest<BlockAllocator<int>>();
}

// Tests STL block allocation with associative containers
TEST_F(MemoryTest, BlockAllocatorAssocTest) {
  AllocatorAssocTest<BlockAllocator<int>>();
}

// Tests STL pool allocation with sequence containers
TEST_F(MemoryTest, PoolAllocatorSeqTest) {
  AllocatorSeqTest<PoolAllocator<int>>();
}

// Tests STL pool allocation with associative containers
TEST_F(MemoryTest, PoolAllocatorAssocTest) {
  AllocatorAssocTest<PoolAllocator<int>>();
}

template <typename Allocator>
class AllocatorEqualityTest : public testing::Test {};

using AllocatorTypes = testing::Types<BlockAllocator<int>, PoolAllocator<int>>;
TYPED_TEST_SUITE(AllocatorEqualityTest, AllocatorTypes);

TYPED_TEST(AllocatorEqualityTest, CopiesCompareEqual) {
  const TypeParam alloc;
  const TypeParam copy(alloc);
  EXPECT_TRUE(alloc == alloc);
  EXPECT_TRUE(alloc == copy);
  EXPECT_FALSE(alloc != copy);
}

TYPED_TEST(AllocatorEqualityTest, DistinctAllocatorsCompareUnequal) {
  const TypeParam alloc1;
  const TypeParam alloc2;
  EXPECT_FALSE(alloc1 == alloc2);
  EXPECT_TRUE(alloc1 != alloc2);
}

TYPED_TEST(AllocatorEqualityTest, ReboundCopiesCompareEqual) {
  using ReboundAllocator =
      typename std::allocator_traits<TypeParam>::template rebind_alloc<double>;
  const TypeParam alloc;
  const ReboundAllocator rebound(alloc);
  EXPECT_TRUE(alloc == rebound);
  EXPECT_TRUE(rebound == alloc);
  EXPECT_TRUE(TypeParam(rebound) == alloc);
}

TYPED_TEST(AllocatorEqualityTest, MoveAssignmentStealsBufferWhenEqual) {
  using Vector = std::vector<int, TypeParam>;
  const TypeParam alloc;
  Vector v1(alloc);
  Vector v2(alloc);
  for (int i = 0; i < 8; ++i) v1.push_back(i);
  const int* const data = v1.data();
  v2 = std::move(v1);
  EXPECT_EQ(v2.data(), data);
  EXPECT_THAT(v2, ElementsAre(0, 1, 2, 3, 4, 5, 6, 7));
}

TYPED_TEST(AllocatorEqualityTest, SwapWithEqualAllocators) {
  using Vector = std::vector<int, TypeParam>;
  const TypeParam alloc;
  Vector v1({1, 2, 3}, alloc);
  Vector v2({4, 5}, alloc);
  v1.swap(v2);
  EXPECT_THAT(v1, ElementsAre(4, 5));
  EXPECT_THAT(v2, ElementsAre(1, 2, 3));
}

}  // namespace
}  // namespace fst
