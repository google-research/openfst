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
// Unit test for multi-threaded access of finite-state transducer objects.

#include <cstdint>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "openfst/compat/file_path.h"
#include "gtest/gtest.h"
#include "absl/flags/flag.h"
#include "absl/functional/function_ref.h"
#include "absl/memory/memory.h"
#include "openfst/compat/compat_memory.h"
#include "absl/types/span.h"
#include "openfst/lib/arc-map.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/arcfilter.h"
#include "openfst/lib/arcsort.h"
#include "openfst/lib/closure.h"
#include "openfst/lib/compose.h"
#include "openfst/lib/concat.h"
#include "openfst/lib/const-fst.h"
#include "openfst/lib/determinize.h"
#include "openfst/lib/edit-fst.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/invert.h"
#include "openfst/lib/lookahead-matcher.h"
#include "openfst/lib/matcher-fst.h"
#include "openfst/lib/project.h"
#include "openfst/lib/properties.h"
#include "openfst/lib/queue.h"
#include "openfst/lib/rational.h"
#include "openfst/lib/rmepsilon.h"
#include "openfst/lib/union.h"
#include "openfst/lib/vector-fst.h"
#include "openfst/lib/verify.h"
#include "openfst/lib/visit.h"

ABSL_FLAG(int32_t, num_threads, 20, "Number of threads for parallel test");
ABSL_FLAG(int32_t, num_tasks, 1000, "Number of tasks for parallel test");

namespace fst {
namespace {

using Arc = StdArc;
using StateId = Arc::StateId;

class ThreadTest : public testing::Test {
 protected:
  // Verifies that `func` can be called in parallel on copies of `fst`.
  static void CopyTest(const Fst<Arc>& fst,
                       absl::FunctionRef<void(const Fst<Arc>&)> func,
                       bool safe = true) {
    const int32_t num_tasks = absl::GetFlag(FLAGS_num_tasks);
    const int32_t num_threads = absl::GetFlag(FLAGS_num_threads);
    ASSERT_GE(num_threads, 1);
    ASSERT_GE(num_tasks, num_threads);

    std::vector<std::unique_ptr<const Fst<Arc>>> tfsts;
    tfsts.reserve(num_tasks);
    for (int32_t i = 0; i < num_tasks; ++i) {
      tfsts.push_back(absl::WrapUnique(fst.Copy(safe)));
    }

    // Verifies their thread-safety in num_threads threads simultaneously.
    // Size of all but last batch.
    const int32_t batch_size = num_tasks / num_threads;
    // When C++20 is allowed, use std::jthread.
    std::vector<std::thread> threads;
    threads.reserve(num_threads);
    absl::Span<std::unique_ptr<const Fst<Arc>>> tfsts_span(tfsts);
    for (int32_t i = 0; i < num_threads; ++i) {
      absl::Span<std::unique_ptr<const Fst<Arc>>> thread_fsts;
      if (i == num_threads - 1) {
        thread_fsts = tfsts_span;
      } else {
        thread_fsts = tfsts_span.first(batch_size);
        tfsts_span = tfsts_span.subspan(batch_size);
      }
      threads.emplace_back([thread_fsts, func] {
        for (const auto& fst : thread_fsts) {
          func(*fst);
        }
      });
    }
    for (auto& thread : threads) {
      thread.join();
    }
  }

  static void Access(const Fst<Arc>& fst, StateId nstates) {
    if (nstates == 0) return;
    PartialVisitor<Arc> visitor(nstates);
    FifoQueue<StateId> queue;
    AnyArcFilter<Arc> filter;
    Visit(fst, &visitor, &queue, filter, /*access_only=*/true);
  }

  static void Verify(const Fst<Arc>& fst) { EXPECT_TRUE(fst::Verify(fst)); }

  static void TestProperties(const Fst<Arc>& fst) {
    fst.Properties(kFstProperties, /*test=*/true);
  }

  void SetUp() override {
    for (auto i = 0; i < 6; ++i) {
      afst_.push_back(absl::WrapUnique(VectorFst<Arc>::Read(
          JoinPath(std::string("."),
                         "openfst/test/testdata/thread/a" +
                             std::to_string(i) + ".fst"))));
      ArcSort(afst_.back().get(), OLabelCompare<Arc>());

      xfst_.push_back(absl::WrapUnique(
          ArcMapFst(*afst_.back(), IdentityArcMapper<Arc>()).Copy()));
    }

    for (auto i = 0; i < 6; ++i) {
      bfst_.push_back(absl::WrapUnique(VectorFst<Arc>::Read(
          JoinPath(std::string("."),
                         "openfst/test/testdata/thread/b" +
                             std::to_string(i) + ".fst"))));

      yfst_.push_back(absl::WrapUnique(
          ArcMapFst(*bfst_.back(), IdentityArcMapper<Arc>()).Copy()));
    }

    for (auto i = 0; i < 2; ++i) {
      cfst_.push_back(absl::WrapUnique(VectorFst<Arc>::Read(
          JoinPath(std::string("."),
                         "openfst/test/testdata/thread/c" +
                             std::to_string(i) + ".fst"))));

      zfst_.push_back(absl::WrapUnique(
          ArcMapFst(*cfst_.back(), IdentityArcMapper<Arc>()).Copy()));
    }
  }

  std::vector<std::unique_ptr<VectorFst<Arc>>> afst_;
  std::vector<std::unique_ptr<VectorFst<Arc>>> bfst_;
  std::vector<std::unique_ptr<VectorFst<Arc>>> cfst_;

  std::vector<std::unique_ptr<Fst<Arc>>> xfst_;
  std::vector<std::unique_ptr<Fst<Arc>>> yfst_;
  std::vector<std::unique_ptr<Fst<Arc>>> zfst_;
};

// FST CLASS TESTS
// Both Properties and Verify can be called on non-thread-safe
// copies since these classes are at least thread-compatible.

TEST_F(ThreadTest, VectorTest) {
  for (auto i = 0; i < afst_.size(); ++i) {
    CopyTest(*afst_[i], Verify, /*safe=*/false);
  }
}

TEST_F(ThreadTest, VectorPropertiesTest) {
  for (auto i = 0; i < afst_.size(); ++i) {
    CopyTest(*afst_[i], TestProperties, /*safe=*/false);
  }
}

TEST_F(ThreadTest, ConstTest) {
  for (auto i = 0; i < afst_.size(); ++i) {
    ConstFst<Arc> fst(*afst_[i]);
    CopyTest(fst, Verify, /*safe=*/false);
  }
}

TEST_F(ThreadTest, ConstPropertiesTest) {
  for (auto i = 0; i < afst_.size(); ++i) {
    ConstFst<Arc> fst(*afst_[i]);
    CopyTest(fst, TestProperties, /*safe=*/false);
  }
}

TEST_F(ThreadTest, EditTest) {
  for (auto i = 0; i < afst_.size(); ++i) {
    EditFst<Arc> fst(*afst_[i]);
    CopyTest(fst, Verify, /*safe=*/false);
  }
}

TEST_F(ThreadTest, EditPropertiesTest) {
  for (auto i = 0; i < afst_.size(); ++i) {
    EditFst<Arc> fst(*afst_[i]);
    CopyTest(fst, TestProperties, /*safe=*/false);
  }
}

// The following tests are use the Fsts in xfst_, yfst_, and zfst_.
// These are identity ArcMapFsts on the disk-read VectorFsts afst_,
// bfst_, and cfst_. In this way we test not only if the operations
// below correctly copy themselves but that they also correctly copy
// their input Fsts.

// FST ALGORITHM TESTS

TEST_F(ThreadTest, ArcSortTest) {
  for (auto i = 0; i < xfst_.size(); ++i) {
    ILabelCompare<Arc> cmp;
    ArcSortFst<Arc, ILabelCompare<Arc>> fst(*xfst_[i], cmp);
    CopyTest(fst, Verify);
  }
}

TEST_F(ThreadTest, ClosureTest) {
  for (auto i = 0; i < xfst_.size(); ++i) {
    ClosureFst<Arc> fst(*xfst_[i], CLOSURE_STAR);
    CopyTest(fst, Verify);
  }
}

TEST_F(ThreadTest, ComposeTest) {
  for (auto i = 0; i < xfst_.size(); ++i) {
    for (auto j = 0; j < yfst_.size(); ++j) {
      ComposeFst<Arc> fst(*xfst_[i], *yfst_[j]);
      Access(fst, (i + j) % 4);
      CopyTest(fst, Verify);
    }
  }
}

TEST_F(ThreadTest, ConcatTest) {
  for (auto i = 0; i < xfst_.size(); ++i) {
    for (auto j = 0; j < yfst_.size(); ++j) {
      ConcatFst<Arc> fst(*xfst_[i], *yfst_[j]);
      CopyTest(fst, Verify);
    }
  }
}

TEST_F(ThreadTest, DeterminizeTest) {
  for (auto i = 0; i < zfst_.size(); ++i) {
    DeterminizeFst<Arc> fst(*zfst_[i]);
    CopyTest(fst, Verify);
  }
}

TEST_F(ThreadTest, InvertTest) {
  for (auto i = 0; i < xfst_.size(); ++i) {
    InvertFst<Arc> fst(*xfst_[i]);
    CopyTest(fst, Verify);
  }
}

TEST_F(ThreadTest, LookAheadTest) {
  for (auto i = 0; i < xfst_.size(); ++i) {
    StdOLabelLookAheadFst xfst(*xfst_[i]);
    for (auto j = 0; j < yfst_.size(); ++j) {
      VectorFst<Arc> yfst(*yfst_[j]);
      LabelLookAheadRelabeler<Arc>::Relabel(&yfst, xfst,
                                            /*relabel_input=*/true);
      ComposeFst<Arc> fst(xfst, yfst);
      Access(fst, (i + j) % 4);
      CopyTest(fst, Verify);
    }
  }
}

TEST_F(ThreadTest, ProjectTest) {
  for (auto i = 0; i < xfst_.size(); ++i) {
    ProjectFst<Arc> fst(*xfst_[i], ProjectType::INPUT);
    CopyTest(fst, Verify);
  }
}

TEST_F(ThreadTest, RmEpsilonTest) {
  for (auto i = 0; i < xfst_.size(); ++i) {
    RmEpsilonFst<Arc> fst(*xfst_[i]);
    CopyTest(fst, Verify);
  }
}

TEST_F(ThreadTest, UnionTest) {
  for (auto i = 0; i < xfst_.size(); ++i) {
    for (auto j = 0; j < yfst_.size(); ++j) {
      UnionFst<Arc> fst(*xfst_[i], *yfst_[j]);
      CopyTest(fst, Verify);
    }
  }
}

}  // namespace
}  // namespace fst
