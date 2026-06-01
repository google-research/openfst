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

// Tests fast math functions for exp2, log2 to verify they do not
// introduce too large an error.

#include "openfst/compat/fastmath.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <iostream>
#include <limits>
#include <ostream>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/log/check.h"
#include "absl/log/log.h"

namespace {

using ::testing::ElementsAreArray;
using ::testing::FloatEq;
using ::testing::Pointwise;

// Constants used in timing tests:

// Increase kIterations for more accurate results.
// Decrease kIterations for a quicker test.
static const int kIterations = 10000000;

// Timing tests inputs from 0 to kStepSize[f]*kIterations
static const double kStepSize = 10. / kIterations;
static const float kStepSizef = 10. / kIterations;

// Multiply WallTimer times by kTimerToNS to get ns of each iteration.
static const double kTimerToNS = 1000000000. / kIterations;

// If two (usually floating point) numbers are within a certain
// absolute margin of error.
template <typename T>
static bool WithinMargin(const T x, const T y, const T margin) {
  DCHECK_GE(margin, 0);
  // this is a little faster than x <= y + margin  &&  x >= y - margin
  return std::abs(x - y) <= margin;
}

// Custom two-argument integer matcher so that we can use it with
// Pointwise.
MATCHER(IntegerWithinOneOf, "") {
  return WithinMargin(std::get<0>(arg), std::get<1>(arg), 1);
}

TEST(FastMathTest, Constants) {
  EXPECT_EQ(static_cast<float>(std::log(2.0)), FASTMATH_LOG_2_F);
  EXPECT_EQ(static_cast<double>(std::log(2.0)), FASTMATH_LOG_2_D);
  EXPECT_EQ(static_cast<float>(1.0 / std::log(2.0)), FASTMATH_INV_LOG_2_F);
  EXPECT_EQ(static_cast<double>(1.0 / std::log(2.0)), FASTMATH_INV_LOG_2_D);
}

TEST(FastMathTest, VerifyTableFloat) {
  auto a = FastMathClass::InternalTestAccess::actual();
  auto b = FastMathClass::InternalTestAccess::expected();
  EXPECT_THAT(a.log, ElementsAreArray(b.log));
  EXPECT_THAT(a.log_diff, ElementsAreArray(b.log_diff));
  EXPECT_THAT(a.exp1, Pointwise(IntegerWithinOneOf(), b.exp1));
  EXPECT_THAT(a.exp2, Pointwise(FloatEq(), b.exp2));
}

// FloatError
//   Check for correctness of single prescision fast math functions.

TEST(FastMathTest, FloatError) {
  double fe_max_err = 0;
  double fl_max_err = 0;
  double deriv_max_err = 0;

  LOG(INFO) << "float correctness test";

  for (double d = -125.9; d < 125.9; d += .13124235) {
    double e = std::exp2(d);
    double fe = fexp2(d);
    double fl = flog2(fe);
    fe_max_err = std::max(fe_max_err, std::fabs((fe / e) - 1));
    fl_max_err =
        std::max(fl_max_err, std::fabs(fl - std::log(fe) / std::log(2.0)));
    deriv_max_err = std::max(
        deriv_max_err,
        std::fabs(((flog2(fexp2(d + 0.01)) - flog2(fexp2(d))) - .01) / .01));
  }
  LOG(INFO) << " fe_max_err: " << fe_max_err;
  LOG(INFO) << " fl_max_err: " << fl_max_err;
  LOG(INFO) << " deriv_max_err: " << deriv_max_err;
  EXPECT_LT(fe_max_err, 2e-5);
  EXPECT_LT(fl_max_err, 2e-5);
  EXPECT_LT(deriv_max_err, 2e-2);
  EXPECT_LT(std::fabs(fexp(10) / std::exp(10) - 1.0), 0.01);
  EXPECT_LT(std::fabs(flog(10) / std::log(10) - 1.0), 0.01);
}

// DoubleError
//   Check for correctness of double prescision fast math functions.

TEST(FastMathTest, DoubleError) {
  double fe_max_err = 0;
  double fl_max_err = 0;
  double deriv_max_err = 0;

  LOG(INFO) << "double correctness test";

  for (double d = -1021.9; d < 1021.9; d += .13124235) {
    double e = std::exp2(d);
    double fe = fexp2d(d);
    double fl = flog2d(fe);
    fe_max_err = std::max(fe_max_err, std::fabs((fe / e) - 1));
    fl_max_err = std::max(fl_max_err, std::fabs(fl - d));
    deriv_max_err = std::max(
        deriv_max_err,
        std::fabs(((flog2d(fexp2d(d + 0.01)) - flog2d(fexp2d(d))) - .01) /
                  .01));
    EXPECT_TRUE((fe_max_err <= 2e-5) && (fl_max_err <= 2e-5))
        << " d=" << d << " e=" << e << " fe=" << fe
        << " le=" << std::log(fe) / std::log(2.0) << " fl=" << fl
        << " fe_max_err=" << fe_max_err;
  }
  LOG(INFO) << " fe_max_err: " << fe_max_err;
  LOG(INFO) << " fl_max_err: " << fl_max_err;
  LOG(INFO) << " deriv_max_err: " << deriv_max_err;
  EXPECT_LT(fe_max_err, 2e-5);
  EXPECT_LT(fl_max_err, 2e-5);
  EXPECT_LT(deriv_max_err, 2e-2);
  EXPECT_LT(std::fabs(fexpd(10) / std::exp(10) - 1.0), 0.01);
  EXPECT_LT(std::fabs(flogd(10) / std::log(10) - 1.0), 0.01);
}

}  // namespace
