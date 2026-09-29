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
// A weight wrapper that counts copies and moves, for testing that code paths
// move rather than copy weights.

#ifndef OPENFST_TEST_COUNTING_WEIGHT_H_
#define OPENFST_TEST_COUNTING_WEIGHT_H_

#include <cstddef>
#include <cstdint>
#include <istream>
#include <ostream>
#include <string>
#include <utility>

#include "absl/base/no_destructor.h"
#include "openfst/lib/weight.h"

namespace fst {
namespace test {

// Number of copies (construction and assignment) and moves (construction and
// assignment) of CountingWeight<W> objects.
struct CopyMoveCounts {
  int copies = 0;
  int moves = 0;
};

// Wraps the semiring W and forwards all semiring operations to it, counting
// copies and moves of the wrapper. Construction from a W (e.g., in `Zero()`,
// `Plus()`, etc.) is not counted. Counters are per W and not thread-safe;
// reset them with `CountingWeight<W>::ResetCounts()` before the code under
// test.
template <class W>
class CountingWeight {
 public:
  using ReverseWeight = CountingWeight<typename W::ReverseWeight>;

  CountingWeight() = default;

  explicit CountingWeight(const W& weight) : weight_(weight) {}

  explicit CountingWeight(W&& weight) noexcept : weight_(std::move(weight)) {}

  CountingWeight(const CountingWeight& other) : weight_(other.weight_) {
    ++MutableCounts().copies;
  }

  CountingWeight(CountingWeight&& other) noexcept
      : weight_(std::move(other.weight_)) {
    ++MutableCounts().moves;
  }

  CountingWeight& operator=(const CountingWeight& other) {
    weight_ = other.weight_;
    ++MutableCounts().copies;
    return *this;
  }

  CountingWeight& operator=(CountingWeight&& other) noexcept {
    weight_ = std::move(other.weight_);
    ++MutableCounts().moves;
    return *this;
  }

  static const CountingWeight& Zero() {
    static const absl::NoDestructor<CountingWeight> zero(W::Zero());
    return *zero;
  }

  static const CountingWeight& One() {
    static const absl::NoDestructor<CountingWeight> one(W::One());
    return *one;
  }

  static const CountingWeight& NoWeight() {
    static const absl::NoDestructor<CountingWeight> no_weight(W::NoWeight());
    return *no_weight;
  }

  static const std::string& Type() {
    static const absl::NoDestructor<std::string> type("counting_" + W::Type());
    return *type;
  }

  static constexpr uint64_t Properties() { return W::Properties(); }

  static const CopyMoveCounts& Counts() { return MutableCounts(); }

  static void ResetCounts() { MutableCounts() = CopyMoveCounts(); }

  const W& Value() const { return weight_; }
  W& Value() { return weight_; }

  bool Member() const { return weight_.Member(); }

  size_t Hash() const { return weight_.Hash(); }

  CountingWeight Quantize(float delta = kDelta) const {
    return CountingWeight(weight_.Quantize(delta));
  }

  ReverseWeight Reverse() const { return ReverseWeight(weight_.Reverse()); }

  std::istream& Read(std::istream& strm) { return weight_.Read(strm); }

  std::ostream& Write(std::ostream& strm) const { return weight_.Write(strm); }

 private:
  static CopyMoveCounts& MutableCounts() {
    static CopyMoveCounts counts;
    return counts;
  }

  W weight_{};
};

template <class W>
inline bool operator==(const CountingWeight<W>& w1,
                       const CountingWeight<W>& w2) {
  return w1.Value() == w2.Value();
}

template <class W>
inline bool operator!=(const CountingWeight<W>& w1,
                       const CountingWeight<W>& w2) {
  return !(w1 == w2);
}

template <class W>
inline bool ApproxEqual(const CountingWeight<W>& w1,
                        const CountingWeight<W>& w2, float delta = kDelta) {
  return ApproxEqual(w1.Value(), w2.Value(), delta);
}

template <class W>
inline CountingWeight<W> Plus(const CountingWeight<W>& w1,
                              const CountingWeight<W>& w2) {
  return CountingWeight<W>(Plus(w1.Value(), w2.Value()));
}

template <class W>
inline CountingWeight<W> Times(const CountingWeight<W>& w1,
                               const CountingWeight<W>& w2) {
  return CountingWeight<W>(Times(w1.Value(), w2.Value()));
}

template <class W>
inline CountingWeight<W> Divide(const CountingWeight<W>& w1,
                                const CountingWeight<W>& w2,
                                DivideType type = DIVIDE_ANY) {
  return CountingWeight<W>(Divide(w1.Value(), w2.Value(), type));
}

template <class W>
inline std::ostream& operator<<(std::ostream& strm,
                                const CountingWeight<W>& weight) {
  return strm << weight.Value();
}

template <class W>
inline std::istream& operator>>(std::istream& strm, CountingWeight<W>& weight) {
  return strm >> weight.Value();
}

}  // namespace test
}  // namespace fst

#endif  // OPENFST_TEST_COUNTING_WEIGHT_H_
