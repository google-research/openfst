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
// Categorial semiring from:
//
// Richard Sproat, Mahsa Yarmohamadi, Izhak Shafran, and Brian Roark. 2014.
// Applications of lexicographic semirings to problems in speech and language
// processing. Computational Linguistics 40(4): 733-761.
// https://aclanthology.org/J14-4002/

#ifndef OPENFST_EXTENSIONS_CATEGORIAL_CATEGORIAL_WEIGHT_H_
#define OPENFST_EXTENSIONS_CATEGORIAL_CATEGORIAL_WEIGHT_H_

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <ios>
#include <istream>
#include <iterator>
#include <list>
#include <optional>
#include <ostream>
#include <set>
#include <stack>
#include <string>
#include <vector>

#include "absl/base/no_destructor.h"
#include "absl/random/bit_gen_ref.h"
#include "absl/random/random.h"
#include "absl/strings/str_split.h"
#include "absl/strings/string_view.h"
#include "openfst/lib/util.h"
#include "openfst/lib/weight.h"

namespace fst {

inline constexpr int kCategoryBad = -2;       // Label for a non-category
inline constexpr int kLeftAngleBracket = -3;  // Label for "<"
// A complex categorial weight in general looks like: e.g., <a\b>\c for a
// left-categorial weight.  The angle brackets are used as delimiters, the
// left "\" is the "division" operator.
inline constexpr int kRightAngleBracket = -4;  // Label for ">"
inline constexpr int kRightDivision = -5;      // Label for "/"
inline constexpr int kLeftDivision = -6;       // Label for "<backslash>"

template <class T>
class Category;
template <class T>
class CategoryIterator;
template <class T>
class CategoryReverseIterator;

template <class T>
Category<T> Concat(Category<T> first, Category<T> second);

template <class T>
Category<T> Division(Category<T> first, Category<T> second);

template <class T>
Category<T> Reduce(Category<T> c);

template <class T>
bool Match(Category<T> first, Category<T> second);

template <class T>
Category<T> ReverseCat(Category<T> c);

template <class T>
std::vector<T> Reduction(std::vector<T> cat, std::set<int> dashes,
                         std::set<int> slashes, std::list<int> dashes_slashes);

// The category definition that underlies the categorial weight. The
// representation of the category is much as the representation of a "string" in
// a string weight, except that the "<", ">", "\" and "/" labels have special
// meanings. As described in the paper referenced above, one can only have "/"
// or "\", not both --- so a category is either a left category or a right
// category.
//
// We experimented with having a more structured (recursive) representation of
// categories to obviate the need for explicit delimiters in the string, but
// this turned out to be more complicated to implement than the current flat
// representation.

template <class T>
class Category {
  friend class CategoryIterator<T>;
  friend class CategoryReverseIterator<T>;
  friend Category<T> Concat<>(Category<T> first, Category<T> second);
  friend Category<T> Division<>(Category<T> first, Category<T> second);
  friend Category<T> Reduce<>(Category<T> c);
  friend bool Match<>(Category<T> first, Category<T> second);
  friend Category<T> ReverseCat<>(Category<T> c);

 public:
  Category() : one_(false), direction_(kLeftDivision), first_(0) {}

  // Basic category with term t, and default direction left
  explicit Category(T t, int dir = kLeftDivision) {
    first_ = 0;
    PushBack(t);
    direction_ = dir;
    one_ = false;
  }

  // A category derived from a slice of another category
  template <typename Iter>
  Category(const Iter& begin, const Iter& end, int dir = kLeftDivision) {
    first_ = 0;
    one_ = false;
    direction_ = dir;
    for (Iter iter = begin; iter != end; ++iter) PushBack(*iter);
  }

  ~Category() = default;

  const bool one() const { return one_; }
  int direction() { return direction_; }
  void set_one(bool one) { one_ = one; }
  void set_direction(int d) { direction_ = d; }
  size_t Size() const { return first_ ? rest_.size() + 1 : 0; }
  void Clear() {
    one_ = false;
    first_ = 0;
    rest_.clear();
  }

  void PushFront(T t) {
    if (first_) rest_.push_front(first_);
    first_ = t;
  }

  void PushBack(T t) {
    if (!first_) {
      first_ = t;
    } else {
      rest_.push_back(t);
    }
  }

 private:
  // Is this a complex category or just consisting of one or fewer terms?
  bool IsComplex();
  // Does this category function as semiring one?
  bool one_;
  // The direction of the category --- left or right
  int direction_;
  T first_;
  std::list<T> rest_;
};  // class Category

// Concatenation of categories is defined as:
//
// a ^ b = a  { if b = 0 or b = 1
//       = b  { if a = 0 or b = 1
//       = ab { otherwise
template <class T>
Category<T> Concat(Category<T> first, Category<T> second) {
  Category<T> zero;
  Category<T> one;
  one.set_one(true);
  // Must check zero and one equalities in advance to make sure direction_ is
  // set correctly.
  if (Match(second, zero) || Match(second, one)) return first;
  if (Match(first, zero) || Match(first, one)) return second;
  for (CategoryIterator<T> iter(second); !iter.Done(); iter.Next())
    first.PushBack(iter.Value());
  return first;
}

// Left-division of categories is defined as follows. (Note that the second
// argument to the function is the divisor.)
//
// a \ b = undef  { if a = 0
//       = 1      { if a == b
//       = b      { if a == 1
//       = a      { if b == 1
//       = a\b    { where a becomes <a>, if it is complex and similarly for b
//
// Right-division is defined similarly.
template <class T>
Category<T> Division(Category<T> first, Category<T> second) {
  Category<T> zero;
  if (Match(second, zero)) return Category<T>(kCategoryBad);
  if (Match(first, zero)) return zero;
  Category<T> one;
  one.set_one(true);
  if (Match(first, second)) return one;
  if (Match(second, one)) return first;
  if (Match(first, one)) return second;
  Category<T> div;
  if (first.IsComplex()) {  // then first --> <first>
    first.PushFront(kLeftAngleBracket);
    first.PushBack(kRightAngleBracket);
  }
  if (second.IsComplex()) {  // then second --> <second>
    second.PushFront(kLeftAngleBracket);
    second.PushBack(kRightAngleBracket);
  }
  if (first.direction() == kLeftDivision) {
    second.PushBack(kLeftDivision);  // second --> <second> <backslash>
    for (CategoryIterator<T> iter(first); !iter.Done(); iter.Next())
      second.PushBack(iter.Value());
    div = second;  // div = <second><backslash>first
    div.set_direction(kLeftDivision);
  } else {                           // kRightDivision
    first.PushBack(kRightDivision);  // first --> first/
    for (CategoryIterator<T> iter(second); !iter.Done(); iter.Next())
      first.PushBack(iter.Value());
    div = first;  // div = first/<second>
    div.set_direction(kRightDivision);
  }
  div.set_one(false);
  return div;
}

// Helper function for Reduce that builds the various slashes and dashes data
// structures.
template <class T>
void BuildSlashesDashes(size_t c_size, std::vector<T> cat,
                        std::list<int>* dashes_slashes, std::set<int>* dashes,
                        std::set<int>* slashes) {
  std::vector<int> matching_paren(c_size);
  // Stack of matching brackets.
  std::stack<int> brackets;
  c_size -= 1;
  for (int j = c_size; j >= 0; j--) {
    if (cat[j] == kRightAngleBracket) {
      if (j < c_size - 1 && brackets.empty() &&
          cat[j + 1] == kLeftAngleBracket) {
        // Potential reducible concat
        if (j < c_size && cat[matching_paren[j + 1] + 1] == kLeftDivision) {
          dashes_slashes->push_front(j);
          dashes->insert(j);
        }
      }
      brackets.push(j);
    } else if (cat[j] == kLeftAngleBracket) {
      // Indicates position of left matching paren.
      matching_paren[j] = brackets.top();
      brackets.pop();
      // If the stack of brackets is empty --- we are not inside brackets ---
      // then this is a potential division position.
    } else if (cat[j] == kLeftDivision && brackets.empty()) {  // Division
      dashes_slashes->push_front(j);
      slashes->insert(j);
      // Otherwise this may be a reducible concat.
    } else if (brackets.empty()) {  // Potential reducible concat
      if (j > 0 && j < c_size && cat[j + 1] == kLeftDivision) {
        // Hallucinates a dash for cat1-cat2<backslash>
        if (cat[j - 1] != kLeftDivision && cat[j - 1] != kLeftAngleBracket &&
            cat[j - 1] != kRightAngleBracket) {  //  cat[j-1] = -
          dashes_slashes->push_front(j - 1);
          dashes->insert(j - 1);
        }
      }
      // Hallucinates a dash for cat1-<cat2><backslash>
      if (j < c_size && cat[j + 1] == kLeftAngleBracket &&
          cat[matching_paren[j + 1] + 1] == kLeftDivision) {
        dashes_slashes->push_front(j);
        dashes->insert(j);
      }
    }
  }
}

// Reduce takes two categories and attempts to reduce them to a simpler category
// by canceling divisions. For example, for a complex category a_a\b, which we
// mark with a dash here to show that a was concatenated with a\b, Reduce(a_a\b)
// = b. Reduction is greedy.
//
// The outer function Reduce() catalogs the position of slashes, and
// "hallucinated" dash markers as in the example above. It calls Reduction(),
// which recursively performs the actual reduction.
template <class T>
Category<T> Reduce(Category<T> c) {
  if (c.direction() == kLeftDivision) {
    CategoryIterator<T> iter(c);
    size_t c_size = c.Size();
    std::vector<T> cat(c_size);
    for (size_t i = 0; !iter.Done(); ++i, iter.Next()) cat[i] = iter.Value();
    // Ordered list of "dash" and slash positions
    std::list<int> dashes_slashes;
    // Catalog of potential reducible concatenations of categories of the form
    // cat1-cat2<backslash>. Note that there is no real "dash" in the input, but
    // we hallucinate one in the position indicated.
    std::set<int> dashes;
    // All divisions that are outside brackets. Divisions inside brackets --
    // e.g. <a\b> are left alone since they are inaccessible to top-level
    // division.
    std::set<int> slashes;
    // Assigns values to dashes_slashes, dashes, and slashes
    BuildSlashesDashes(c_size, cat, &dashes_slashes, &dashes, &slashes);
    // Checks potential reducibles and performs possible reductions
    std::vector<T> reduced_vector =
        Reduction(cat, dashes, slashes, dashes_slashes);
    Category<T> reduced_cat;
    for (int j = 0; j < reduced_vector.size(); j++)
      reduced_cat.PushBack(reduced_vector[j]);
    reduced_cat.set_direction(kLeftDivision);
    return reduced_cat;
  } else {  // first.direction_ == kRightDivision
    return ReverseCat(Reduce(ReverseCat(c)));
  }
}

// Reduction() is a helper function for Reduce() that performs the actual
// reductions recursively.
template <class T>
std::vector<T> Reduction(std::vector<T> cat, std::set<int> dashes,
                         std::set<int> slashes, std::list<int> dashes_slashes) {
  for (std::list<int>::reverse_iterator d = dashes_slashes.rbegin();
       d != dashes_slashes.rend(); d++) {
    for (std::set<int>::reverse_iterator s = slashes.rbegin();
         s != slashes.rend(); s++) {
      // We have located the space between a "dash" and a slash. This is the
      // potential reducee.
      if (*d < *s) {
        int reduction_length = *s - (*d + 1);
        bool second_has_bracket = false;
        // If our span is bracketed, then we need to ignore those when
        // considering the match. So in <a\bc>\d, the denominator should match
        // <a\bc>
        if (cat[*d + 1] == kLeftAngleBracket) {
          reduction_length = reduction_length - 2;
          second_has_bracket = true;
        }
        int begin_second = second_has_bracket ? *d + 2 : *d + 1;
        // The first is the stuff before the "dash"
        int end_first = *d;
        if (dashes.count(*d)) end_first++;
        // If the length of the first is at least as long as the length of the
        // stuff to be reduced then this proceeds to try to reduce, first
        // checking for a match between the two segments.
        if (end_first - reduction_length >= 0) {
          bool match = true;
          int j = begin_second;
          int i = end_first - reduction_length;
          while (i < end_first && j < begin_second + reduction_length) {
            if (cat[i] != cat[j]) {
              match = false;
              break;
            }
            i++;
            j++;
          }
          // If there is a match, we create a reduced category that cancels
          // the two pieces.
          if (match) {
            std::vector<T> reduced_cat;
            // Adds in all the material from the beginning up to the beginning
            // of the first.
            for (int i = 0; i < end_first - reduction_length; i++)
              reduced_cat.push_back(cat[i]);
            // Adds in all the material from the end of the second to the end.
            int to_end = begin_second + reduction_length + 1;
            if (second_has_bracket) to_end = to_end + 1;
            for (int i = to_end; i < cat.size(); i++)
              reduced_cat.push_back(cat[i]);
            int reduction_len = to_end - (end_first - reduction_length);
            // Since the old slashes etc, dashes, etc, are now out of date, we
            // have to update these.
            std::list<int> new_dashes_slashes;
            std::set<int> new_dashes;
            std::set<int> new_slashes;
            for (const int& n_d : dashes_slashes) {
              if (n_d > end_first - reduction_length && n_d < to_end) {
                // Here we do nothing: we are inside an already reduced area.
              } else if (n_d < end_first - reduction_length) {
                // This is before the reduced area, so we add this to
                // new_dashes_slashes without change
                new_dashes_slashes.push_back(n_d);
                if (dashes.count(n_d)) {
                  new_dashes.insert(n_d);
                } else {
                  new_slashes.insert(n_d);
                }
              } else if (n_d >= to_end) {
                // This after the reduced area, so we must subtract the
                // reduction length from the dash or slash position, and then
                // add to new_dashes_slashes.
                new_dashes_slashes.push_back(n_d - reduction_len);
                if (dashes.count(n_d)) {
                  new_dashes.insert(n_d - reduction_len);
                } else {
                  new_slashes.insert(n_d - reduction_len);
                }
              }
            }
            // If reduced_cat is between <>, remove <>, we find potential
            // reduction points and repeat the reduction
            if (reduced_cat[0] == kLeftAngleBracket &&
                reduced_cat[reduced_cat.size() - 1] == kRightAngleBracket) {
              std::vector<int> matching_paren2(reduced_cat.size());
              std::stack<int> brackets1;
              for (int j = reduced_cat.size() - 1; j >= 0; j--) {
                if (reduced_cat[j] == kRightAngleBracket) {
                  brackets1.push(j);
                } else if (reduced_cat[j] == kLeftAngleBracket &&
                           !brackets1.empty()) {
                  matching_paren2[j] = brackets1.top();
                  brackets1.pop();
                }
              }
              if (matching_paren2[0] == reduced_cat.size() - 1) {
                reduced_cat.erase(reduced_cat.begin());
                reduced_cat.pop_back();
                BuildSlashesDashes(reduced_cat.size(), reduced_cat,
                                   &new_dashes_slashes, &new_dashes,
                                   &new_slashes);
              }
            }
            // Attempts further reductions.
            return Reduction(reduced_cat, new_dashes, new_slashes,
                             new_dashes_slashes);
          }
        }
      }
    }
  }
  return cat;
}

// Matcher for categories
template <class T>
bool Match(Category<T> first, Category<T> second) {
  if (first.Size() != second.Size() || first.one() != second.one()) {
    return false;
  }
  CategoryIterator<T> iter1(first);
  CategoryIterator<T> iter2(second);
  for (; !iter1.Done(); iter1.Next(), iter2.Next())
    if (iter1.Value() != iter2.Value()) return false;
  return true;
}

template <class T>
bool Category<T>::IsComplex() {
  if ((*this).Size() > 1) return true;
  return false;
}

// Category reversal, replacing a left category by its corresponding right
// category and vice versa. For example:
//
// reverse(1\4) = 4/1
// reverse(1_2\<1\3>) = <3/1>/2_1
template <class T>
Category<T> ReverseCat(Category<T> c) {
  Category<T> reverse;
  for (CategoryIterator<T> iter(c); !iter.Done(); iter.Next()) {
    if (iter.Value() == kLeftAngleBracket) {
      reverse.PushFront(kRightAngleBracket);
    } else if (iter.Value() == kRightAngleBracket) {
      reverse.PushFront(kLeftAngleBracket);
    } else if (iter.Value() == kRightDivision) {
      reverse.PushFront(kLeftDivision);
    } else if (iter.Value() == kLeftDivision) {
      reverse.PushFront(kRightDivision);
    } else {
      reverse.PushFront(iter.Value());
    }
  }
  if (c.one()) reverse.set_one(true);
  if (c.direction_ == kLeftDivision) {
    reverse.set_direction(kRightDivision);
  } else {
    reverse.set_direction(kLeftDivision);
  }
  return reverse;
}

// Traverses in the forward direction.
template <typename T>
class CategoryIterator {
 public:
  explicit CategoryIterator(const Category<T>& w)
      : first_(w.first_),
        rest_(w.rest_),
        one_(w.one_),
        init_(true),
        iter_(rest_.begin()) {}

  const T& Value() const { return init_ ? first_ : *iter_; }
  const bool Value_One() const { return one_; }

  bool Done() const {
    if (init_) {
      return first_ == 0;
    } else {
      return iter_ == rest_.end();
    }
  }

  void Next() {
    if (init_) {
      init_ = false;
    } else {
      ++iter_;
    }
  }

  void Reset() {
    init_ = true;
    iter_ = rest_.begin();
  }

 private:
  const T& first_;
  const std::list<T>& rest_;
  const bool one_;
  bool init_;  // Is it in the initialized state?
  typename std::list<T>::const_iterator iter_;
};  // class CategoryIterator

// Traverses in the backward direction.
template <typename T>
class CategoryReverseIterator {
 public:
  explicit CategoryReverseIterator(const Category<T>& w)
      : first_(w.first_),
        rest_(w.rest_),
        fin_(first_ == 0),
        iter_(rest_.rbegin()) {}

  bool Done() const { return fin_; }

  const T& Value() const { return iter_ == rest_.rend() ? first_ : *iter_; }

  void Next() {
    if (iter_ == rest_.rend()) {
      fin_ = true;
    } else {
      ++iter_;
    }
  }

  void Reset() {
    fin_ = false;
    iter_ = rest_.rbegin();
  }

 private:
  const T& first_;
  const std::list<T>& rest_;
  bool fin_;  // Is it in the initialized state?
  typename std::list<T>::const_reverse_iterator iter_;
};  // class CategoryReverseIterator

// Definition of categorial weight, built on the underlying category defined
// above.
//
// In order for this to function as a semiring, we must distinguish between the
// value, which is the actual category this represents, from the history, which
// is how it was constructed. As explained in Section 3.2 of the above-cited
// paper, the history is used for Plus to determine which of the two weights is
// lexicographically prior:
//
// a + b = a { if h(w1) < h(w2)
//       = b { otherwise
//
// For times, we need to define both the history and the value of the result:
//
// h(a x b) = h(a)h(b)
// v(a x b) = Reduce(h(a x b))
//
// Division (for a left weight) is given as:
//
// h(a \ b) = h(a) \ h(b)
// v(a \ b) = v(a) \ v(b)

enum class CategoryType : uint8_t { LEFT = 0, RIGHT = 1 };

constexpr CategoryType ReverseCategoryType(CategoryType c) {
  switch (c) {
    case CategoryType::LEFT:
      return CategoryType::RIGHT;
    case CategoryType::RIGHT:
      return CategoryType::LEFT;
  }
  return CategoryType::LEFT;  // Unreachable.
}

template <typename T, CategoryType C = CategoryType::LEFT>
class CategorialWeight;

template <typename T, CategoryType C>
bool operator==(const CategorialWeight<T, C>& w1,
                const CategorialWeight<T, C>& w2);

template <typename T, CategoryType C>
class CategorialWeight {
 public:
  using ReverseWeight = CategorialWeight<T, ReverseCategoryType(C)>;

  friend bool operator==
      <>(const CategorialWeight<T, C>& w1, const CategorialWeight<T, C>& w2);

  explicit CategorialWeight(bool free = false) : value_(Category<T>()) {
    if (free) value_.set_one(true);
  }

  explicit CategorialWeight(Category<T> category)
      : value_(category), history_(category) {}

  CategorialWeight(Category<T> category, Category<T> history)
      : value_(category), history_(history) {}

  static const CategorialWeight<T, C>& NoWeight() {
    Category<T> no_weight_cat(kCategoryBad);
    static const CategorialWeight<T, C> no_weight(no_weight_cat);
    return no_weight;
  }

  static const CategorialWeight<T, C>& Zero() {
    static const CategorialWeight<T, C> zero;
    return zero;
  }

  static const CategorialWeight<T, C>& One() {
    static const CategorialWeight<T, C> one(true);
    return one;
  }

  static const std::string& Type() {
    static const absl::NoDestructor<std::string> type(
        C == CategoryType::LEFT ? "category" : "right_category");
    return *type;
  }

  bool Member() const;
  std::istream& Read(std::istream& strm);
  std::ostream& Write(std::ostream& strm) const;
  size_t Hash() const;
  ReverseWeight Reverse() const;

  CategorialWeight<T, C> Quantize(float delta = kDelta) const { return *this; }

  static constexpr uint64_t Properties() {
    return ((C == CategoryType::LEFT ? kLeftSemiring : kRightSemiring) |
            kIdempotent | kPath);
  }

  const Category<T> Value() const { return value_; }

  const Category<T> History() const { return history_; }

 private:
  // The actual value of the weight
  Category<T> value_;
  // The construction history of the weight
  Category<T> history_;
};  // class CategorialWeight

template <typename L, CategoryType S>
inline std::istream& CategorialWeight<L, S>::Read(std::istream& strm) {
  value_.Clear();
  int32_t size;
  bool isone;
  ReadType(strm, &isone);
  ReadType(strm, &size);
  for (int i = 0; i < size; ++i) {
    L label;
    ReadType(strm, &label);
    value_.PushBack(label);
  }
  value_.set_one(isone);
  return strm;
}

template <typename L, CategoryType S>
inline std::ostream& CategorialWeight<L, S>::Write(std::ostream& strm) const {
  bool isone = value_.one();
  WriteType(strm, isone);
  int32_t size = value_.Size();
  WriteType(strm, size);
  for (CategoryIterator<L> iter(value_); !iter.Done(); iter.Next()) {
    L label = iter.Value();
    WriteType(strm, label);
  }
  return strm;
}

template <typename T, CategoryType C>
inline bool CategorialWeight<T, C>::Member() const {
  if (value_.Size() != 1) return true;
  CategoryIterator<T> iter(value_);
  return iter.Value() != kCategoryBad;
}

template <typename T, CategoryType C>
inline typename CategorialWeight<T, C>::ReverseWeight
CategorialWeight<T, C>::Reverse() const {
  ReverseWeight rw(ReverseCat(value_));
  return rw;
}

// This is the same as the hashing function for strings, except that we may have
// negative labels as low as kLeftDivision (= -6) so we increment by
// -kLeftDivision to avoid the obvious problem of the negatives being
// interpreted as huge numbers.
template <typename T, CategoryType C>
inline size_t CategorialWeight<T, C>::Hash() const {
  size_t h = 0;
  for (CategoryIterator<T> iter(value_); !iter.Done(); iter.Next())
    h ^= h << 1 ^ (iter.Value() - kLeftDivision);
  return h;
}

template <typename T, CategoryType C>
inline bool operator==(const CategorialWeight<T, C>& w1,
                       const CategorialWeight<T, C>& w2) {
  return Match(w1.Value(), w2.Value());
}

template <typename T, CategoryType C>
inline bool operator!=(const CategorialWeight<T, C>& w1,
                       const CategorialWeight<T, C>& w2) {
  return !(w1 == w2);
}

// TODO: Check why on very rare occasions the test fails here with a right
// categorial weight.
template <typename T, CategoryType C>
inline bool ApproxEqual(const CategorialWeight<T, C>& w1,
                        const CategorialWeight<T, C>& w2,
                        float delta = kDelta) {
  return w1 == w2;
}

// The stream operations are modeled closely on the StringWeight operations.
template <typename T, CategoryType C>
inline std::ostream& operator<<(std::ostream& strm,
                                const CategorialWeight<T, C>& w) {
  Category<T> v = w.Value();
  CategoryIterator<T> iter(v);
  if (w == CategorialWeight<T, C>::Zero()) {
    return strm << "Zero";
  } else if (w == CategorialWeight<T, C>::One()) {
    return strm << "One";
  } else if (iter.Value() == kCategoryBad) {
    return strm << "BadCategory";
  } else {
    for (size_t i = 0; !iter.Done(); ++i, iter.Next()) {
      if (i > 0) strm << '_';
      strm << iter.Value();
    }
  }
  return strm;
}

template <typename T, CategoryType C>
inline std::istream& operator>>(std::istream& strm, CategorialWeight<T, C>& w) {
  std::string s;
  strm >> s;
  if (s == "Zero") {
    w = CategorialWeight<T, C>::Zero();
  } else if (s == "One") {
    w = CategorialWeight<T, C>::One();
  } else if (s == "BadCategory") {
    Category<T> c(kCategoryBad);
    CategorialWeight<T, C> cw(c);
    w = cw;
  } else {
    Category<T> v;
    v.Clear();
    for (absl::string_view sv : absl::StrSplit(s, '_')) {
      auto maybe_label = ParseInt64(sv);
      if (!maybe_label.has_value()) {
        strm.clear(std::ios::badbit);
        break;
      }
      v.PushBack(*maybe_label);
    }
    CategorialWeight<T, C> cw(v);
    w = cw;
  }
  return strm;
}

// Plus is generic for both directions, since if it's a reverse weight it's
// reversed at the categorial level. Always picks the lexicographically less
// depending on the linearization of the weights. As described above, it uses
// history_ for comparison
template <typename T, CategoryType C>
inline CategorialWeight<T, C> Plus(const CategorialWeight<T, C>& w1,
                                   const CategorialWeight<T, C>& w2) {
  if (!w1.Member() || !w2.Member()) return CategorialWeight<T, C>::NoWeight();
  if (w1 == CategorialWeight<T, C>::Zero()) return w2;
  if (w2 == CategorialWeight<T, C>::Zero()) return w1;
  if (C == CategoryType::LEFT) {
    Category<T> w1_hist = w1.History();
    Category<T> w2_hist = w2.History();
    CategoryIterator<T> iter1(w1_hist);
    CategoryIterator<T> iter2(w2_hist);
    for (; !iter1.Done() && !iter2.Done(); iter1.Next(), iter2.Next())
      if (iter1.Value() < iter2.Value())
        return w1;
      else if (iter1.Value() > iter2.Value())
        return w2;
    if (!iter2.Done()) return w1;
    return w2;
  } else {  //  C == CategoryType::RIGHT
    Category<T> rw1_hist = ReverseCat(w1.History());
    Category<T> rw2_hist = ReverseCat(w2.History());
    CategoryIterator<T> iter1(rw1_hist);
    CategoryIterator<T> iter2(rw2_hist);
    for (; !iter1.Done() && !iter2.Done(); iter1.Next(), iter2.Next())
      if (iter1.Value() < iter2.Value())
        return w1;
      else if (iter1.Value() > iter2.Value())
        return w2;
    if (!iter2.Done()) return w1;
    return w2;
  }
}

// As described above, this reduces values, but concatenates histories.
template <typename T, CategoryType C>
inline CategorialWeight<T, C> Times(const CategorialWeight<T, C>& w1,
                                    const CategorialWeight<T, C>& w2) {
  Category<T> w1_value = w1.Value();
  Category<T> w2_value = w2.Value();
  Category<T> w1_hist = w1.History();
  Category<T> w2_hist = w2.History();
  if (!w1.Member() || !w2.Member()) return CategorialWeight<T, C>::NoWeight();
  if (w1 == CategorialWeight<T, C>::One()) return w2;
  if (w2 == CategorialWeight<T, C>::One()) return w1;
  if (w1 == CategorialWeight<T, C>::Zero()) {
    return CategorialWeight<T, C>::Zero();
  }
  if (w2 == CategorialWeight<T, C>::Zero()) {
    return CategorialWeight<T, C>::Zero();
  }
  if (C == CategoryType::LEFT) {
    w1_value.set_direction(kLeftDivision);
    w2_value.set_direction(kLeftDivision);
  } else {
    w1_value.set_direction(kRightDivision);
    w2_value.set_direction(kRightDivision);
  }
  return CategorialWeight<T, C>(Reduce(Concat(w1_hist, w2_hist)),  // value
                                Concat(w1_hist, w2_hist));         // history
}

// Division calls the underlying category division so it knows the inherent
// direction.
template <typename T, CategoryType C>
inline CategorialWeight<T, C> Divide(const CategorialWeight<T, C>& w1,
                                     const CategorialWeight<T, C>& w2,
                                     DivideType typ) {
  Category<T> w1_value = w1.Value();
  Category<T> w2_value = w2.Value();
  Category<T> w1_hist = w1.History();
  Category<T> w2_hist = w2.History();
  if (!w1.Member() || !w2.Member()) return CategorialWeight<T, C>::NoWeight();
  return CategorialWeight<T, C>(Division(w1_value, w2_value),  // value
                                Division(w1_hist, w2_hist));   // history
}

// This function object generates random categorial weights. This is primarily
// intended for testing.
template <typename L, CategoryType C>
class WeightGenerate<CategorialWeight<L, C>> {
 public:
  using Weight = CategorialWeight<L, C>;

  explicit WeightGenerate(bool allow_zero = true,
                          int alphabet_size = kNumRandomWeights,
                          int max_string_length = kNumRandomWeights,
                          int max_operations = kNumRandomWeights)
      : allow_zero_(allow_zero),
        alphabet_size_(alphabet_size),
        max_string_length_(max_string_length),
        max_operations_(max_operations) {}

  Weight operator()(absl::BitGenRef bit_gen) const {
    int n = absl::Uniform(bit_gen, 0, max_string_length_ + allow_zero_);
    int n_op =
        absl::Uniform(absl::IntervalClosedClosed, bit_gen, 1, max_operations_);
    if (allow_zero_ && n == max_string_length_) return Weight::Zero();
    // Vector of random operations, filled in as below.
    std::vector<int> rand_operation(n_op);
    // Vector of random simplex categories.
    std::vector<std::vector<L>> v(n_op);
    // Generates n_op random operations on n random categories.
    for (int j = 0; j < n_op; j++) {
      // rand_operation[k] = 0 -> Concat()
      // rand_operation[k] = 1 -> Division()
      rand_operation[j] = absl::Bernoulli(bit_gen, 0.5);
      for (int i = 0; i < n; ++i) {
        v[j].push_back(absl::Uniform<L>(absl::IntervalClosedClosed, bit_gen, 1,
                                        alphabet_size_));
      }
    }
    int dir = kLeftDivision;
    if (C == CategoryType::RIGHT) dir = kRightDivision;
    // Constructs a complex category out of the simplex categories using the
    // randomly selected operations.
    Category<L> w_cat(v[0].begin(), v[0].end(), dir);
    for (int j = 1; j < n_op; j++) {
      // Random operation to perform is concatenation.
      if (rand_operation[j] == 0) {
        w_cat = Concat(w_cat, Category<L>(v[j].begin(), v[j].end(), dir));
      } else if (!v[j].empty()) {
        // Random operation to perform is division, but we must first check
        // to see if we'd be dividing by zero.
        w_cat = Division(w_cat, Category<L>(v[j].begin(), v[j].end(), dir));
      }
    }
    return Weight(w_cat);
  }

 private:
  // Permits Zero() and zero divisors.
  bool allow_zero_;
  // Alphabet size for random weights.
  const int alphabet_size_;
  // Alphabet size for random weights.
  const int max_string_length_;
  // Number of alternative random operations.
  const int max_operations_;
};

}  // namespace fst

#endif  // OPENFST_EXTENSIONS_CATEGORIAL_CATEGORIAL_WEIGHT_H_
