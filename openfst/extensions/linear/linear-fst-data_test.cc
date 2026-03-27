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
// TODO: clean up test; separate test logic from data

#include "openfst/extensions/linear/linear-fst-data.h"

#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/log/log.h"
#include "openfst/extensions/linear/linear-fst-data-builder.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/float-weight.h"
#include "openfst/lib/fst.h"

using ::testing::Contains;
using ::testing::ElementsAre;
using ::testing::Not;
using ::testing::Test;

namespace fst {

typedef StdArc::Label Label;
typedef StdArc::StateId StateId;
typedef StdArc::Weight Weight;
typedef LinearFstData<StdArc> StdLinearFstData;
typedef LinearFstDataBuilder<StdArc> StdLinearFstDataBuilder;
typedef LinearClassifierFstDataBuilder<StdArc>
    StdLinearClassifierFstDataBuilder;

class LinearFstDataTest : public Test {
 protected:
  static constexpr int kNumWords = 3;

  void AddWords(StdLinearFstDataBuilder* builder) {
    for (Label ilabel = 1; ilabel <= kNumWords; ++ilabel) {
      std::vector<Label> feature(1, ilabel + kNumWords);
      builder->AddWord(ilabel, feature);
    }
  }

  void AddConstrainedWords(StdLinearFstDataBuilder* builder) {
    for (Label ilabel = 1; ilabel <= kNumWords; ++ilabel) {
      std::vector<Label> feature(1, ilabel + kNumWords), possible;
      for (Label olabel = ilabel; olabel <= kNumWords; ++olabel)
        possible.push_back(olabel);
      builder->AddWord(ilabel, feature, possible);
    }
  }

  Weight UnigramFeature(Label ilabel, Label olabel) {
    if ((ilabel + olabel) % 2)
      return Weight(-(ilabel + olabel + 1));
    else
      return Weight::One();
  }

  Weight StartUnigramFeature(Label ilabel, Label olabel, int power = 1) {
    Weight base;
    if ((ilabel + olabel) % 2)
      base = Weight::One();
    else
      base = Weight(-(ilabel + olabel + 1));
    Weight ret = Weight::One();
    while (power-- > 0) ret = Times(ret, base);
    return ret;
  }

  Weight EndUnigramFeature(Label ilabel, Label olabel, int power = 1) {
    Weight base = UnigramFeature(ilabel, olabel);
    Weight ret = Weight::One();
    while (power-- > 0) ret = Times(ret, base);
    return ret;
  }

  Weight BigramFeature(Label ilabel1, Label ilabel2, Label olabel) {
    if ((ilabel1 + ilabel2 + olabel) % 2)
      return Weight(-(ilabel1 + ilabel2 + olabel));
    else
      return Weight::One();
  }

  Weight TransitionFeature(Label olabel1, Label olabel2) {
    if ((olabel1 + olabel2) % 2)
      return Weight(-(olabel2 + olabel1));
    else
      return Weight::One();
  }

  void AddUnigramFeatures(size_t group, StdLinearFstDataBuilder* builder) {
    for (Label ilabel = 1; ilabel <= kNumWords; ++ilabel) {
      std::vector<Label> feature(1, ilabel + kNumWords);
      for (Label olabel = 1; olabel <= kNumWords; ++olabel) {
        Weight weight = UnigramFeature(ilabel, olabel);
        if (weight != Weight::One()) {
          std::vector<Label> output(1, olabel);
          builder->AddWeight(group, feature, output, weight);
        }
      }
    }
  }

  void AddStartUnigramFeatures(size_t group, StdLinearFstDataBuilder* builder) {
    for (Label ilabel = 1; ilabel <= kNumWords; ++ilabel) {
      // x
      std::vector<Label> feature0(1, ilabel + kNumWords);
      // <s> x
      std::vector<Label> feature1(2, ilabel + kNumWords);
      // <s> <s> x
      std::vector<Label> feature2(3, ilabel + kNumWords);
      feature1[0] = StdLinearFstData::kStartOfSentence;
      feature2[0] = feature2[1] = StdLinearFstData::kStartOfSentence;
      for (Label olabel = 1; olabel <= kNumWords; ++olabel) {
        Weight weight = StartUnigramFeature(ilabel, olabel);
        if (weight != Weight::One()) {
          // y
          std::vector<Label> output0(1, olabel);
          // <s> y
          std::vector<Label> output1(2, olabel);
          output1[0] = StdLinearFstData::kStartOfSentence;

          builder->AddWeight(group, feature0, output1, weight);
          builder->AddWeight(group, feature1, output0, weight);
          builder->AddWeight(group, feature1, output1, weight);
          builder->AddWeight(group, feature2, output0, weight);
          builder->AddWeight(group, feature2, output1, weight);
        }
      }
    }
  }

  void AddEndUnigramFeatures(size_t group, size_t future_size,
                             StdLinearFstDataBuilder* builder) {
    for (Label ilabel = 1; ilabel <= kNumWords; ++ilabel) {
      // x . </s> ...
      std::vector<Label> feature0(1 + future_size,
                                  StdLinearFstData::kEndOfSentence);
      // x </s> . </s> ...
      std::vector<Label> feature1(2 + future_size,
                                  StdLinearFstData::kEndOfSentence);
      feature0[0] = feature1[0] = ilabel + kNumWords;
      for (Label olabel = 1; olabel <= kNumWords; ++olabel) {
        Weight weight = EndUnigramFeature(ilabel, olabel);
        if (weight != Weight::One()) {
          // y
          std::vector<Label> output0(1, olabel);
          // y </s>
          std::vector<Label> output1(2, olabel);
          output1[1] = StdLinearFstData::kEndOfSentence;

          if (future_size > 0)
            builder->AddWeight(group, feature0, output0, weight);
          builder->AddWeight(group, feature1, output1, weight);
        }
      }
    }
  }

  void AddBigramFeatures(size_t group, StdLinearFstDataBuilder* builder) {
    for (Label ilabel1 = 1; ilabel1 <= kNumWords; ++ilabel1) {
      for (Label ilabel2 = ilabel1; ilabel2 <= kNumWords; ++ilabel2) {
        for (Label olabel = 1; olabel <= kNumWords; ++olabel) {
          Weight weight = BigramFeature(ilabel1, ilabel2, olabel);
          if (weight != Weight::One()) {
            std::vector<Label> feature, output;
            feature.push_back(ilabel1 + kNumWords);
            feature.push_back(ilabel2 + kNumWords);
            output.push_back(olabel);
            builder->AddWeight(group, feature, output, weight);
          }
        }
      }
    }
  }

  void AddTransitionFeatures(size_t group, StdLinearFstDataBuilder* builder) {
    for (Label olabel1 = 1; olabel1 <= kNumWords; ++olabel1) {
      for (Label olabel2 = olabel1; olabel2 <= kNumWords; ++olabel2) {
        Weight weight = TransitionFeature(olabel1, olabel2);
        if (weight != Weight::One()) {
          std::vector<Label> feature, output;
          output.push_back(olabel1);
          output.push_back(olabel2);
          builder->AddWeight(group, feature, output, weight);
        }
      }
    }
  }

  Label In(Label l) {
    if (l == 0) return StdLinearFstData::kEndOfSentence;
    return l;
  }

  Label Out(Label l) {
    if (l == 0) return StdLinearFstData::kStartOfSentence;
    return l;
  }
};

TEST_F(LinearFstDataTest, UnigramNoDelay) {
  StdLinearFstDataBuilder builder;
  int group = builder.AddGroup(0);
  ASSERT_GE(group, 0);
  AddWords(&builder);
  AddUnigramFeatures(group, &builder);

  std::unique_ptr<const StdLinearFstData> data_ptr(builder.Dump());
  const StdLinearFstData& data = *data_ptr;

  // Simple properties
  {
    EXPECT_EQ(1, data.MinInputLabel());
    EXPECT_EQ(kNumWords, data.MaxInputLabel());
    EXPECT_EQ(0, data.MaxFutureSize());
    for (Label ilabel = 1; ilabel <= kNumWords; ++ilabel) {
      auto range = data.PossibleOutputLabels(ilabel);
      std::vector<Label> possible(range.first, range.second);
      for (Label olabel = 1; olabel <= kNumWords; ++olabel)
        EXPECT_THAT(possible, Contains(olabel));
    }
  }

  // Check start
  {
    std::vector<Label> state;
    data.EncodeStartState(&state);
    ASSERT_THAT(state, ElementsAre(1));
  }

  // Path with both non-zero and zero weights
  {
    std::vector<Label> state;
    std::vector<Label> buffer;  // observed input labels
    data.EncodeStartState(&state);
    int interesting = 0;
    for (int i = 0; i < kNumWords * 2; ++i) {
      Label ilabel = i % kNumWords + 1;
      Label olabel = (i / 2) % kNumWords + 1;
      std::vector<Label> next;
      Weight weight = Weight::One();
      data.TakeTransition(buffer.end(), state.begin(), state.end(), In(ilabel),
                          Out(olabel), &next, &weight);
      EXPECT_EQ(UnigramFeature(ilabel, olabel), weight);
      EXPECT_THAT(next, ElementsAre(0));
      if (weight != Weight::One()) ++interesting;
      state = next;
      buffer.push_back(ilabel);
    }
    EXPECT_NE(0, interesting);
  }
}

TEST_F(LinearFstDataTest, StartUnigramNoDelay) {
  StdLinearFstDataBuilder builder;
  int group = builder.AddGroup(0);
  ASSERT_GE(group, 0);
  AddWords(&builder);
  AddUnigramFeatures(group, &builder);
  AddStartUnigramFeatures(group, &builder);

  std::unique_ptr<const StdLinearFstData> data_ptr(builder.Dump());
  const StdLinearFstData& data = *data_ptr;

  // Check start
  {
    std::vector<Label> state;
    data.EncodeStartState(&state);
    ASSERT_THAT(state, ElementsAre(1));
  }

  // Length 2 paths
  {
    int interesting = 0;
    for (Label ilabel0 = 1; ilabel0 <= kNumWords; ++ilabel0) {
      for (Label olabel0 = 1; olabel0 <= kNumWords; ++olabel0) {
        for (Label ilabel1 = 1; ilabel1 <= kNumWords; ++ilabel1) {
          for (Label olabel1 = 1; olabel1 <= kNumWords; ++olabel1) {
            std::vector<Label> state;
            std::vector<Label> buffer;  // observed input labels
            std::vector<Label> next;
            Weight weight = Weight::One();
            data.EncodeStartState(&state);
            data.TakeTransition(buffer.end(), state.begin(), state.end(),
                                In(ilabel0), Out(olabel0), &next, &weight);
            ASSERT_EQ(Times(StartUnigramFeature(ilabel0, olabel0, 5),
                            UnigramFeature(ilabel0, olabel0)),
                      weight);
            EXPECT_THAT(next, ElementsAre(0));
            if (weight != Weight::One()) ++interesting;
            buffer.push_back(ilabel0);
            state = next;
            next.clear();
            weight = Weight::One();

            data.TakeTransition(buffer.end(), state.begin(), state.end(),
                                In(ilabel1), Out(olabel1), &next, &weight);
            ASSERT_EQ(UnigramFeature(ilabel1, olabel1), weight);
            EXPECT_THAT(next, ElementsAre(0));
            if (weight != Weight::One()) ++interesting;
          }
        }
      }
    }
    EXPECT_NE(0, interesting);
  }
}

TEST_F(LinearFstDataTest, EndUnigram) {
  for (size_t future_size = 0; future_size <= 4; ++future_size) {
    StdLinearFstDataBuilder builder;
    int group = builder.AddGroup(future_size);
    ASSERT_GE(group, 0);
    AddWords(&builder);
    AddEndUnigramFeatures(group, future_size, &builder);

    std::unique_ptr<const StdLinearFstData> data_ptr(builder.Dump());
    const StdLinearFstData& data = *data_ptr;

    // Check start
    {
      std::vector<Label> state;
      data.EncodeStartState(&state);
      ASSERT_THAT(state, ElementsAre(1));
    }

    // Length 2 paths
    {
      int interesting = 0;
      for (Label ilabel0 = 1; ilabel0 <= kNumWords; ++ilabel0) {
        for (Label olabel0 = 1; olabel0 <= kNumWords; ++olabel0) {
          for (Label ilabel1 = 1; ilabel1 <= kNumWords; ++ilabel1) {
            for (Label olabel1 = 1; olabel1 <= kNumWords; ++olabel1) {
              std::vector<Label> input(2 + future_size,
                                       StdLinearFstData::kEndOfSentence),
                  output(2 + future_size, StdLinearFstData::kStartOfSentence);
              input[0] = ilabel0;
              input[1] = ilabel1;
              output[future_size] = olabel0;
              output[future_size + 1] = olabel1;

              std::vector<Label> state;
              std::vector<Label> buffer(
                  future_size, StdLinearFstData::kStartOfSentence);  // observed
                                                                     // input
                                                                     // labels
              std::vector<Label> next;
              Weight weight = Weight::One();

              data.EncodeStartState(&state);
              for (size_t i = 0; i < 2 + future_size; ++i) {
                next.clear();
                data.TakeTransition(buffer.end(), state.begin(), state.end(),
                                    In(input[i]), Out(output[i]), &next,
                                    &weight);
                buffer.push_back(input[i]);
                state = next;
              }

              weight =
                  Times(weight, data.FinalWeight(next.begin(), next.end()));

              Weight final_weight =
                  future_size == 0 ? EndUnigramFeature(ilabel1, olabel1)
                                   : EndUnigramFeature(ilabel1, olabel1, 2);
              ASSERT_EQ(final_weight, weight);
              if (final_weight != Weight::One()) ++interesting;
            }
          }
        }
      }
      EXPECT_NE(0, interesting);
    }
  }
}

TEST_F(LinearFstDataTest, EmptyStore) {
  StdLinearFstDataBuilder builder;
  AddWords(&builder);
  std::unique_ptr<StdLinearFstData> data_ptr(builder.Dump());
  const StdLinearFstData& data = *data_ptr;
  for (Label ilabel1 = 1; ilabel1 <= kNumWords; ++ilabel1)
    for (Label olabel1 = 1; olabel1 <= kNumWords; ++olabel1)
      for (Label ilabel2 = 1; ilabel2 <= kNumWords; ++ilabel2)
        for (Label olabel2 = 1; olabel2 <= kNumWords; ++olabel2) {
          std::vector<Label> state, next, buffer;
          data.EncodeStartState(&state);
          {
            Weight weight = Weight::One();
            data.TakeTransition(buffer.end(), state.begin(), state.end(),
                                In(ilabel1), Out(olabel1), &next, &weight);
            EXPECT_EQ(Weight::One(), weight);
            state = next;
          }
          {
            Weight weight = Weight::One();
            data.TakeTransition(buffer.end(), state.begin(), state.end(),
                                In(ilabel2), Out(olabel2), &next, &weight);
            EXPECT_EQ(Weight::One(), weight);
            state = next;
          }
        }
}

TEST_F(LinearFstDataTest, BigramNoDelay) {
  StdLinearFstDataBuilder builder;
  int group = builder.AddGroup(0);
  ASSERT_GE(group, 0);
  AddConstrainedWords(&builder);
  AddBigramFeatures(group, &builder);

  std::unique_ptr<StdLinearFstData> data_ptr(builder.Dump());
  const StdLinearFstData& data = *data_ptr;

  // Simple properties
  {
    EXPECT_EQ(1, data.MinInputLabel());
    EXPECT_EQ(kNumWords, data.MaxInputLabel());
    EXPECT_EQ(0, data.MaxFutureSize());
    for (Label ilabel = 1; ilabel <= kNumWords; ++ilabel) {
      auto range = data.PossibleOutputLabels(ilabel);
      std::vector<Label> possible(range.first, range.second);
      for (Label olabel = 1; olabel <= kNumWords; ++olabel)
        if (olabel >= ilabel)
          EXPECT_THAT(possible, Contains(olabel));
        else
          EXPECT_THAT(possible, Not(Contains(olabel)));
    }
  }

  // Check start
  {
    std::vector<Label> state;
    data.EncodeStartState(&state);
    ASSERT_THAT(state, ElementsAre(1));
  }

  // First step; no feature is fired
  {
    std::vector<Label> state, buffer;
    data.EncodeStartState(&state);
    for (Label ilabel = 1; ilabel <= kNumWords; ++ilabel)
      for (Label olabel = 1; olabel <= kNumWords; ++olabel) {
        std::vector<Label> next;
        Weight weight = Weight::One();
        data.TakeTransition(buffer.end(), state.begin(), state.end(),
                            In(ilabel), Out(olabel), &next, &weight);
        EXPECT_NE(0, next[0]);
        EXPECT_EQ(Weight::One(), weight);
      }
  }

  // Path: 3:1 2:2 1:3, decreasing ilabel thus no feature is fired
  {
    std::vector<Label> state, buffer;
    data.EncodeStartState(&state);
    for (int i = 1; i <= kNumWords; ++i) {
      Label ilabel = kNumWords + 1 - i;
      Label olabel = i;
      std::vector<Label> next;
      Weight weight = Weight::One();
      data.TakeTransition(buffer.end(), state.begin(), state.end(), In(ilabel),
                          Out(olabel), &next, &weight);
      EXPECT_NE(0, next[0]);
      EXPECT_EQ(Weight::One(), weight);
      state = next;
      buffer.push_back(ilabel);
    }
  }

  // Path: 1:1 1:2 2:3 2:1 3:2 3:3
  {
    std::vector<Label> state, buffer;
    data.EncodeStartState(&state);
    int interesting = 0;
    for (int i = 0; i < kNumWords * 2; ++i) {
      Label ilabel = (i / 2 % kNumWords) + 1;
      Label olabel = i % kNumWords + 1;
      std::vector<Label> next;
      Weight weight = Weight::One();
      data.TakeTransition(buffer.end(), state.begin(), state.end(), In(ilabel),
                          Out(olabel), &next, &weight);
      if (i == 0) {
        EXPECT_EQ(Weight::One(), weight);
      } else {
        Label prev_ilabel = ((i - 1) / 2 % kNumWords) + 1;
        EXPECT_EQ(BigramFeature(prev_ilabel, ilabel, olabel), weight);
        if (weight != Weight::One()) ++interesting;
      }
      state = next;
      buffer.push_back(ilabel);
    }
    EXPECT_NE(0, interesting);
  }
}

TEST_F(LinearFstDataTest, Bigram1Delay) {
  StdLinearFstDataBuilder builder;
  int group = builder.AddGroup(1);
  ASSERT_GE(group, 0);
  AddConstrainedWords(&builder);
  AddBigramFeatures(group, &builder);

  std::unique_ptr<StdLinearFstData> data_ptr(builder.Dump());
  const StdLinearFstData& data = *data_ptr;

  // Simple properties
  {
    EXPECT_EQ(1, data.MinInputLabel());
    EXPECT_EQ(kNumWords, data.MaxInputLabel());
    EXPECT_EQ(1, data.MaxFutureSize());
    for (Label ilabel = 1; ilabel <= kNumWords; ++ilabel) {
      auto range = data.PossibleOutputLabels(ilabel);
      std::vector<Label> possible(range.first, range.second);
      for (Label olabel = 1; olabel <= kNumWords; ++olabel)
        if (olabel >= ilabel)
          EXPECT_THAT(possible, Contains(olabel));
        else
          EXPECT_THAT(possible, Not(Contains(olabel)));
    }
  }

  // Check start
  {
    std::vector<Label> state;
    data.EncodeStartState(&state);
    ASSERT_THAT(state, ElementsAre(1));
  }

  // First step; no feature is fired
  {
    std::vector<Label> state;
    data.EncodeStartState(&state);
    for (Label ilabel = 1; ilabel <= kNumWords; ++ilabel)
      for (Label olabel = 0; olabel <= kNumWords; ++olabel) {
        std::vector<Label> next, buffer(1, StdLinearFstData::kStartOfSentence);
        Weight weight = Weight::One();
        data.TakeTransition(buffer.end(), state.begin(), state.end(),
                            In(ilabel), Out(olabel), &next, &weight);
        EXPECT_NE(0, next[0]);
        EXPECT_EQ(Weight::One(), weight);
      }
  }

  // Path: 3:0 2:1 1:2 0:3, decreasing ilabel thus no feature is fired
  {
    std::vector<Label> state, buffer(1, StdLinearFstData::kStartOfSentence);
    data.EncodeStartState(&state);
    for (int i = 0; i <= kNumWords; ++i) {
      Label ilabel = kNumWords - i;
      Label olabel = i;
      std::vector<Label> next;
      Weight weight = Weight::One();
      data.TakeTransition(buffer.end(), state.begin(), state.end(), In(ilabel),
                          Out(olabel), &next, &weight);
      if (i != kNumWords)
        EXPECT_NE(0, next[0]);
      else
        EXPECT_EQ(0, next[0]);
      EXPECT_EQ(Weight::One(), weight);
      state = next;
      buffer.push_back(ilabel);
    }
  }

  // Input:  1 1 2 2 3 3 0
  //          ╲ ╲ ╲ ╲ ╲ ╲
  // Output: 0 1 2 3 1 2 3
  {
    std::vector<Label> state, buffer(1, StdLinearFstData::kStartOfSentence);
    data.EncodeStartState(&state);
    std::vector<Label> input, output;
    output.push_back(0);
    for (int i = 0; i < kNumWords * 2; ++i) {
      input.push_back(i / 2 % kNumWords + 1);
      output.push_back(i % kNumWords + 1);
    }
    input.push_back(0);

    int interesting = 0;
    for (int i = 0; i <= kNumWords * 2; ++i) {
      Label ilabel = input[i];
      Label olabel = output[i];
      std::vector<Label> next;
      Weight weight = Weight::One();
      data.TakeTransition(buffer.end(), state.begin(), state.end(), In(ilabel),
                          Out(olabel), &next, &weight);
      if (i == 0 || i == kNumWords * 2) {
        EXPECT_EQ(Weight::One(), weight);
      } else {
        Label prev_ilabel = input[i - 1];
        EXPECT_EQ(BigramFeature(prev_ilabel, ilabel, olabel), weight);
      }
      if (weight != Weight::One()) ++interesting;
      state = next;
      buffer.push_back(ilabel);
    }
    EXPECT_NE(0, interesting);
  }
}

TEST_F(LinearFstDataTest, Unigram0DelayBigram1Delay) {
  StdLinearFstDataBuilder builder;
  AddWords(&builder);
  AddUnigramFeatures(builder.AddGroup(0), &builder);
  AddBigramFeatures(builder.AddGroup(1), &builder);

  std::unique_ptr<StdLinearFstData> data_ptr(builder.Dump());
  const StdLinearFstData& data = *data_ptr;

  // Simple properties
  {
    EXPECT_EQ(1, data.MinInputLabel());
    EXPECT_EQ(kNumWords, data.MaxInputLabel());
    EXPECT_EQ(1, data.MaxFutureSize());
    for (Label ilabel = 1; ilabel <= kNumWords; ++ilabel) {
      auto range = data.PossibleOutputLabels(ilabel);
      std::vector<Label> possible(range.first, range.second);
      for (Label olabel = 1; olabel <= kNumWords; ++olabel)
        EXPECT_THAT(possible, Contains(olabel));
    }
  }

  // Check start
  {
    std::vector<Label> state;
    data.EncodeStartState(&state);
    ASSERT_THAT(state, ElementsAre(1, 1));
  }

  // First step; no feature is fired because of delay in unigram features
  {
    std::vector<Label> state;
    data.EncodeStartState(&state);
    for (Label ilabel = 1; ilabel <= kNumWords; ++ilabel)
      for (Label olabel = 0; olabel <= kNumWords; ++olabel) {
        std::vector<Label> next, buffer(1, StdLinearFstData::kStartOfSentence);
        Weight weight = Weight::One();
        data.TakeTransition(buffer.end(), state.begin(), state.end(),
                            In(ilabel), Out(olabel), &next, &weight);
        // Unigram state
        EXPECT_EQ(1, next[0]);
        // Bigram state
        EXPECT_NE(0, next[1]);
        // No weight!
        EXPECT_EQ(Weight::One(), weight);
      }
  }

  // Input:  3 2 1 0
  //          ╲ ╲ ╲
  // Output: 0 3 1 2
  //
  // Decreasing ilabel thus only unigram features are
  // fired (with delay)
  {
    std::vector<Label> state, buffer(1, StdLinearFstData::kStartOfSentence);
    data.EncodeStartState(&state);
    std::vector<Label> input, output;
    output.push_back(0);
    for (int i = 1; i <= kNumWords; ++i) {
      input.push_back(kNumWords - i + 1);
      output.push_back((i + 1) % kNumWords + 1);
    }
    input.push_back(0);
    int interesting = 0;
    for (int i = 0; i <= kNumWords; ++i) {
      Label ilabel = input[i];
      Label olabel = output[i];
      std::vector<Label> next;
      Weight weight = Weight::One();
      data.TakeTransition(buffer.end(), state.begin(), state.end(), In(ilabel),
                          Out(olabel), &next, &weight);
      // Weight
      if (i > 0) {
        Label prev_ilabel = input[i - 1];
        EXPECT_EQ(UnigramFeature(prev_ilabel, olabel), weight);
      }
      if (weight != Weight::One()) ++interesting;
      state = next;
      buffer.push_back(ilabel);
    }
    EXPECT_NE(0, interesting);
  }

  // Input:  1 1 2 2 3 3 0
  //          ╲ ╲ ╲ ╲ ╲ ╲
  // Output: 0 1 2 3 1 2 3
  {
    std::vector<Label> state, buffer(1, StdLinearFstData::kStartOfSentence);
    data.EncodeStartState(&state);
    std::vector<Label> input, output;
    output.push_back(0);
    for (int i = 0; i < kNumWords * 2; ++i) {
      input.push_back(i / 2 % kNumWords + 1);
      output.push_back(i % kNumWords + 1);
    }
    input.push_back(0);
    int interesting_unigram = 0, interesting_bigram = 0;
    for (int i = 0; i <= kNumWords * 2; ++i) {
      Label ilabel = input[i];
      Label olabel = output[i];
      std::vector<Label> next;
      Weight weight = Weight::One();
      data.TakeTransition(buffer.end(), state.begin(), state.end(), In(ilabel),
                          Out(olabel), &next, &weight);
      // Unigram features and bigram features may both contribute to the weight
      Weight expected_weight = Weight::One();
      if (i > 0) {
        Label prev_ilabel = input[i - 1];
        expected_weight =
            Times(expected_weight, UnigramFeature(prev_ilabel, olabel));
        if (UnigramFeature(prev_ilabel, olabel) != Weight::One())
          ++interesting_unigram;
        if (i < kNumWords * 2) {
          expected_weight = Times(expected_weight,
                                  BigramFeature(prev_ilabel, ilabel, olabel));
          if (BigramFeature(prev_ilabel, ilabel, olabel) != Weight::One())
            ++interesting_bigram;
        }
      }
      EXPECT_EQ(expected_weight, weight);
      state = next;
      buffer.push_back(ilabel);
    }
    EXPECT_NE(0, interesting_unigram);
    EXPECT_NE(0, interesting_bigram);
  }
}

TEST_F(LinearFstDataTest, SubsetFeatures) {
  StdLinearFstDataBuilder builder;
  AddWords(&builder);
  int group = builder.AddGroup(0);
  ASSERT_GE(group, 0);
  AddUnigramFeatures(group, &builder);
  AddBigramFeatures(group, &builder);

  std::unique_ptr<StdLinearFstData> data_ptr(builder.Dump());
  const StdLinearFstData& data = *data_ptr;

  // Input:  3 2 1
  // Output: 2 2 2
  //
  // Decreasing ilabel thus only unigram features are
  // fired
  {
    std::vector<Label> state, buffer;
    data.EncodeStartState(&state);
    std::vector<Label> input, output;
    for (int i = 1; i <= kNumWords; ++i) {
      input.push_back(kNumWords - i + 1);
      output.push_back(2);
    }
    int interesting = 0;
    for (int i = 0; i < kNumWords; ++i) {
      Label ilabel = input[i];
      Label olabel = output[i];
      std::vector<Label> next;
      Weight weight = Weight::One();
      data.TakeTransition(buffer.end(), state.begin(), state.end(), In(ilabel),
                          Out(olabel), &next, &weight);
      // Weight
      EXPECT_EQ(UnigramFeature(ilabel, olabel), weight);
      if (weight != Weight::One()) ++interesting;
      state = next;
      buffer.push_back(ilabel);
    }
    EXPECT_NE(0, interesting);
  }

  // Input:  1 1 2 2 3 3
  // Output: 1 2 3 1 2 3
  {
    std::vector<Label> state, buffer;
    data.EncodeStartState(&state);
    std::vector<Label> input, output;
    for (int i = 0; i < kNumWords * 2; ++i) {
      input.push_back(i / 2 % kNumWords + 1);
      output.push_back(i % kNumWords + 1);
    }
    int interesting_unigram = 0, interesting_bigram = 0;
    for (int i = 0; i < kNumWords * 2; ++i) {
      Label ilabel = input[i];
      Label olabel = output[i];
      std::vector<Label> next;
      Weight weight = Weight::One();
      data.TakeTransition(buffer.end(), state.begin(), state.end(), In(ilabel),
                          In(olabel), &next, &weight);
      // Unigram features and bigram features may both contribute to the weight
      Weight expected_weight = UnigramFeature(ilabel, olabel);
      if (expected_weight != Weight::One()) ++interesting_unigram;
      if (i > 0) {
        Label prev_ilabel = input[i - 1];
        Weight bigram_weight = BigramFeature(prev_ilabel, ilabel, olabel);
        expected_weight = Times(expected_weight, bigram_weight);
        if (bigram_weight != Weight::One()) ++interesting_bigram;
      }
      EXPECT_EQ(expected_weight, weight);
      state = next;
      buffer.push_back(ilabel);
    }
    EXPECT_NE(0, interesting_unigram);
    EXPECT_NE(0, interesting_bigram);
  }
}

TEST_F(LinearFstDataTest, HMM) {
  StdLinearFstDataBuilder builder;
  AddWords(&builder);
  int emission = builder.AddGroup(0);
  int transition = builder.AddGroup(0);
  ASSERT_GE(emission, 0);
  ASSERT_GE(transition, 0);
  AddUnigramFeatures(emission, &builder);
  AddTransitionFeatures(transition, &builder);

  std::unique_ptr<StdLinearFstData> data_ptr(builder.Dump());
  const StdLinearFstData& data = *data_ptr;

  // Input:  1 1 1
  // Output: 3 2 1
  // Decreasing output, only emission features
  {
    std::vector<Label> state, buffer;
    data.EncodeStartState(&state);
    std::vector<Label> input, output;
    for (int i = 0; i < kNumWords; ++i) {
      input.push_back(1);
      output.push_back(kNumWords - i);
    }
    int interesting_unigram = 0;
    for (int i = 0; i < kNumWords; ++i) {
      Label ilabel = input[i];
      Label olabel = output[i];
      std::vector<Label> next;
      Weight weight = Weight::One();
      data.TakeTransition(buffer.end(), state.begin(), state.end(), In(ilabel),
                          Out(olabel), &next, &weight);
      Weight expected_weight = UnigramFeature(ilabel, olabel);
      if (expected_weight != Weight::One()) ++interesting_unigram;
      EXPECT_EQ(expected_weight, weight);
      state = next;
      buffer.push_back(ilabel);
    }
    EXPECT_NE(0, interesting_unigram);
  }

  // Input:  1 1 2 2 3 3
  // Output: 1 2 3 1 2 3
  {
    std::vector<Label> state, buffer;
    data.EncodeStartState(&state);
    std::vector<Label> input, output;
    for (int i = 0; i < kNumWords * 2; ++i) {
      input.push_back(i / 2 % kNumWords + 1);
      output.push_back(i % kNumWords + 1);
    }
    int interesting_emission = 0, interesting_transition = 0;
    for (int i = 0; i < kNumWords * 2; ++i) {
      Label ilabel = input[i];
      Label olabel = output[i];
      std::vector<Label> next;
      Weight weight = Weight::One();
      data.TakeTransition(buffer.end(), state.begin(), state.end(), In(ilabel),
                          Out(olabel), &next, &weight);
      Weight emission_weight = UnigramFeature(ilabel, olabel);
      Weight transition_weight = Weight::One();
      if (i > 0) {
        Label prev_olabel = output[i - 1];
        transition_weight = TransitionFeature(prev_olabel, olabel);
      }
      if (emission_weight != Weight::One()) ++interesting_emission;
      if (transition_weight != Weight::One()) ++interesting_transition;
      EXPECT_EQ(Times(emission_weight, transition_weight), weight);
      state = next;
      buffer.push_back(ilabel);
    }
    EXPECT_NE(0, interesting_emission);
    EXPECT_NE(0, interesting_transition);
  }
}

TEST_F(LinearFstDataTest, UnigramNoDelayBigram2Delay) {
  StdLinearFstDataBuilder builder;
  AddWords(&builder);
  AddUnigramFeatures(builder.AddGroup(0), &builder);
  AddBigramFeatures(builder.AddGroup(2), &builder);

  std::unique_ptr<StdLinearFstData> data_ptr(builder.Dump());
  const StdLinearFstData& data = *data_ptr;

  EXPECT_EQ(2, data.MaxFutureSize());

  // Input:  3 2 1 0 0
  // Output: 0 0 2 3 1
  // Decreasing ilabel thus only unigram features fire
  {
    const int kLength = 5;
    const int input[kLength] = {3, 2, 1, 0, 0};
    const int output[kLength] = {0, 0, 2, 3, 1};
    std::vector<Label> state, buffer(2, StdLinearFstData::kStartOfSentence);
    data.EncodeStartState(&state);
    int interesting = 0;
    for (int i = 0; i < kLength; ++i) {
      std::vector<Label> next;
      Weight weight = Weight::One();
      data.TakeTransition(buffer.end(), state.begin(), state.end(),
                          In(input[i]), Out(output[i]), &next, &weight);
      if (i >= 2) {
        Weight unigram_weight = UnigramFeature(input[i - 2], output[i]);
        if (unigram_weight != Weight::One()) ++interesting;
        EXPECT_EQ(unigram_weight, weight);
      }
      state = next;
      buffer.push_back(input[i]);
    }
    EXPECT_NE(0, interesting);
  }

  // Input:  1 1 2 2 3 3 0 0
  // Output: 0 0 1 2 3 1 2 3
  {
    const int kLength = 8;
    const int input[kLength] = {1, 1, 2, 2, 3, 3, 0, 0};
    const int output[kLength] = {0, 0, 1, 2, 3, 1, 2, 3};
    std::vector<Label> state, buffer(2, StdLinearFstData::kStartOfSentence);
    data.EncodeStartState(&state);
    int interesting_unigram = 0, interesting_bigram = 0;
    for (int i = 0; i < kLength; ++i) {
      std::vector<Label> next;
      Weight weight = Weight::One();
      data.TakeTransition(buffer.end(), state.begin(), state.end(),
                          In(input[i]), Out(output[i]), &next, &weight);
      Weight unigram_weight = Weight::One(), bigram_weight = Weight::One();
      if (i >= 2) {
        unigram_weight = UnigramFeature(input[i - 2], output[i]);
        if (unigram_weight != Weight::One()) ++interesting_unigram;
      }
      if (2 <= i && i < kLength - 2) {
        bigram_weight = BigramFeature(input[i - 1], input[i], output[i]);
        if (bigram_weight != Weight::One()) ++interesting_bigram;
      }
      EXPECT_EQ(Times(unigram_weight, bigram_weight), weight);
      state = next;
      buffer.push_back(input[i]);
    }
    EXPECT_NE(0, interesting_unigram);
    EXPECT_NE(0, interesting_bigram);
  }
}

class LinearFstDataBuilderDeathTest : public Test {
 protected:
  void AddWordTwice() {
    StdLinearFstDataBuilder builder;
    std::vector<Label> features(1, 1), possible(1, 1);
    builder.AddWord(1, features);
    builder.AddWord(1, features, possible);
  }

  void AddEpsilonWord() {
    StdLinearFstDataBuilder builder;
    std::vector<Label> features(1, 1);
    builder.AddWord(0, features);
  }

  void AddEpsilonFeature() {
    StdLinearFstDataBuilder builder;
    std::vector<Label> features(1, -1);
    builder.AddWord(1, features);
  }

  void AddEmptyConstraint() {
    StdLinearFstDataBuilder builder;
    std::vector<Label> features(1, 1), possible;
    builder.AddWord(1, features, possible);
  }

  void AddWeightEpsilonFeature() {
    StdLinearFstDataBuilder builder;
    int group = builder.AddGroup(0);
    std::vector<Label> input(1, 0), output;
    builder.AddWeight(group, input, output, Weight::One());
  }

  void AddWeightEpsilonOutput() {
    StdLinearFstDataBuilder builder;
    int group = builder.AddGroup(0);
    std::vector<Label> input, output(1, 0);
    builder.AddWeight(group, input, output, Weight::One());
  }

  void DumpBranchingBackOffEitherSide() {
    StdLinearFstDataBuilder builder;
    int group = builder.AddGroup(0);
    std::vector<Label> feat;
    feat.push_back(1);
    feat.push_back(2);
    builder.AddWeight(group, feat, feat, Weight::One());
    builder.AddWeight(group, feat, std::vector<Label>(), Weight::One());
    builder.AddWeight(group, std::vector<Label>(), feat, Weight::One());
    std::unique_ptr<StdLinearFstData> data_ptr(builder.Dump());
  }

  void DumpBranchingBackOffOneSide() {
    StdLinearFstDataBuilder builder;
    int group = builder.AddGroup(0);
    std::vector<Label> complete, partial;
    complete.push_back(1);
    complete.push_back(2);
    partial.push_back(2);
    builder.AddWeight(group, complete, partial, Weight::One());
    builder.AddWeight(group, complete, std::vector<Label>(), Weight::One());
    builder.AddWeight(group, partial, partial, Weight::One());
    std::unique_ptr<StdLinearFstData> data_ptr(builder.Dump());
  }

  // TODO: pass this test
  void DumpOverlappingContext() {
    StdLinearFstDataBuilder builder;
    int group = builder.AddGroup(0);
    std::vector<Label> in1, in2, out1, out2;
    for (int i = 1; i <= 5; ++i) in1.push_back(i);
    for (int i = 4; i <= 5; ++i) out1.push_back(i);
    for (int i = 2; i <= 4; ++i) in2.push_back(i);
    for (int i = 3; i <= 4; ++i) out2.push_back(i);
    builder.AddWeight(group, in1, out1, Weight::One());
    builder.AddWeight(group, in2, out2, Weight::One());
    std::unique_ptr<StdLinearFstData> data_ptr(builder.Dump());
  }

  // TODO: more death tests
};

TEST_F(LinearFstDataBuilderDeathTest, AddWordTwice) {
  EXPECT_DEATH(AddWordTwice(), "Input word .* is added twice");
}

TEST_F(LinearFstDataBuilderDeathTest, AddEpsilonWord) {
  EXPECT_DEATH(AddEpsilonWord(), "Word label must be > 0; got .*");
}

TEST_F(LinearFstDataBuilderDeathTest, AddEpsilonFeature) {
  EXPECT_DEATH(AddEpsilonFeature(), "Feature label must be > 0; got .*");
}

TEST_F(LinearFstDataBuilderDeathTest, AddEmptyConstraint) {
  EXPECT_DEATH(AddEmptyConstraint(), "Empty possible output constraint");
}

TEST_F(LinearFstDataBuilderDeathTest, AddWeightEpsilonFeature) {
  EXPECT_DEATH(AddWeightEpsilonFeature(), "Feature label must be > 0; got .*");
}

TEST_F(LinearFstDataBuilderDeathTest, AddWeightEpsilonOutput) {
  EXPECT_DEATH(AddWeightEpsilonOutput(), "Output label must be > 0; got .*");
}

TEST_F(LinearFstDataBuilderDeathTest, DumpBranchingBackOffEitherSide) {
  EXPECT_DEATH(DumpBranchingBackOffEitherSide(), "Branching back-off chain");
}

TEST_F(LinearFstDataBuilderDeathTest, DumpBranchingBackOffOneSide) {
  EXPECT_DEATH(DumpBranchingBackOffOneSide(), "Branching back-off chain");
}

// TODO: be able to check this
// TEST_F(LinearFstDataBuilderDeathTest, DumpOverlappingContext) {
//   EXPECT_DEATH(DumpOverlappingContext(), ".*");
// }

TEST(GuessStartOrEndTest, NoChange) {
  std::vector<Label> a;
  int unresolved = GuessStartOrEnd<StdArc>(&a, kNoLabel);
  EXPECT_EQ(0, unresolved);
  EXPECT_TRUE(a.empty());

  for (int i = 0; i < 10; ++i) a.push_back(i);
  std::vector<Label> b(a);
  unresolved = GuessStartOrEnd<StdArc>(&a, kNoLabel);
  EXPECT_EQ(0, unresolved);
  EXPECT_EQ(b, a);
}

TEST(GuessStartOrEndTest, GuessStart) {
  const Label input[] = {kNoLabel, kNoLabel, kNoLabel, 0, kNoLabel, 0, 0, 0};
  const Label answer[] = {StdLinearFstData::kStartOfSentence,
                          StdLinearFstData::kStartOfSentence,
                          StdLinearFstData::kStartOfSentence,
                          0,
                          StdLinearFstData::kStartOfSentence,
                          0,
                          0,
                          0};
  std::vector<Label> a(input, input + sizeof(input) / sizeof(Label)),
      b(answer, answer + sizeof(answer) / sizeof(Label));
  ASSERT_NE(b, a);
  int unresolved = GuessStartOrEnd<StdArc>(&a, kNoLabel);
  EXPECT_EQ(0, unresolved);
  EXPECT_EQ(b, a);
}

TEST(GuessStartOrEndTest, GuessEnd) {
  const Label input[] = {0, 0, 0, kNoLabel, kNoLabel, kNoLabel};
  const Label answer[] = {0,
                          0,
                          0,
                          StdLinearFstData::kEndOfSentence,
                          StdLinearFstData::kEndOfSentence,
                          StdLinearFstData::kEndOfSentence};
  std::vector<Label> a(input, input + sizeof(input) / sizeof(Label)),
      b(answer, answer + sizeof(answer) / sizeof(Label));
  ASSERT_NE(b, a);
  int unresolved = GuessStartOrEnd<StdArc>(&a, kNoLabel);
  EXPECT_EQ(0, unresolved);
  EXPECT_EQ(b, a);
}

TEST(GuessStartOrEndTest, GuessUnresolved) {
  const Label input[] = {kNoLabel, kNoLabel, kNoLabel, kNoLabel};
  const Label answer[] = {kNoLabel, kNoLabel, kNoLabel, kNoLabel};
  std::vector<Label> a(input, input + sizeof(input) / sizeof(Label)),
      b(answer, answer + sizeof(answer) / sizeof(Label));
  int unresolved = GuessStartOrEnd<StdArc>(&a, kNoLabel);
  EXPECT_EQ(4, unresolved);
  EXPECT_EQ(b, a);
}

TEST(GuessStartOrEndTest, GuessStartAndEnd) {
  const Label input[] = {kNoLabel, kNoLabel, 0, kNoLabel, kNoLabel};
  const Label answer[] = {
      StdLinearFstData::kStartOfSentence, StdLinearFstData::kStartOfSentence, 0,
      StdLinearFstData::kEndOfSentence, StdLinearFstData::kEndOfSentence};
  std::vector<Label> a(input, input + sizeof(input) / sizeof(Label)),
      b(answer, answer + sizeof(answer) / sizeof(Label));
  ASSERT_NE(b, a);
  int unresolved = GuessStartOrEnd<StdArc>(&a, kNoLabel);
  EXPECT_EQ(0, unresolved);
  EXPECT_EQ(b, a);
}

TEST(ClassifierBuilderTest, LogicalGroups) {
  StdLinearClassifierFstDataBuilder builder(3);
  {
    // Logical group ids are consecutive.
    int a = builder.AddGroup(), b = builder.AddGroup(), c = builder.AddGroup();
    EXPECT_EQ(0, a);
    EXPECT_EQ(1, b);
    EXPECT_EQ(2, c);
  }
  std::unique_ptr<StdLinearFstData> data(builder.Dump());
  ASSERT_TRUE(data != nullptr);
  EXPECT_EQ(9, data->NumGroups());
}

TEST(ClassifierBuilderTest, AddWeight) {
  const int kNumClasses = 2;
  const int kNumGroups = 3;
  StdLinearClassifierFstDataBuilder builder(kNumClasses);
  std::vector<Label> feat(1, 1);
  builder.AddWord(1, feat);
  for (int i = 0; i < kNumGroups; ++i) builder.AddGroup();
  for (Label pred = 1; pred <= kNumClasses; ++pred)
    for (int i = 0; i < kNumGroups; ++i)
      builder.AddWeight(i, feat, pred, Weight(pred * i + pred + i));

  std::unique_ptr<StdLinearFstData> data(builder.Dump());
  ASSERT_TRUE(data != nullptr);

  for (Label pred = 1; pred <= kNumClasses; ++pred) {
    Weight weight = Weight::One(), expected_weight = Weight::One();
    for (int group = 0; group < kNumGroups; ++group) {
      int real_group = group * kNumClasses + pred - 1;
      Label start = data->GroupStartState(real_group);
      data->GroupTransition(real_group, start, 1, pred, &weight);
      expected_weight =
          Times(expected_weight, Weight(pred * group + pred + group));
    }
    EXPECT_EQ(expected_weight, weight);
  }
}

TEST(ClassifierBuilderTest, Boundary) {
  StdLinearClassifierFstDataBuilder builder(1);
  std::vector<Label> feat(1, 1);
  builder.AddWord(1, feat);
  builder.AddGroup();

  feat.resize(2);

  // <s> 1
  {
    feat[0] = StdLinearFstData::kStartOfSentence;
    feat[1] = 1;
    bool added = builder.AddWeight(0, feat, 1, Weight(1));
    ASSERT_TRUE(added);
  }

  // 1 1
  {
    feat[0] = feat[1] = 1;
    bool added = builder.AddWeight(0, feat, 1, Weight(2));
    ASSERT_TRUE(added);
  }

  // 1 </s>
  {
    feat[0] = 1;
    feat[1] = StdLinearFstData::kEndOfSentence;
    bool added = builder.AddWeight(0, feat, 1, Weight(3));
    ASSERT_TRUE(added);
  }

  std::unique_ptr<StdLinearFstData> data(builder.Dump());
  ASSERT_TRUE(data != nullptr);

  Label state = data->GroupStartState(0);
  Weight weight = Weight::One();

  EXPECT_EQ(1, state);

  state = data->GroupTransition(0, state, 1, 1, &weight);
  EXPECT_EQ(Weight(1), weight);
  EXPECT_EQ(3, state);

  state = data->GroupTransition(0, state, 1, 1, &weight);
  EXPECT_EQ(Weight(3), weight);
  EXPECT_EQ(3, state);

  for (int i = 0; i < 5; ++i) {
    LOG(INFO) << i << " " << data->GroupFinalWeight(0, i);
  }
  weight = Times(weight, data->GroupFinalWeight(0, state));
  EXPECT_EQ(Weight(6), weight);
}

}  // namespace fst
