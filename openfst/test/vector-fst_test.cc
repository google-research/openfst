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
// Unit test for VectorFst and friends.

#include "openfst/lib/vector-fst.h"

#include <array>
#include <ostream>
#include <string>
#include <type_traits>
#include <typeinfo>
#include <utility>

#include "gtest/gtest.h"
#include "absl/base/no_destructor.h"
#include "absl/log/log.h"
#include "absl/strings/string_view.h"
#include "openfst/lib/expanded-fst.h"
#include "openfst/lib/float-weight.h"
#include "openfst/lib/fst-decl.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/impl-to-fst.h"
#include "openfst/lib/mutable-fst.h"

namespace fst {
namespace {

// Names for each of the methods invoked on a custom arc type.
enum MethodName {
  CUSTOM_CTOR = 0,
  DEFAULT_CTOR = 1,
  COPY_CTOR = 2,
  MOVE_CTOR = 3,
  DTOR = 4,
  COPY_ASSIGN = 5,
  MOVE_ASSIGN = 6,
  NUM_METHOD_NAMES = 7,
};

// Helper for counting each of the method invocations on a custom arc type.
// Not thread-safe. Only for use in single-threaded tests.
class MethodCounters {
 public:
  int Get(MethodName key) const { return counter_.at(key); }

  void Increment(MethodName key) { counter_.at(key)++; }

  void Reset() { counter_.fill(0); }

  int CtorDtorBalance() {
    return counter_.at(CUSTOM_CTOR) + counter_.at(DEFAULT_CTOR) +
           counter_.at(COPY_CTOR) + counter_.at(MOVE_CTOR) - counter_.at(DTOR);
  }

 private:
  std::array<int, NUM_METHOD_NAMES> counter_{};
};

template <class B, class D>
using DisableDerived = typename std::enable_if_t<
    !std::is_base_of_v<B, typename std::remove_reference_t<D>>>;

// Class wrapper that injects instrumentation for counting calls to
// constructors, destructor, and assignment operators.
//
// Lightly customized to wrap the static Zero and One functions of FST weight
// types. This is fine when wrapping other types (such as arc types), because
// SFINAE.
//
// Not thread-safe. Only for use in single-threaded tests.
template <class T, int log_level>
class Instrumented : public T {
 public:
  static MethodCounters& Counters() {
    // Pointer cleanup policy: Never clean up.
    static MethodCounters* const counters = new MethodCounters();
    return *counters;
  }

  static const std::string& Type() {
    static const absl::NoDestructor<std::string> type([]() {
      std::string s("instrumented_");
      s.append(T::Type());
      return s;
    }());
    return *type;
  }

  void Log(absl::string_view msg) const {
    static const absl::NoDestructor<std::string> name([]() {
      std::string name("_Z");
      name.append(typeid(T).name());
      return name;
    }());
    VLOG(log_level) << *name << " " << msg << ": " << *this;
  }

  ~Instrumented() noexcept {
    Counters().Increment(DTOR);
    Log("destructor");
  }

  Instrumented() : T() {
    Counters().Increment(DEFAULT_CTOR);
    Log("default constructor");
  }

  // Constructor template deliberately left implicit, since it is intended
  // to wrap the implicit constructors of weights like TropicalWeight.
  //
  // Disabled for Arg instantiated to Instrumented or subtypes, to avoid
  // competing with the copy constructors during overload resolution.
  template <class Arg, class Dummy = DisableDerived<Instrumented, Arg>>
  Instrumented(Arg&& arg)
      : T(std::forward<Arg>(arg)) {
    Counters().Increment(CUSTOM_CTOR);
    Log("custom constructor, 1 argument");
  }

  template <class Arg1, class Arg2, class... Args>
  Instrumented(Arg1&& arg1, Arg2&& arg2, Args&&... args)
      : T(std::forward<Arg1>(arg1), std::forward<Arg2>(arg2),
          std::forward<Args>(args)...) {
    Counters().Increment(CUSTOM_CTOR);
    Log("custom constructor, 2+ arguments");
  }

  Instrumented(const Instrumented& t) : T(t) {
    Counters().Increment(COPY_CTOR);
    Log("copy constructor");
  }

  Instrumented(Instrumented&& t) noexcept : T(std::move(t)) {
    Counters().Increment(MOVE_CTOR);
    Log("move constructor");
  }

  Instrumented& operator=(const Instrumented& t) {
    T::operator=(t);
    Counters().Increment(COPY_ASSIGN);
    Log("copy assignment");
    return *this;
  }

  Instrumented& operator=(Instrumented&& t) noexcept {
    T::operator=(std::move(t));
    Counters().Increment(MOVE_ASSIGN);
    Log("move assignment");
    return *this;
  }

  static const Instrumented& Zero() {
    // Pointer cleanup policy: Never clean up.
    static const Instrumented* const zero = new Instrumented(T::Zero());
    return *zero;
  }

  static const Instrumented& One() {
    // Pointer cleanup policy: Never clean up.
    static const Instrumented* const one = new Instrumented(T::One());
    return *one;
  }
};

template <class W>
struct MyArcTpl {
  using Label = int;
  using Weight = W;
  using StateId = int;

  Label ilabel;
  Label olabel;
  Weight weight;
  StateId nextstate;

  static const std::string& Type() {
    static const absl::NoDestructor<std::string> type([]() {
      std::string s("my_arc_");
      s.append(W::Type());
      return s;
    }());
    return *type;
  }

  MyArcTpl() = default;

  template <class U>
  MyArcTpl(Label i, Label o, U&& w, StateId n)
      : ilabel(i), olabel(o), weight(std::forward<U>(w)), nextstate(n) {}
};

template <class W>
inline std::ostream& operator<<(std::ostream& strm, const MyArcTpl<W>& arc) {
  return strm << "Arc(Type=\"" << MyArcTpl<W>::Type() << "\", " << arc.ilabel
              << ", " << arc.olabel << ", " << arc.weight << ", "
              << arc.nextstate << ")";
}

class VectorFstTest : public ::testing::Test {
 protected:
  using MyWeight = Instrumented<TropicalWeight, 2>;

  using MyArc = Instrumented<MyArcTpl<MyWeight>, 1>;

  using MyVectorFst = VectorFst<MyArc>;

  using MyMutableFst = MutableFst<MyArc>;

  // Private implementation details of MyVectorFst.

  using MyState = VectorState<MyArc>;

  using MyImpl = internal::VectorFstImpl<MyState>;

  using MyImplToMutableFst = ImplToMutableFst<MyImpl, MyMutableFst>;

  using MyImplToExpandedFst = ImplToExpandedFst<MyImpl, MyMutableFst>;

  using MyImplToFst = ImplToFst<MyImpl, MyMutableFst>;

  // Check the class invariant: For each constructor call there is an equal and
  // opposite destructor call on the instrumented class types.
  static void CheckInvariant() {
    EXPECT_EQ(0, weight_counters_.CtorDtorBalance());
    EXPECT_EQ(0, arc_counters_.CtorDtorBalance());
  }

  // Exception to the class invariant: The objects representing Zero and One of
  // MyWeight are statically created and never cleaned up.
  static void SetUpTestSuite() {
    CheckInvariant();
    MyWeight::Zero();
    MyWeight::One();
    EXPECT_EQ(2, weight_counters_.CtorDtorBalance());
    weight_counters_.Reset();
    CheckInvariant();
  }

  void SetUp() override { CheckInvariant(); }

  void TearDown() override { CheckInvariant(); }

  static void TearDownTestSuite() { CheckInvariant(); }

  static MethodCounters& weight_counters_;
  static MethodCounters& arc_counters_;
};

MethodCounters& VectorFstTest::weight_counters_ = MyWeight::Counters();

MethodCounters& VectorFstTest::arc_counters_ = MyArc::Counters();

// Check that private implementation details are correctly exposed for testing
// by the type definitions above.
TEST_F(VectorFstTest, ImplementationDetails) {
  EXPECT_TRUE((std::is_same_v<MyVectorFst::State, MyState>));

  EXPECT_TRUE((std::is_base_of_v<MyMutableFst, MyImplToFst>));
  EXPECT_TRUE((std::is_base_of_v<MyImplToFst, MyImplToExpandedFst>));
  EXPECT_TRUE((std::is_base_of_v<MyImplToExpandedFst, MyImplToMutableFst>));
  EXPECT_TRUE((std::is_base_of_v<MyImplToMutableFst, MyVectorFst>));
}

// Check that auxiliary classes are concrete and movable.
TEST_F(VectorFstTest, AuxiliaryTypeTraits) {
  EXPECT_FALSE(std::is_abstract_v<MyWeight>);
  EXPECT_TRUE(std::is_nothrow_move_constructible_v<MyWeight>);
  EXPECT_TRUE(std::is_nothrow_move_assignable_v<MyWeight>);

  EXPECT_FALSE(std::is_abstract_v<MyArc>);
  EXPECT_TRUE(std::is_nothrow_move_constructible_v<MyArc>);
  EXPECT_TRUE(std::is_nothrow_move_assignable_v<MyArc>);

  EXPECT_FALSE(std::is_abstract_v<MyState>);
  EXPECT_TRUE(std::is_nothrow_move_constructible_v<MyState>);
  EXPECT_TRUE(std::is_nothrow_move_assignable_v<MyState>);
}

TEST_F(VectorFstTest, ImplTypeTraits) {
  EXPECT_FALSE(std::is_abstract_v<internal::FstImpl<MyArc>>);
  EXPECT_TRUE(std::is_nothrow_move_constructible_v<internal::FstImpl<MyArc>>);
  EXPECT_TRUE(std::is_nothrow_move_assignable_v<internal::FstImpl<MyArc>>);

  EXPECT_FALSE(std::is_abstract_v<internal::VectorFstBaseImpl<MyState>>);
  EXPECT_TRUE(std::is_nothrow_move_constructible_v<
              internal::VectorFstBaseImpl<MyState>>);
  EXPECT_TRUE(
      std::is_nothrow_move_assignable_v<internal::VectorFstBaseImpl<MyState>>);

  EXPECT_FALSE(std::is_abstract_v<internal::VectorFstImpl<MyState>>);
  EXPECT_TRUE(
      std::is_nothrow_move_constructible_v<internal::VectorFstImpl<MyState>>);
  EXPECT_TRUE(
      std::is_nothrow_move_assignable_v<internal::VectorFstImpl<MyState>>);
}

TEST_F(VectorFstTest, FstTypeTraits) {
  EXPECT_TRUE(std::is_abstract_v<MyMutableFst>);
  EXPECT_TRUE(std::is_abstract_v<MyImplToFst>);
  EXPECT_TRUE(std::is_abstract_v<MyImplToMutableFst>);

  EXPECT_FALSE(std::is_abstract_v<MyVectorFst>);
  EXPECT_TRUE(std::is_nothrow_move_constructible_v<MyVectorFst>);
  EXPECT_TRUE(std::is_nothrow_move_assignable_v<MyVectorFst>);
}

// Check that the copy constructor is called correctly for an instrumented arc.
TEST_F(VectorFstTest, InstrumentedArcCopyCtor) {
  const int custom_ctor_count = arc_counters_.Get(CUSTOM_CTOR);
  const int copy_ctor_count = arc_counters_.Get(COPY_CTOR);
  const int move_ctor_count = arc_counters_.Get(MOVE_CTOR);

  VLOG(1) << "Creating new arc:";
  MyArc arc1(1, 2, 3, 4);
  EXPECT_EQ(1 + custom_ctor_count, arc_counters_.Get(CUSTOM_CTOR));
  EXPECT_EQ(0 + copy_ctor_count, arc_counters_.Get(COPY_CTOR));
  EXPECT_EQ(0 + move_ctor_count, arc_counters_.Get(MOVE_CTOR));

  VLOG(1) << "Copying arc:";
  MyArc arc2 = arc1;
  EXPECT_EQ(1 + custom_ctor_count, arc_counters_.Get(CUSTOM_CTOR));
  EXPECT_EQ(1 + copy_ctor_count, arc_counters_.Get(COPY_CTOR));
  EXPECT_EQ(0 + move_ctor_count, arc_counters_.Get(MOVE_CTOR));
}

// Check that the move constructor for Impl is called correctly.
TEST_F(VectorFstTest, InstrumentedImplMove) {
  constexpr int num_states = 10;

  const int custom_ctor_count = arc_counters_.Get(CUSTOM_CTOR);
  const int copy_ctor_count = arc_counters_.Get(COPY_CTOR);
  const int move_ctor_count = arc_counters_.Get(MOVE_CTOR);
  const int dtor_count = arc_counters_.Get(DTOR);
  const int move_assign_count = arc_counters_.Get(MOVE_ASSIGN);

  // We'll be creating two fsts.
  const int expected_custom_ctor_count =
      custom_ctor_count + 2 * num_states * num_states;
  {
    MyImpl fst1;
    fst1.ReserveStates(num_states);
    VLOG(1) << "Adding states:";
    for (int i = 0; i < num_states; ++i) {
      fst1.AddState();
    }
    fst1.SetStart(0);
    EXPECT_EQ(num_states, fst1.NumStates());

    VLOG(1) << "Adding arcs:";
    for (int src = 0; src < num_states; ++src) {
      fst1.ReserveArcs(src, num_states);
      for (int tgt = 0; tgt < num_states; ++tgt) {
        fst1.EmplaceArc(src, src, tgt, 42, tgt);
      }
    }

    VLOG(1) << "Constructing second FST:";
    MyImpl fst2;
    fst2.ReserveStates(num_states);
    VLOG(1) << "Adding states:";
    for (int i = 0; i < num_states; ++i) {
      fst2.AddState();
    }
    fst2.SetStart(0);
    EXPECT_EQ(num_states, fst2.NumStates());

    VLOG(1) << "Adding arcs:";
    for (int src = 0; src < num_states; ++src) {
      fst2.ReserveArcs(src, num_states);
      for (int tgt = 0; tgt < num_states; ++tgt) {
        fst2.EmplaceArc(src, src, tgt, 42, tgt);
      }
    }
    EXPECT_EQ(expected_custom_ctor_count, arc_counters_.Get(CUSTOM_CTOR));
    EXPECT_EQ(copy_ctor_count, arc_counters_.Get(COPY_CTOR));
    EXPECT_EQ(0 + move_ctor_count, arc_counters_.Get(MOVE_CTOR));
    EXPECT_EQ(0 + dtor_count, arc_counters_.Get(DTOR));
    EXPECT_EQ(0 + move_assign_count, arc_counters_.Get(MOVE_ASSIGN));

    VLOG(1) << "Move-constructing second FST into first";
    fst1 = std::move(fst2);
    EXPECT_EQ(copy_ctor_count, arc_counters_.Get(COPY_CTOR));
    // std::move of fst just moves the poiners, not the arcs themselves.
    EXPECT_EQ(move_assign_count, arc_counters_.Get(MOVE_ASSIGN));
    // Arcs in fst1 are cleared in the move.
    EXPECT_EQ(num_states * num_states + dtor_count, arc_counters_.Get(DTOR));
  }
}

// Check that the move constructor is called correctly for an instrumented arc.
TEST_F(VectorFstTest, InstrumentedArcMoveCtor) {
  const int custom_ctor_count = arc_counters_.Get(CUSTOM_CTOR);
  const int copy_ctor_count = arc_counters_.Get(COPY_CTOR);
  const int move_ctor_count = arc_counters_.Get(MOVE_CTOR);

  VLOG(1) << "Creating new arc:";
  MyArc arc1(1, 2, 3, 4);
  EXPECT_EQ(1 + custom_ctor_count, arc_counters_.Get(CUSTOM_CTOR));
  EXPECT_EQ(0 + copy_ctor_count, arc_counters_.Get(COPY_CTOR));
  EXPECT_EQ(0 + move_ctor_count, arc_counters_.Get(MOVE_CTOR));

  VLOG(1) << "Moving arc:";
  MyArc arc2 = std::move(arc1);
  EXPECT_EQ(1 + custom_ctor_count, arc_counters_.Get(CUSTOM_CTOR));
  EXPECT_EQ(0 + copy_ctor_count, arc_counters_.Get(COPY_CTOR));
  EXPECT_EQ(1 + move_ctor_count, arc_counters_.Get(MOVE_CTOR));
}

// Check that calling VectorFst::AddArc() copy-constructs an arc passed by
// lvalue reference to const.
TEST_F(VectorFstTest, AddArcByLvalueConstRef) {
  const int custom_ctor_count = arc_counters_.Get(CUSTOM_CTOR);
  const int copy_ctor_count = arc_counters_.Get(COPY_CTOR);
  const int move_ctor_count = arc_counters_.Get(MOVE_CTOR);
  const int dtor_count = arc_counters_.Get(DTOR);
  const int assign_count =
      arc_counters_.Get(COPY_ASSIGN) + arc_counters_.Get(MOVE_ASSIGN);
  {
    MyVectorFst fst;
    VLOG(1) << "Calling AddState():";
    const auto state = fst.AddState();
    EXPECT_EQ(1, fst.NumStates());

    // This accounts for the lone custom constructor call below:
    VLOG(1) << "Calling MyArc custom constructor:";
    const MyArc arc(1, 1, 2, state);

    // The call to AddArc() will copy-construct a new arc object:
    VLOG(1) << "Calling AddArc(const&):";
    fst.AddArc(state, arc);
    EXPECT_EQ(1, fst.NumArcs(state));
  }
  EXPECT_EQ(1 + custom_ctor_count, arc_counters_.Get(CUSTOM_CTOR));
  EXPECT_EQ(1 + copy_ctor_count, arc_counters_.Get(COPY_CTOR));
  EXPECT_EQ(0 + move_ctor_count, arc_counters_.Get(MOVE_CTOR));
  EXPECT_EQ(2 + dtor_count, arc_counters_.Get(DTOR));
  EXPECT_EQ(0 + assign_count,
            arc_counters_.Get(COPY_ASSIGN) + arc_counters_.Get(MOVE_ASSIGN));
}

// Check that calling VectorFst::AddArc() copy-constructs an arc passed by
// lvalue reference.
TEST_F(VectorFstTest, AddArcByLvalueRef) {
  const int custom_ctor_count = arc_counters_.Get(CUSTOM_CTOR);
  const int copy_ctor_count = arc_counters_.Get(COPY_CTOR);
  const int move_ctor_count = arc_counters_.Get(MOVE_CTOR);
  const int dtor_count = arc_counters_.Get(DTOR);
  const int assign_count =
      arc_counters_.Get(COPY_ASSIGN) + arc_counters_.Get(MOVE_ASSIGN);
  {
    MyVectorFst fst;
    VLOG(1) << "Calling AddState():";
    const auto state = fst.AddState();
    EXPECT_EQ(1, fst.NumStates());

    // This accounts for the lone custom constructor call below:
    VLOG(1) << "Calling MyArc custom constructor:";
    MyArc arc(1, 1, 2, state);

    // The call to AddArc() will copy-construct a new arc object:
    VLOG(1) << "Calling AddArc(&):";
    fst.AddArc(state, arc);
    EXPECT_EQ(1, fst.NumArcs(state));
  }
  EXPECT_EQ(1 + custom_ctor_count, arc_counters_.Get(CUSTOM_CTOR));
  EXPECT_EQ(1 + copy_ctor_count, arc_counters_.Get(COPY_CTOR));
  EXPECT_EQ(0 + move_ctor_count, arc_counters_.Get(MOVE_CTOR));
  EXPECT_EQ(2 + dtor_count, arc_counters_.Get(DTOR));
  EXPECT_EQ(0 + assign_count,
            arc_counters_.Get(COPY_ASSIGN) + arc_counters_.Get(MOVE_ASSIGN));
}

// Check that calling VectorFst::AddArc() move-constructs an arc passed by
// rvalue reference.
TEST_F(VectorFstTest, AddArcByRvalueRef) {
  const int custom_ctor_count = arc_counters_.Get(CUSTOM_CTOR);
  const int copy_ctor_count = arc_counters_.Get(COPY_CTOR);
  const int move_ctor_count = arc_counters_.Get(MOVE_CTOR);
  const int dtor_count = arc_counters_.Get(DTOR);
  const int assign_count =
      arc_counters_.Get(COPY_ASSIGN) + arc_counters_.Get(MOVE_ASSIGN);
  {
    MyVectorFst fst;
    VLOG(1) << "Calling AddState():";
    const auto state = fst.AddState();
    EXPECT_EQ(1, fst.NumStates());

    // The call to AddArc invokes the 4-argument custom constructor to create a
    // temporary, which is then move-constructed into the underlying storage,
    // after which the temporary is destroyed.
    VLOG(1) << "Calling AddArc(&&):";
    fst.AddArc(state, MyArc(1, 1, 2, state));
    EXPECT_EQ(1 + dtor_count, arc_counters_.Get(DTOR));
    EXPECT_EQ(1, fst.NumArcs(state));
  }
  EXPECT_EQ(1 + custom_ctor_count, arc_counters_.Get(CUSTOM_CTOR));
  EXPECT_EQ(0 + copy_ctor_count, arc_counters_.Get(COPY_CTOR));
  EXPECT_EQ(1 + move_ctor_count, arc_counters_.Get(MOVE_CTOR));
  EXPECT_EQ(2 + dtor_count, arc_counters_.Get(DTOR));
  EXPECT_EQ(0 + assign_count,
            arc_counters_.Get(COPY_ASSIGN) + arc_counters_.Get(MOVE_ASSIGN));
}

// Check that calling VectorFst::EmplaceArc() constructs an arc in place by
// perfect forwarding to its custom constructor.
TEST_F(VectorFstTest, EmplaceArc) {
  const int custom_ctor_count = arc_counters_.Get(CUSTOM_CTOR);
  const int copy_ctor_count = arc_counters_.Get(COPY_CTOR);
  const int move_ctor_count = arc_counters_.Get(MOVE_CTOR);
  const int dtor_count = arc_counters_.Get(DTOR);
  const int assign_count =
      arc_counters_.Get(COPY_ASSIGN) + arc_counters_.Get(MOVE_ASSIGN);
  {
    MyVectorFst fst;
    VLOG(1) << "Calling AddState():";
    const auto state = fst.AddState();
    EXPECT_EQ(1, fst.NumStates());

    // The call to EmplaceArc forwards the arguments after 'state' to the arc's
    // custom ctor. No temporaries are created, no other constructors called.
    VLOG(1) << "Calling EmplaceArc():";
    fst.EmplaceArc(state, 1, 1, 2, state);
    EXPECT_EQ(1 + custom_ctor_count, arc_counters_.Get(CUSTOM_CTOR));
    EXPECT_EQ(0 + dtor_count, arc_counters_.Get(DTOR));
    EXPECT_EQ(1, fst.NumArcs(state));
  }
  EXPECT_EQ(1 + custom_ctor_count, arc_counters_.Get(CUSTOM_CTOR));
  EXPECT_EQ(0 + copy_ctor_count, arc_counters_.Get(COPY_CTOR));
  EXPECT_EQ(0 + move_ctor_count, arc_counters_.Get(MOVE_CTOR));
  EXPECT_EQ(1 + dtor_count, arc_counters_.Get(DTOR));
  EXPECT_EQ(0 + assign_count,
            arc_counters_.Get(COPY_ASSIGN) + arc_counters_.Get(MOVE_ASSIGN));
}

TEST_F(VectorFstTest, VectorFstMoveOperations) {
  constexpr int num_states = 10;

  const int custom_ctor_count = arc_counters_.Get(CUSTOM_CTOR);
  const int copy_ctor_count = arc_counters_.Get(COPY_CTOR);
  const int move_ctor_count = arc_counters_.Get(MOVE_CTOR);
  const int dtor_count = arc_counters_.Get(DTOR);
  const int assign_count =
      arc_counters_.Get(COPY_ASSIGN) + arc_counters_.Get(MOVE_ASSIGN);

  const int expected_custom_ctor_count =
      custom_ctor_count + num_states * num_states;
  {
    MyVectorFst fst1;
    fst1.ReserveStates(num_states);
    VLOG(1) << "Adding states:";
    for (int i = 0; i < num_states; ++i) {
      fst1.AddState();
    }
    fst1.SetStart(0);
    EXPECT_EQ(num_states, fst1.NumStates());

    VLOG(1) << "Adding arcs:";
    for (int src = 0; src < num_states; ++src) {
      fst1.ReserveArcs(src, num_states);
      for (int tgt = 0; tgt < num_states; ++tgt) {
        fst1.EmplaceArc(src, src, tgt, 42, tgt);
      }
    }
    EXPECT_EQ(expected_custom_ctor_count, arc_counters_.Get(CUSTOM_CTOR));
    EXPECT_EQ(0 + copy_ctor_count, arc_counters_.Get(COPY_CTOR));
    EXPECT_EQ(0 + move_ctor_count, arc_counters_.Get(MOVE_CTOR));
    EXPECT_EQ(0 + dtor_count, arc_counters_.Get(DTOR));

    VLOG(1) << "Move-constructing second FST:";
    MyVectorFst fst2 = std::move(fst1);
    EXPECT_EQ(0, fst1.NumStates());
    EXPECT_EQ(num_states, fst2.NumStates());
    // Number of arcs constructed is the same, since only the impl is moved.
    EXPECT_EQ(expected_custom_ctor_count, arc_counters_.Get(CUSTOM_CTOR));
    EXPECT_EQ(0 + copy_ctor_count, arc_counters_.Get(COPY_CTOR));
    EXPECT_EQ(0 + move_ctor_count, arc_counters_.Get(MOVE_CTOR));
    EXPECT_EQ(0 + dtor_count, arc_counters_.Get(DTOR));

    fst1 = MyVectorFst();
    fst1.ReserveStates(num_states);
    VLOG(1) << "Adding states to FST1 again:";
    for (int i = 0; i < num_states; ++i) {
      fst1.AddState();
    }
    fst1.SetStart(0);
    EXPECT_EQ(num_states, fst1.NumStates());
    VLOG(1) << "Adding arcs:";
    for (int src = 0; src < num_states; ++src) {
      fst1.ReserveArcs(src, num_states);
      for (int tgt = 0; tgt < num_states; ++tgt) {
        fst1.EmplaceArc(src, src, tgt, 42, tgt);
      }
    }
    VLOG(1) << "Move-assigning to first FST, erasing its states:";
    fst1 = std::move(fst2);
    EXPECT_EQ(num_states, fst1.NumStates());
    EXPECT_EQ(0, fst2.NumStates());
    // We recreated fst1, so count those arc creations; but then we moved those
    // so no additional creations or copies occurred.
    EXPECT_EQ(num_states * num_states + expected_custom_ctor_count,
              arc_counters_.Get(CUSTOM_CTOR));
    EXPECT_EQ(0 + copy_ctor_count, arc_counters_.Get(COPY_CTOR));
    EXPECT_EQ(0 + move_ctor_count, arc_counters_.Get(MOVE_CTOR));
    // When we moved fst2 to fst1, fst1's arcs were deleted; count these.
    EXPECT_EQ(num_states * num_states + dtor_count, arc_counters_.Get(DTOR));

    VLOG(1) << "Destroying FSTs:";
  }
  EXPECT_EQ(num_states * num_states + expected_custom_ctor_count,
            arc_counters_.Get(CUSTOM_CTOR));
  EXPECT_EQ(0 + copy_ctor_count, arc_counters_.Get(COPY_CTOR));
  EXPECT_EQ(0 + move_ctor_count, arc_counters_.Get(MOVE_CTOR));
  EXPECT_EQ(0 + assign_count,
            arc_counters_.Get(COPY_ASSIGN) + arc_counters_.Get(MOVE_ASSIGN));
}

// Check that AddStates gives the proper state count.
TEST(VectorFstAddStates, VectorFstAddStates) {
  constexpr int kNumStates = 10;
  StdVectorFst fst;
  fst.AddStates(kNumStates);
  // Asks it how many states it thinks it has.
  EXPECT_EQ(kNumStates, fst.NumStates());
  // Counts them via iteration.
  int count = 0;
  for (StateIterator<StdVectorFst> siter(fst); !siter.Done(); siter.Next()) {
    ++count;
  }
  EXPECT_EQ(kNumStates, count);
}

}  // namespace
}  // namespace fst
