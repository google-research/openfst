
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
//
// Copyright 2005-2010 Google, Inc.
// Author: krr@google.com (Kasturi Rangan Raghavan)
// \file
// LogWeight along with sign information that represents the value X in the
// linear domain as <sign(X), -ln(|X|)>
// The sign is a TropicalWeight:
//  positive, TropicalWeight.Value() > 0.0, recommended value 1.0
//  negative, TropicalWeight.Value() <= 0.0, recommended value -1.0

#ifndef FST_LIB_SIGNED_LOG_WEIGHT_H_
#define FST_LIB_SIGNED_LOG_WEIGHT_H_

#include <fst/float-weight.h>
#include <fst/pair-weight.h>

namespace fst {
  template <class T>
  class SignedLogWeightTpl
    : public PairWeight<TropicalWeight, LogWeightTpl<T> > {
   public:
    typedef TropicalWeight X1;
    typedef LogWeightTpl<T> X2;
    using PairWeight<X1, X2>::Value1;
    using PairWeight<X1, X2>::Value2;

    using PairWeight<X1, X2>::Reverse;
    using PairWeight<X1, X2>::Quantize;
    using PairWeight<X1, X2>::Member;

    typedef SignedLogWeightTpl<T> ReverseWeight;

    SignedLogWeightTpl() : PairWeight<X1, X2>() {}

    SignedLogWeightTpl(const SignedLogWeightTpl<T>& w)
        : PairWeight<X1, X2> (w) { }

    SignedLogWeightTpl(const PairWeight<X1, X2>& w)
        : PairWeight<X1, X2> (w) { }

    SignedLogWeightTpl(const X1& x1, const X2& x2)
        : PairWeight<X1, X2>(x1, x2) { }

    static const SignedLogWeightTpl<T> &Zero() {
      static const SignedLogWeightTpl<T> zero(X1(1.0), X2::Zero());
      return zero;
    }

    static const SignedLogWeightTpl<T> &One() {
      static const SignedLogWeightTpl<T> one(X1(1.0), X2::One());
      return one;
    }

    static const string &Type() {
      static const string type = "signed_log_" + X1::Type() + "_" + X2::Type();
      return type;
    }

    ProductWeight<X1, X2> Quantize(float delta = kDelta) const {
      return PairWeight<X1, X2>::Quantize();
    }

    ReverseWeight Reverse() const {
      return PairWeight<X1, X2>::Reverse();
    }

    bool Member() const {
      return PairWeight<X1, X2>::Member();
    }

    static uint64 Properties() {
      // not idempotent nor path
      return kLeftSemiring | kRightSemiring | kCommutative;
    }

    size_t Hash() const {
      size_t h1;
      if (Value2() == LogWeight::Zero() || Value1().Value() > 0.0)
        h1 = TropicalWeight(1.0).Hash();
      else
        h1 = TropicalWeight(-1.0).Hash();
      size_t h2 = Value2().Hash();
      const int lshift = 5;
      const int rshift = CHAR_BIT * sizeof(size_t) - 5;
      return h1 << lshift ^ h1 >> rshift ^ h2;
    }
  };

  template <class T>
  inline SignedLogWeightTpl<T> Plus(const SignedLogWeightTpl<T> &w1,
                                    const SignedLogWeightTpl<T> &w2) {
    bool s1 = w1.Value1().Value() > 0.0;
    bool s2 = w2.Value1().Value() > 0.0;
    T f1 = w1.Value2().Value();
    T f2 = w2.Value2().Value();
    if (f1 == FloatLimits<T>::kPosInfinity)
      return w2;
    else if (f2 == FloatLimits<T>::kPosInfinity)
      return w1;
    else if (f1 == f2) {
      if (s1 == s2)
        return SignedLogWeightTpl<T>(w1.Value1(), (f2 - log(2.0F)));
      else
        return SignedLogWeightTpl<T>::Zero();
    } else if (f1 > f2) {
      if (s1 == s2) {
        return SignedLogWeightTpl<T>(
          w1.Value1(), (f2 - log(1.0F + exp(f2 - f1))));
      } else {
        return SignedLogWeightTpl<T>(
          w2.Value1(), (f2 - log(1.0F - exp(f2 - f1))));
      }
    } else {
      if (s2 == s1) {
        return SignedLogWeightTpl<T>(
          w2.Value1(), (f1 - log(1.0F + exp(f1 - f2))));
      } else {
        return SignedLogWeightTpl<T>(
          w1.Value1(), (f1 - log(1.0F - exp(f1 - f2))));
      }
    }
  }

  template <class T>
  inline SignedLogWeightTpl<T> Times(const SignedLogWeightTpl<T> &w1,
                                     const SignedLogWeightTpl<T> &w2) {
    bool s1 = w1.Value1().Value() > 0.0;
    bool s2 = w2.Value1().Value() > 0.0;
    T f1 = w1.Value2().Value();
    T f2 = w2.Value2().Value();
    if (s1 == s2)
      return SignedLogWeightTpl<T>(TropicalWeight(1.0), (f1 + f2));
    else
      return SignedLogWeightTpl<T>(TropicalWeight(-1.0), (f1 + f2));
  }

  template <class T>
  inline SignedLogWeightTpl<T> Divide(const SignedLogWeightTpl<T> &w1,
                                      const SignedLogWeightTpl<T> &w2,
                                      DivideType typ = DIVIDE_ANY) {
    bool s1 = w1.Value1().Value() > 0.0;
    bool s2 = w2.Value1().Value() > 0.0;
    T f1 = w1.Value2().Value();
    T f2 = w2.Value2().Value();
    if (f2 == FloatLimits<T>::kPosInfinity)
      return SignedLogWeightTpl<T>(TropicalWeight(1.0),
        FloatLimits<T>::kNumberBad);
    else if (f1 == FloatLimits<T>::kPosInfinity)
      return SignedLogWeightTpl<T>(TropicalWeight(1.0),
        FloatLimits<T>::kPosInfinity);
    else if (s1 == s2)
      return SignedLogWeightTpl<T>(TropicalWeight(1.0), (f1 - f2));
    else
      return SignedLogWeightTpl<T>(TropicalWeight(-1.0), (f1 - f2));
  }

  template <class T>
  inline bool ApproxEqual(const SignedLogWeightTpl<T> &w1,
                          const SignedLogWeightTpl<T> &w2,
                          float delta = kDelta) {
    bool s1 = w1.Value1().Value() > 0.0;
    bool s2 = w2.Value1().Value() > 0.0;
    if (s1 == s2)
      return ApproxEqual(w1.Value2(), w2.Value2(), kDelta);
    else
      return w1.Value2() == LogWeightTpl<T>::Zero() &&
             w2.Value2() == LogWeightTpl<T>::Zero();
  }

  template <class T>
  inline bool operator==(const SignedLogWeightTpl<T> &w1,
                         const SignedLogWeightTpl<T> &w2) {
    bool s1 = w1.Value1().Value() > 0.0;
    bool s2 = w2.Value1().Value() > 0.0;
    if (s1 == s2)
      return w1.Value2() == w2.Value2();
    else
      return (w1.Value2() == LogWeightTpl<T>::Zero()) &&
             (w2.Value2() == LogWeightTpl<T>::Zero());
  }

  typedef SignedLogWeightTpl<float> SignedLogWeight;
}  // fst

#endif  // FST_LIB_SIGNED_LOG_WEIGHT_H_
