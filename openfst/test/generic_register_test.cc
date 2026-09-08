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

#include <thread>
#include <vector>

#include "gtest/gtest.h"
#include "openfst/test/my_register.h"

namespace fst {
namespace {

constexpr int kNumConcurrentReaderThreads = 8;
constexpr int kNumConcurrentReaderIterations = 1000;

// Simple test that ensures Generic Registers and Generic Registerers work in
// the most basic of cases, i.e., that a just-stored key-value pair can be
// retrieved.

TEST(GenericRegister, SimpleRegistrationWorks) {
  static MyRegisterer register_foo_to_bar("foo", "bar");

  MyRegister* reg = MyRegister::GetRegister();

  ASSERT_EQ("bar", reg->GetEntry("foo"));
}

TEST(GenericRegister, ConcurrentReaders) {
  static MyRegisterer register_apple("apple", "pie");
  static MyRegisterer register_banana("banana", "split");

  MyRegister* reg = MyRegister::GetRegister();

  std::vector<std::thread> threads;
  threads.reserve(kNumConcurrentReaderThreads);

  for (int i = 0; i < kNumConcurrentReaderThreads; ++i) {
    threads.emplace_back([reg]() {
      for (int j = 0; j < kNumConcurrentReaderIterations; ++j) {
        EXPECT_EQ("pie", reg->GetEntry("apple"));
        EXPECT_EQ("split", reg->GetEntry("banana"));
      }
    });
  }

  for (auto& t : threads) {
    t.join();
  }
}

}  // namespace
}  // namespace fst
