# OpenFst Advanced Usage

[TOC]

Below are a variety of topics covered in greater depth or of more specialized
interest than found in the Quick Tour. Reading the [Quick Tour](quick_tour.md)
first is recommended.

## Arc Iterators

An arc iterator
[`ArcIterator`](https://www.openfst.org/doxygen/fst/html/classfst_1_1ArcIterator.html)
is used to access the transitions leaving an FST state. It has the form:

```cpp
template <class F>
class ArcIterator {
  typedef typename F::Arc Arc;
  typedef typename Arc::StateId StateId;

 public:
  ArcIterator(const &F fst, StateId s);
  // End of iterator?
  bool Done() const;
  // Current arc (when !Done)
  const Arc& Value() const;
  // Advances to next arc (when !Done)
  void Next();
  // Returns current position
  size_t Position();
  // Returns to initial position
  void Reset();
  // Arc access by position
  void Seek(size_t pos);
  // Returns arc flags
  uint32 Flags() const;
  // Sets arc flags
  void SetFlags(uint32 flags, uint32 mask);
};
```

It is templated on the Fst class `F` to allow efficient specializations but
defaults to a generic version on the abstract base
[`Fst`](advanced_usage.md#base-fsts) class.

See [here](conventions.md) for conventions that arc iterator use must respect.

All current OpenFst library `Seek()` methods are constant time.

An example use of an arc iterator is shown [here](quick_tour.md#arc-iterators).

A `MutableArcIterator`
[`MutableArcIterator`](https://www.openfst.org/doxygen/fst/html/classfst_1_1MutableArcIterator.html)
is similar to an `ArcIterator` except its constructor takes a pointer to a
`MutableFst` and it additionally has a `SetValue()` method.

## Arc Filters

Arc filters are accepted by various operations to control which arcs are
transitioned. An arc filter has the form:

```cpp
template <class Arc>
class SomeArcFilter {
public:
  // Return true iff arc is to be transitioned.
  bool operator()(const Arc &arc) const;
};
```

Pre-defined arc filters include:

Name                     | Description                                     |     |
------------------------ | ----------------------------------------------- | ---
`AnyArcFilter`           | Accept all arcs                                 | [`AnyArcFilter`](https://www.openfst.org/doxygen/fst/html/classfst_1_1AnyArcFilter.html)
`EpsilonArcFilter`       | Accept only arcs with input and output epsilons | [`AnyArcFilter`](https://www.openfst.org/doxygen/fst/html/classfst_1_1AnyArcFilter.html)
`InputEpsilonArcFilter`  | Accept only arcs with input epsilons            | [`InputEpsilonArcFilter`](https://www.openfst.org/doxygen/fst/html/classfst_1_1InputEpsilonArcFilter.html)
`OutputEpsilonArcFilter` | Accept only arcs with output epsilons           | [`InputEpsilonArcFilter`](https://www.openfst.org/doxygen/fst/html/classfst_1_1InputEpsilonArcFilter.html)

## Arc Mappers

*Arc mappers* are function objects used by the [ArcMap](arc_map.md) operation to
transform arcs and/or final states. An arc mapper has the form:

```cpp
// This determines how final weights are mapped.
enum MapFinalAction {
  // A final weight is mapped into a final weight. An error
  // is raised if this is not possible.
  MAP_NO_SUPERFINAL,

  // A final weight is mapped to an arc to the superfinal state
  // when the result cannot be represented as a final weight.
  // The superfinal state will be added only if it is needed.
  MAP_ALLOW_SUPERFINAL,

  // A final weight is mapped to an arc to the superfinal state
  // unless the result can be represented as a final weight of weight
  // Zero(). The superfinal state is always added (if the input is
  // not the empty Fst).
  MAP_REQUIRE_SUPERFINAL
};

// This determines how symbol tables are mapped.
enum MapSymbolsAction {
  // Symbols should be cleared in the result by the map.
  MAP_CLEAR_SYMBOLS,

  // Symbols should be copied from the input FST by the map.
  MAP_COPY_SYMBOLS,

  // Symbols should not be modified in the result by the map itself.
  // (They may set by the mapper).
  MAP_NOOP_SYMBOLS
};

class SomeArcMapper {
  public:
   // Assumes input arc type is A and result arc type is B
   typedef A FromArc;
   typedef B ToArc;

   // Maps an arc type A to arc type B.
   B operator()(const A &arc);
   // Specifies final action the mapper requires (see above).
   // The mapper will be passed final weights as arcs of the
   // form A(0, 0, weight, kNoStateId).
   MapFinalAction FinalAction() const;
   // Specifies input symbol table action the mapper requires (see above).
   MapSymbolsAction InputSymbolsAction() const;
   // Specifies output symbol table action the mapper requires (see above).
   MapSymbolsAction OutputSymbolsAction() const;
   // This specifies the known properties of an Fst mapped by this
   // mapper. It takes as argument the input Fst's known properties
   uint64 Properties(uint64 props) const;
};
```

The following arc mappers are defined in the OpenFst library:

Name                  | Description                                                                                                                    |     |
--------------------- | ------------------------------------------------------------------------------------------------------------------------------ | ---
`FromGallicMapper`    | Extracts output label from [gallic](advanced_usage.md#weights) weight                                                          | [`FromGallicMapper`](https://www.openfst.org/doxygen/fst/html/classfst_1_1FromGallicMapper.html)
`IdentityArcMapper`   | Maps to self                                                                                                                   | [`IdentityArcMapper`](https://www.openfst.org/doxygen/fst/html/classfst_1_1IdentityArcMapper.html)
`InvertWeightMapper`  | Reciprocate all non-0 weights                                                                                                  | [`InvertWeightMapper`](https://www.openfst.org/doxygen/fst/html/classfst_1_1InvertWeightMapper.html)
`PlusMapper`          | Adds (⊕) a constant to all weights                                                                                             | [`PlusMapper`](https://www.openfst.org/doxygen/fst/html/classfst_1_1PlusMapper.html)
`QuantizeMapper`      | Quantize all weights                                                                                                           | [`QuantizeMapper`](https://www.openfst.org/doxygen/fst/html/classfst_1_1QuantizeMapper.html)
`ReverseWeightMapper` | [Reverse](weight_requirements.md) all weights                                                                                  | [`ReverseWeightMapper`](https://www.openfst.org/doxygen/fst/html/classfst_1_1ReverseWeightMapper.html)
`RmWeightMapper`      | Map all non-0 weights to 1                                                                                                     | [`RmWeightMapper`](https://www.openfst.org/doxygen/fst/html/classfst_1_1RmWeightMapper.html)
`SuperFinalMapper`    | Redirects final states to new superfinal state                                                                                 | [`SuperFinalMapper`](https://www.openfst.org/doxygen/fst/html/classfst_1_1SuperFinalMapper.html)
`TimesMapper`         | (Right) multiplies (⊗) a constant to all weights                                                                               | [`TimesMapper`](https://www.openfst.org/doxygen/fst/html/classfst_1_1TimesMapper.html)
`ToGallicMapper`      | Combines output label and weight into [gallic](advanced_usage.md#weights) weight                                               | [`ToGallicMapper`](https://www.openfst.org/doxygen/fst/html/classfst_1_1ToGallicMapper.html)
`WeightConvertMapper` | Converts arc weight types (assuming appropriate `WeightConvert` class specialization), leaving labels and nextstates the same. | [`WeightConvertMapper`](https://www.openfst.org/doxygen/fst/html/classfst_1_1WeightConvertMapper.html)

`ToGallicMapper` and `FromGallicMapper` are used, for example, to implement
transducer [determinization](determinize.md) and [minimization](minimize.md)
using weighted acceptor versions of these algorithms. Other specialized arc
mappers are used to implement [Decode](encode_decode.md),
[Encode](encode_decode.md), [Invert](invert.md), and [Project](project.md).

## Arcs

An `Arc` is a type that represents an FST transition from a given source state.
It specifies an input label, an output label, a weight, and a destination state
ID and it has a type name. In particular, it has the following form:

```cpp
struct SomeArc {
   typedef W Weight;
   typedef L Label;
   typedef S StateId;

   static const string &Type();

   Label ilabel;
   Label olabel;
   Weight weight;
   StateId nextstate;
};
```

where `W` is a valid [weight type](weight_requirements.md), and `L` and `S` are
signed integral types.

The following arc types are defined in the OpenFst library:

Name                       | Label Type | State ID Type | Weight Type                            | Registered
-------------------------- | ---------- | ------------- | -------------------------------------- | ----------
`ExpectationArc<A, W>`     | `int`      | `int`         | `ExpectationWeight<A::Weight, W>`      |
`GallicArc<A, S>`          | `A::Label` | `A::StateId`  | `GallicWeight<A::Label, A::Weight, S>` |
`LexicographicArc<W1, W2>` | `int`      | `int`         | `LexicographicWeight<W1, W2>`          |
`LogArc`                   | `int`      | `int`         | `LogWeight`                            | ✓
`Log64Arc`                 | `int`      | `int`         | `Log64Weight`                          | ✓
`MinMaxArc`                | `int`      | `int`         | `MinMaxWeight`                         |
`PowerArc<A, n>`           | `int`      | `int`         | `PowerWeight<A::Weight, n>`            |
`ProductArc<W1, W2>`       | `int`      | `int`         | `ProductWeight<W1, W2>`                |
`SignedLogArc`             | `int`      | `int`         | `SignedLogWeight`                      | ✓
`SignedLog64Arc`           | `int`      | `int`         | `SignedLog64Weight`                    | ✓
`SparsePowerArc<A>`        | `int`      | `int`         | `SparsePowerWeight<A::Weight>`         |
`StdArc`                   | `int`      | `int`         | `TropicalWeight`                       | ✓
`StringArc<S>`             | `int`      | `int`         | `StringWeight<int, S>`                 |

Additional arc information:

*   [Corresponding weight types](advanced_usage.md#weights)
*   [Elementary arc information](quick_tour.md#std-arc)
*   [Fst I/O:](advanced_usage.md#input-output) How to register arc types for
    I/O.
*   [User-defined arcs](advanced_usage.md#user-defined-arcs-and-weights)

## Base FSTs

Every [`Fst`](https://www.openfst.org/doxygen/fst/html/classfst_1_1Fst.html)
must specify an initial state, the final weights, arc and epsilon counts per
states, an Fst type name, the Fst's [properties](advanced_usage.md#properties),
how to copy, read and write the Fst, and the input and output symbol tables (if
any). In particular, the base `Fst` class has the interface:

```cpp
template <class A>
class Fst {
 public:
  typedef A Arc;
  typedef typename A::Weight Weight;
  typedef typename A::StateId StateId;

  // Initial state
  virtual StateId Start() const = 0;
  // States's final weight
  virtual Final(StateId) const = 0:
  // State's arc count
  virtual NumArcs(StateId) const = 0;
  // States's input epsilon count
  virtual NumInputEpsilons(StateId) const = 0;
  // State's output epsilon count
  virtual NumOutputEpsilons(StateId) const = 0;
  // If test=false, return stored properties bits for mask (some poss. unknown)
  // If test=true, return property bits for mask (computing o.w. unknown)
  virtual Properties(uint64 mask, bool test) const = 0;
  // Fst type name
  virtual const string& Type() const = 0;
  // Get a copy of this Fst
  virtual Fst<A> *Copy() const = 0;
  // Read an Fst from an input stream; returns NULL on error
  static Fst<A> *Read(istream &strm, const FstReadOptions &opts);
  // Read an Fst from a file; return NULL on error
  // Empty filename reads from standard input
  static Fst<A> *Read(const string &filename);
  // Write an Fst to an output stream; return false on error
  virtual bool Write(ostream &strm, const FstWriteOptions &opts);
  // Write an Fst to a file; return false on error
  // Empty filename writes to standard output
  virtual bool Write(const string &filename);
  // Return input label symbol table; return NULL if not specified
  virtual const SymbolTable* InputSymbols() const = 0;
  // Return output label symbol table; return NULL if not specified
  virtual const SymbolTable* OutputSymbols() const = 0;
};
```

`Fst` is an abstract class (note the pure virtual methods). All OpenFst FSTs
must meet this interface.

The companion [state iterator](advanced_usage.md#state-iterators) and
[arc iterator](advanced_usage.md#arc-iterators) classes provide access to the
states and transitions of the FST.

## Caching

Most of the [delayed Fst classes](quick_tour.md#delayed-fsts) use internal
caching to save expanded states and arcs. This caching is controlled by this
struct:

```cpp
struct CacheOptions {
  // enable GC
  bool gc;
  // # of bytes allowed before GC
  size_t gc_limit;

  CacheOptions(bool g, size_t l) : gc(g), gc_limit(l) {}
  CacheOptions()
      : gc(FLAGS_fst_default_cache_gc),
        gc_limit(FLAGS_fst_default_cache_gc_limit) {}
};
```

All OpenFst cached Fsts have constructors that accept this (or a class derived
from it) as an argument. The member defaults are controlled by
[global flags](advanced_usage.md#command-line-flags). These options can be used
for:

*   *Maximal caching*: If `gc` is `false`, then any expanded state will be
    cached for the extent of the FST. This case is useful when states are
    revisited and memory is not a concern.

*   *Bounded caching*: If `gc` is `true`, then the cache will be
    garbage-collected when it grows past `gc_limit`. This case is useful when
    states are revisited and memory is a concern. This is the default case
    (based on the global flags).

*   *Minimal caching*: It is generally not possible to avoid all caching in such
    an FST since the cache is used to implement the arc iterators efficiently
    (creating an iterator computes and writes the state's arcs to the cache,
    iterating reads from the cache). However, if (1) `gc` is `true`, (2)
    `gc_limit` is 0, and (3) arcs iterators have been created (and then
    destroyed) *only one state at a time*, then only information for that state
    is cached and this case is especially optimized. This case is useful when
    states are not revisited (e.g. when a cached FST is simply being copied to a
    mutable FST).

The default cache storage (*VectorCacheStore*) maintains a vector of state
pointers; the pointed to objects are GCed but the vector is never shrunk (see
*cache.h* for other cache stores). Also delayed operations typically maintain an
internal state table between the output state and the essential computation
state (such as the corresponding state pair in composition) that is not GCed
(since the consumer may come back to that state). As such, delayed operations
that continue to grow in states will continue to consume memory. Some key
operations, like *ComposeFst*, let you change the cache store and the state
table to allow customization such as described
[here](https://static.googleusercontent.com/media/research.google.com/en//pubs/archive/41410.pdf).

## Command Line Flags

OpenFst has several global options in the library proper that most users can
ignore, leaving them with their default values:

Option                             | Type     | Default   | Description
---------------------------------- | -------- | --------- | -----------
`FLAGS_fst_compat_symbols`         | `bool`   | `true`    | Require symbol tables to match when appropriate
`FLAGS_fst_default_cache_gc`       | `bool`   | `true`    | Enable garbage collection of cached Fsts
`FLAGS_fst_default_cache_gc_limit` | `int64`  | `1048576` | Byte size that triggers garbage collection of cached Fsts
`FLAGS_fst_error_fatal`            | `bool`   | `true`    | FST errors are fatal; o.w. return objects flagged as bad: e.g., FSTs - `kError` prop. true, FST weights - not a `Member()`
`FLAGS_fst_field_separator`        | `string` | `" \t"`   | Set of characters used as a separator between printed fields
`FLAGS_fst_weight_parentheses`     | `string` | `""`      | Characters enclosing the first weight of a printed composite weight (and derived classes) to ensure proper I/O of nested composite weights; must have size 0 (none) or 2 (open and close parenthesis)
`FLAGS_fst_weight_separator`       | `string` | `","`     | Character separator between printed composite weights; must be a single character
`FLAGS_fst_verify_properties`      | `bool`   | `false`   | Verify Fst properties are correctly set when queried

The first ensures the arguments of binary FST operations (e.g.
[composition](compose.md)) have compatible symbol tables (e.g. output symbol
table matches input symbol table for composition). The second two are used to
control the [caching](advanced_usage.md#caching) of expanded state and arc
information found in most [delayed Fst classes](quick_tour.md); the default
values should normally be satisfactory. The next determines how
[errors are handled](advanced_usage.md#error-handling). The next is used in the
textual representation of FSTs and symbol tables. The next two are used to
control the text formating of [`ProductWeight`](advanced_usage.md#weights) and
other weight tuples. The last is used to ensure that the
[properties](advanced_usage.md#properties) of an FST have been correctly set; it
is used for debugging only since it incurs considerable computational cost.

In each of the Fst distribution installed binaries, the above options, as well
as any of those defined specific to the binary, can be set from the command line
using e.g. `--fst_default_cache_gc=false` or `--fst_weight_parenthesis="("` .
Additionally, the option `--help` and `--v=N` (where N = 0,1,2,..) will print
out usage information and set the verbosity level of logging, respectively. The
flag processing is modeled after the Google
[gflags](https://code.google.com/p/google-gflags) package.

In a user-defined binary, the command line options processing will all also work
if the user calls:

```txt
SetFlags(usage, &argc, &argv, true);
```

In that case, the user can set his own flags as well, following the conventions
in
[`<fst/flags.h>`](https://www.openfst.org/doxygen/fst/html/flags_8h_source.html).

Alternatively, the user can process options in his own way and directly assign
to any of the above global options if he wishes to modify their defaults.

## Composition Filters

A *composition filter* determines which matches are allowed to proceed in
[composition](compose.md). The basic filters handle correct epsilon matching. In
particular, they ensure that redundant epsilon paths, which would be incorrect
with [non-idempotent](weight_requirements.md) weights, are not created. More
generally, composition filters can be used to block or modify composition paths
for efficiency or other purposes usually working in tandem with specialized
[matchers](advanced_usage.md#look-ahead-matchers). Their interface is:

```cpp
template <class M1, M2>
class SomeComposeFilter {
 public:
    typedef typename M1::FST1 FST1;
    typedef typename M1::FST2 FST2;
    typedef typename FST1::Arc Arc;
    typedef ... FilterState;
    typedef ... Matcher1;
    typedef ... Matcher2;
    typedef ... FilterState;

    typedef typename Arc::StateId StateId;
    typedef typename Arc::Weight Weight;

   // Required constructor. The filter is either passed composition matchers or constructs
   // them internally. This is done so the filter can possibly modify the result (useful e.g. with lookahead).
   SomeComposeFilter(const FST1 &fst1, const FST2 &fst2, M1 *matcher1 = 0, M2 *matcher2);
   // Return start state of filter.
   FilterState Start() const;
   // Specifies current composition state.
   void SetState(StateId s1, StateId s2, const FilterState &f);
   Matcher2 *GetMatcher2();

   // Apply filter at current composition state to these transitions.
   // If an arc label to be matched is kNolabel, then that side does not consume a symbol.
   // Returns the new filter state or, if disallowed, FilterState::NoState().
   // The filter is permitted to modify its inputs, e.g. for optimizations.
   FilterState FilterArc(A *arc1, A *arc2) const;

   // Apply filter at current composition state to these final weights
   // (cf. superfinal transitions). The filter may modify its inputs,
   // e.g. for optimizations.
   void FilterFinal(Weight *final1, Weight *final2) const;

   // Return resp matchers. Ownership stays with the filter. These
   // methods allow the filter to access and possibly modify
   // the composition matchers (useful e.g. with lookahead).
   Matcher1 *GetMatcher1();
};
```

The filter's state is represented by the type `SomeComposeFilter::FilterState`
and is stored in the composition [state table](advanced_usage.md#state-tables)
tuple. It has the form:

```cpp
class SomeFilterState {
  public:
   // Required constructors
   SomeFilterState();
   SomeFilterState(const SomeFilterState &f);
   // An invalid filter state.
   static const SomeFilterState NoState();
   // Maps state to integer for hashing.
   size_t Hash() const;
   // Equality of filter states.
   bool operator==(const SomeFilterState &f) const;
   // Inequality of filter states.
   bool operator!=(const SomeFilterState &f) const;
   // Assignment to filter states.
   SomeFilterState& operator=(const SomeFilterState& f);
};
```

The following composition filters are defined in the OpenFst library:

Name                       | Description                                                                     |     |
-------------------------- | ------------------------------------------------------------------------------- | ---
`SequenceComposeFilter`    | Requires epsilons on FST1 to be read before epsilons on FST2                    | [`SequenceComposeFilter`](https://www.openfst.org/doxygen/fst/html/classfst_1_1SequenceComposeFilter.html)
`AltSequenceComposeFilter` | Requires epsilons on FST2 to be read before epsilons on FST1                    | [`AltSequenceComposeFilter`](https://www.openfst.org/doxygen/fst/html/classfst_1_1AltSequenceComposeFilter.html)
`MatchComposeFilter`       | Requires epsilons on FST1 to be matched with epsilons on FST2 whenever possible | [`MatchComposeFilter`](https://www.openfst.org/doxygen/fst/html/classfst_1_1MatchComposeFilter.html)
`LookAheadComposeFilter`   | Used with a lookahead matcher to block non-coaccessible paths                   | [`LookAheadComposeFilter`](https://www.openfst.org/doxygen/fst/html/classfst_1_1LookAheadComposeFilter.html)
`PushWeightsComposeFilter` | Adds weight-pushing to a lookahead composition filter                           | [`PushWeightsComposeFilter`](https://www.openfst.org/doxygen/fst/html/classfst_1_1PushWeightsComposeFilter.html)
`PushLabelsComposeFilter`  | Adds label-pushing to a lookahead composition filter                            | [`PushLabelsComposeFilter`](https://www.openfst.org/doxygen/fst/html/classfst_1_1PushLabelsComposeFilter.html)

`SequenceComposeFilter` is the default composition filter. It can be
[changed](advanced_usage.md#operation-options) by using the version of
`ComposeFst` that accepts `ComposeFstOptions`.

See [lookahead matchers](advanced_usage.md#look-ahead-matchers) for more
information about composition with lookahead.

## Error Handling

If `FLAGS_fst_error_fatal` is true (the default), then most serious errors cause
program exit. The exception are most functions and methods that return NULL, a
boolean, or `NoWeight()` on error - typically I/O and weight operations. If
`FLAGS_fst_error_fatal` is false, then no operation is fatal. In that case,
operations that return FSTs set the `kError` property bit. Otherwise classes
have an Error() method that should be checked and functions return a boolean. It
is intended that a sequence of operations preserve the `kError` property bit and
the `NoWeight()`, so that it should suffice to check for error at the end of the
sequence.

## Expanded FSTs

An `ExpandedFst`
[`ExpandedFst`](https://www.openfst.org/doxygen/fst/html/classfst_1_1ExpandedFst.html)
is an [`Fst`](advanced_usage.md#base-fsts) that has an additional method that
specifies the state count as well as methods to copy and read the expanded FST.
In particular, an `ExpandedFst` class has the interface:

```cpp
template <class A>
class ExpandedFst : public Fst<class A> {
 public:
  typedef A Arc;
  typedef typename A::StateId StateId;

  // State count
  StateId NumStates();
  // Get a copy of this ExpandedFst
  virtual ExpandedFst<A> *Copy() const = 0;
  // Read an ExpandedFst from an input stream; returns NULL on error
  static ExpandedFst<A> *Read(istream &strm, const FstReadOptions &opts);
  // Read an ExpandedFst from a file; return NULL on error
  // Empty filename reads from standard input
  static ExpandedFst<A> *Read(const string &filename);
};
```

`ExpandedFst` is an abstract class (note the pure virtual methods). Examples are
`VectorFst`
[`VectorFst`](https://www.openfst.org/doxygen/fst/html/classfst_1_1VectorFst.html)
and `ConstFst`
[`ConstFst`](https://www.openfst.org/doxygen/fst/html/classfst_1_1ConstFst.html)

## FST Input/Output <a id="input-output"></a>

The following describes methods for reading and writing binary file
representations of FSTS. Note these binary file representations are
[machine architecture dependent](https://en.wikipedia.org/wiki/Endianness); use
the [textual](quick_tour.md#fstcompile) file format cross-platform independence.

The code:

```txt
VectorFst<Arc> ifst;
...
ifst.Write("a.fst");
VectorFst<Arc> *ofst = VectorFst<Arc>::Read("a.fst");
```

writes and reads a defined FST type (`VectorFst`) and arc type (`Arc`) to and
from a file in a straight-forward way.

### Library Registration

The call:

```txt
Fst<Arc> *fst = Fst<Arc>::Read("a.fst");
```

reads the same `VectorFst` from the file as above, but returns a base `Fst`.
This form, useful for code that works generically for different FST types, can
not work unless the Fst and arc type are appropriately *registered*. Some arc
types (see [here](advanced_usage.md#arcs)) are already registered for common FST
types defined in the OpenFst library. Other arc type `Arc` and Fst type `F`
pairs can be registered with the following call:

```txt
REGISTER_FST(F, Arc);
```

To avoid code bloat in a given program, registering arc types, in particular,
should be used sparingly.

### Script Registration

In the above examples, the user provided the arc type as a template parameter.
However, the call:

```bash
$ fstdeterminize in.fst >out.fst
```

works e.g. for both `StdArc` and `LogArc` arcs. This is accomplished by calling
in `main(argc, argv)`:

```cpp
namespace script {
FstClass *ifst = FstClass::Read(in_name);
VectorFstClass ofst(ifst->ArcType());
Determinize(*ifst, &ofst);
ofst.Write(out_name);
}
```

where:

```cpp
class VectorFstClass;
void Determinize(const FstClass &ifst, MutableFstClass *ofst);
```

are a class and function in the [fst:script](advanced_usage.md#fstscript)
namespace that do not depend on the `Arc` template parameter. These forms,
useful for code that works generically for different Arc types, can not work
unless the arc type is appropriately *registered*. Some arc types (see
[here](advanced_usage.md#arcs)) are already registered. Other arc types `Arc`
can be registered with the following calls:

```txt
REGISTER_FST_CLASS(VectorFstClass, Arc);
REGISTER_FST_OPERATION(Determinize, Arc, DeterminizeArgs);
```

If `Arc` defines a new weight type, it can be registered at the script level
(enabling [WeightClass](advanced_usage.md#weightclass) support) with the call:

```txt
REGISTER_FST_WEIGHT(Arc::Weight);
```

To avoid code bloat in a given program, registering arc types should be used
sparingly.

### FST Dynamic Shared Objects

The examples above show how users can modify programs to be able to read new arc
and FST types. However, it would not be ideal to have to do so for all the
distribution binaries or other existing programs. Instead, this can be done more
easily with *dynamic shared objects (DSOs)*.

To add a new Fst type, `MyFst` with `MyFst::Type()` = `"my_fst"`, use the code:

```txt
// Register some arc types with this Fst type
REGISTER_FST(MyFst, StdArc);
REGISTER_FST(MyFst, LogArc);
```

compiled into a dynamic shared object `my_fst.so`. If `my_fst.so` can be found
in the `LD_LIBRARY_PATH` (or equivalent), you should be able to read the new Fst
type with existing programs.

To add a new arc type, `MyArc` with `MyArc::Type()` = `"my_arc"`, use the code:

```txt
// Register some FST types with this arc type
REGISTER_FST(VectorFst, MyArc);
REGISTER_FST(ConstFst, MyArc);

// Register the fst::script operations with this arc type
REGISTER_FST_OPERATIONS(MyArc);
// Register some other operation with this arc type
REGISTER_FST_OPERATION(Operations, MyArc, Args);
```

compiled into a dynamic shared object `my_arc.so`. If can be found in
`LD_LIBRARY_PATH` (or equivalent), you should be able to read the new arc type
with existing programs.

## fst::script

The OpenFst library offers an additional abstraction layer that facilitates
scripting with FSTs of different arc types. It allows users to load FSTs and
perform various operations on them without specifying the underlying FST arc
types. This higher-level layer, found in the `fst::script` namespace, is
somewhat less efficient, due to indirection, and in general exposes fewer
methods and options. It is used principally to implement the shell-level FST
commands and [Python bindings](python_extension.md). From C++, users are
encouraged in most circumstances to use the lower-level templated classes and
operations for their efficiency and completeness. However, for simple 'glue'
code that will work seamlessly with different arc types (esp. I/O), this
higher-level might be appropriate. To use the scripting layer, include
`<fst/fstscript.h>` in the installation include directory and link to
`libfstscript.{a,so}` in the installation library directory. The
[extension](extensions.md) libraries work similarly; e.g. for the FAR extension,
include `<fst/extension/farscript.h>` and link to `lib/libfstfarscript.{a,so}`.

In the following, all classes and methods mentioned are in the namespace
`fst::script`.

### Class Overview

At FST script level, the `Fst<Arc>` class
[template hierarchy](quick_tour.md#calling-fst-operations-from-c) is partially
mirrored with a template-free `FstClass` hierarchy. For example, these classes
are defined:

#### FstClass

```cpp
class FstClass {
 public:
  // Construct an FstClass from a templated Fst, hiding its arc type.
  template<class Arc>
  explicit FstClass(const Fst<Arc> &fst);
  // Copy constructor
  explicit FstClass(const FstClass &other);
  // Read an arc-templated Fst from disk, and return as an FstClass
  static FstClass *Read(const string &fname);
  // String representation of the arc type
  virtual const string &ArcType() const;
  // String representation of the underlying Fst type (e.g. 'vector')
  virtual const string &FstType() const;
  // String representation of the arc's weight type
  virtual const string &WeightType() const;
  // A pointer to this Fst's input symbol table
  virtual const SymbolTable *InputSymbols() const;
  // A pointer to this Fst's output symbol table
  virtual const SymbolTable *OutputSymbols() const;
  // Write the underlying arc-templated Fst to disk
  virtual void Write(const string &fname);
  // Return an integer representing all the properties (see Fst::Properties)
  virtual uint64 Properties(uint64 mask, bool test) const;
  // Call to get the underlying FST, if you know the concrete arc type
  // e.g. for an FstClass fc,
  // const Fst<StdArc> &f = *(fc.GetFst<StdArc>());
  // Returns NULL if the given arc type doesn't match the underlying FST.
  template<class Arc>
  const Fst<Arc> *GetFst() const;

  virtual ~FstClass();
}
```

Unlike its lower-level analogue `Fst<Arc>`, `FstClass` is not abstract; it is a
container which can be constructed from an arbitrary `Fst<Arc>`.

#### MutableFstClass

```cpp
class MutableFstClass : public FstClass {
 public:
  // Construct a MutableFstClass from some kind of MutableFst<>
  template<class Arc>
  explicit MutableFstClass(const MutableFst<Arc> &fst);
  // If your code knows the arc type of the underlying MutableFst<>, it
  // can use this method to extract a pointer to it. This pointer can be used
  // to change the underlying MutableFst<>
  template<class Arc>
  MutableFst<Arc> *GetMutableFst();
  // Set the input symbol table of the underlying MutableFst
  virtual void SetInputSymbols(SymbolTable *is);
  // Set the output symbol table of the underlying MutableFst
  virtual void SetOutputSymbols(SymbolTable *os);
};
```

#### VectorFstClass

```cpp
class VectorFstClass : public MutableFstClass {
public:
  // Construct a copy of "other" as a VectorFstClass
  explicit VectorFstClass(const FstClass &other);
  // Construct a blank VectorFstClass with the given arc type
  explicit VectorFstClass(const string &arc_type);
  // Wrap the given VectorFst<Arc>
  template<class Arc>
  explicit VectorFstClass(VectorFst<Arc> *fst);
};
```

#### WeightClass

This class hides the weight type similar to how the classes above hide the arc
type of an FST. It is useful for weight I/O and for passing weights into the
operations that require them.

```cpp
class WeightClass {
 public:
  // Construct a Zero
  WeightClass();
  // Wrap a weight of the given type
  template<class W>
  explicit WeightClass(const W &weight);
  // Construct a weight given the string representation of its type
  // (e.g. "tropical") and a string representation of the weight
  // itself.
  WeightClass(const string &weight_type, const string &weight_str);
  // Copy constructor and assign
  WeightClass(const WeightClass &other);
  WeightClass &operator = (const WeightClass &other);
  // If you know the correct weight type, you can get it with this
  method. Will return NULL if an incorrect type is attempted.
  template<class W>
  W *GetWeight() const;
  // Constants representing zero and one in all possible weight types
  static const WeightClass &Zero();
  static const WeightClass &One();

  ~WeightClass();
};
```

### Operations

In general, many of the [operations](quick_tour.md#available-fst-operations)
that are implemented for the underlying templated FSTs are implemented for
instances of `FstClass`, sometimes with modified option lists. Check
`<fst/fstscript.h>`.

### Example

The code in the [quick tour](quick_tour.md#fst-application-from-c) could be
reimplemented using the `fst::script` library as follows. This new version will
work with FSTs of all arc types, not just `StdArc` (though the arc types in
`input.fst` and `model.fst` must match).

```txt
// Reads in an input FST.
FstClass *input = FstClass::Read("input.fst");
// Reads in the transduction model.
FstClass *model = FstClass::Read("model.fst");

// The FSTs must be sorted along the dimensions they will be joined.
// In fact, only one needs to be so sorted.
// This could have instead been done for "model.fst" when it was created.
ArcSort(input, OLABEL_COMPARE);
ArcSort(model, ILABEL_COMPARE);
// Container for composition result.
VectorFstClass result(input->ArcType());
// Create the composed FST.
Compose(*input, *model, &result);
// Just keeps the output labels.
Project(&result,  PROJECT_OUTPUT);
```

## FST Types

The following non-abstract FST types with file representations are defined in
the OpenFst library:

Name                                    | Usage                                                  | Description                                                      | Registered                                                                |     |
--------------------------------------- | ------------------------------------------------------ | ---------------------------------------------------------------- | ------------------------------------------------------------------------- | ---
vector                                  | `VectorFst<A>`                                         | General-purpose mutable FST                                      | `libfst.{a,so}`                                                           | [`VectorFst`](https://www.openfst.org/doxygen/fst/html/classfst_1_1VectorFst.html)
const                                   | `ConstFst<A>`                                          | General-purpose expanded, immutable FST (# arcs < $2^{32}$)    | `libfst.{a,so}`                                                           | [`ConstFst`](https://www.openfst.org/doxygen/fst/html/classfst_1_1ConstFst.html)
constN, N=8,16,64                       | `ConstFst<A, uintN>`                                   | General-purpose expanded, immutable FST (# arcs < $2^N$)       | `fst/libfstconst.{a,so}`, `fst/constN-fst.so`                             | [`ConstFst`](https://www.openfst.org/doxygen/fst/html/classfst_1_1ConstFst.html)
compact_string                          | `CompactFst<A, StringCompactor<A>>`                    | Compact, immutable, unweighted, string FST (# arcs < $2^{32}$) | `libfst.{a,so}`                                                           | [`CompactFst`](https://www.openfst.org/doxygen/fst/html/classfst_1_1CompactFst.html)
compactN_string, N=8,16,64              | `CompactFst<A, StringCompactor<A>, uintN>`             | Compact, immutable, unweighted string FST (# arcs < $2^N$)     | `fst/libfstcompact.{a,so}`, `fst/compactN-fst.{a,so}`                     | [`CompactFst`](https://www.openfst.org/doxygen/fst/html/classfst_1_1CompactFst.html)
compact_weighted_string                 | `CompactFst<A, WeightedStringCompactor<A>>`            | Compact, immutable, weighted, string FST (# arcs < $2^{32}$)   | `libfst.{a,so}`                                                           | [`CompactFst`](https://www.openfst.org/doxygen/fst/html/classfst_1_1CompactFst.html)
compactN_weighted_string, N=8,16,64     | `CompactFst<A, WeightedStringCompactor<A>, uintN>`     | Compact, immutable, weighted, string FST (# arcs < $2^N$)      | `fst/libfstcompact.{a,so}`, `fst/compactN_weighted-fst.{a,so}`            | [`CompactFst`](https://www.openfst.org/doxygen/fst/html/classfst_1_1CompactFst.html)
compact_acceptor                        | `CompactFst<A, AcceptorCompactor<A>>`                  | Compact, immutable, weighted FSA (# arcs < $2^{32}$)           | `libfst.{a,so}`                                                           | [`CompactFst`](https://www.openfst.org/doxygen/fst/html/classfst_1_1CompactFst.html)
compactN_acceptor, N=8,16,64            | `CompactFst<A, AcceptorCompactor<A>, uintN>`           | Compact, immutable, weighted FSA (# arcs < $2^N$)              | `fst/libfstcompact.{a,so}`, `fst/compactN_acceptor-fst.{a,so}`            | [`CompactFst`](https://www.openfst.org/doxygen/fst/html/classfst_1_1CompactFst.html)
compact_unweighted                      | `CompactFst<A, UnweightedCompactor<A>>`                | Compact, immutable, unweighted FST (# arcs < $2^{32}$)         | `libfst.{a,so}`                                                           | [`CompactFst`](https://www.openfst.org/doxygen/fst/html/classfst_1_1CompactFst.html)
compactN_unweighted N=8,16,64           | `CompactFst<A, UnweightedCompactor<A>, uintN>`         | Compact, immutable, unweighted FST (# arcs < $2^N$)            | `fst/libfstcompact.{a,so}`, `fst/compactN_unweighted-fst.{a,so}`          | [`CompactFst`](https://www.openfst.org/doxygen/fst/html/classfst_1_1CompactFst.html)
compact_unweighted_acceptor             | `CompactFst<A, UnweightedAcceptorCompactor<A>>`        | Compact, immutable, unweighted FSA (# arcs < $2^{32}$)         | `libfst.{a,so}`                                                           | [`CompactFst`](https://www.openfst.org/doxygen/fst/html/classfst_1_1CompactFst.html)
compactN_unweighted_acceptor, N=8,16,64 | `CompactFst<A, UnweightedAcceptorCompactor<A>, uintN>` | Compact, immutable, unweighted FSA (# arcs < $2^N$)            | `fst/libfstcompact.{a,so}`, `fst/compactN_unweighted_acceptor-fst.{a,so}` | [`CompactFst`](https://www.openfst.org/doxygen/fst/html/classfst_1_1CompactFst.html)
ilabel_lookahead                        | `{Std,Log}ILabelLookAheadFst`                          | Immutable FST with input label lookahead matcher                 | `fst/libfstlookahead.{a,so}`, `fst/ilabel_lookahead-fst.{a,so}`           |
olabel_lookahead                        | `{Std,Log}OLabelLookAheadFst`                          | Immutable FST with output label lookahead matcher                | `fst/libfstllookahead.{a,so}`, `fst/olabel_lookahead-fst.{a,so}`          |
arc_lookahead                           | `{Std,Log}ArcLookAheadFst`                             | Immutable FST with arc lookahead matcher                         | `fst/libfstllookahead.{a,so}`, `fst/arc_lookahead-fst.{a,so}`             |
ngram                                   | `NGramFst<A>`                                          | Immutable FST for n-gram language models                         | `fst/libfstngram.{a,so}`                                                  | [`NGramFst`](https://www.openfst.org/doxygen/fst/html/classfst_1_1NGramFst.html)

These FST types are registered for `[`StdArc`](advanced_usage.md#arcs)` and
`[`LogArc`](advanced_usage.md#arcs)` in the indicated libraries. The user must
[register](advanced_usage.md#input-output) other types themselves for general
FST I/O.

Note the libraries other than `libfst.{a,so}` are [extensions](extensions.md)
that must be built and linked separately (to avoid code bloat). For each of
these, there is a version that contains all variants of that extension (e.g.,
`lib/libfstconst.{a,so}`) that should be specified at compile time .
Alternatively, there are per variant libraries (e.g. `lib/constN-fst.so`) that
will be dynamically loaded into any binary compiled with OpenFst when the
`LD_LIBRARY_PATH` (or equivalent) includes e.g. `/usr/local/lib/fst`.

Note `Std{I,O}LabelLookAheadFst`, despite its name, uses the `LogWeight::Plus()`
during weight-pushing in composition (only). This choice was made for reasons of
efficiency and convenience; it can circumvented by changing the *accumulator*
[`FastLogAccumulator`](https://www.openfst.org/doxygen/fst/html/classfst_1_1FastLogAccumulator.html)
used.

Non-abstract FST types without file representations include the on-the-fly Fst
[operations](quick_tour.md#available-fst-operations) and the following:

Name         | Description                                                               |     |
------------ | ------------------------------------------------------------------------- | ---
`EditFst<A>` | Wraps an `ExpandedFst` as a `MutableFst`, sharing non-mutated components. | [`EditFst`](https://www.openfst.org/doxygen/fst/html/classfst_1_1EditFst.html)

<a id="fst-types"></a> You can convert to a non-abstract FST Type `F<A>` by
calling its `F<A>(const Fst<A> &)` constructor from C++ or the `fstconvert
-fst_type=Fname` shell-level command (when there is a file representation).

## Look-Ahead Matchers

*Lookahead matchers* are [matchers](advanced_usage.md#matchers) that implement
additional functionality to allow looking-ahead along paths. When used in
combination with a [lookahead filter](advanced_usage.md#composition-filters) in
composition, this can result in considerable efficiency improvements. See Cyril
Allauzen, Michael Riley and Johan Schalkwyk,
["Filters for Efficient Composition of Weighted Finite-State Transducers"](https://www.openfst.org/twiki/pub/FST/FstAdvancedUsage/ciaa10.pdf),
*Proceedings of the Fifteenth International Conference on Implementation and
Application of Automata*, (CIAA 2010), Winnipeg, MB.

The matcher interface is augmented with the following methods:

```cpp
template <class F>
class SomeLookAheadMatcher {
 public:
  typedef F FST;
  typedef F::Arc Arc;
  typedef typename Arc::StateId StateId;
  typedef typename Arc::Label Label;
  typedef typename Arc::Weight Weight;

 // Required constructors.
 LookAheadMatcher(const F &fst, MatchType match_type);
 LookAheadMatcher(const LookAheadMatcher &matcher);

 Below are methods for looking ahead for a match to a label and
 more generally, to a rational set. Each returns false if there is
 definitely not a match and returns true if there possibly is a match

 // LABEL LOOKAHEAD: Can 'label' be read from the current matcher state
 // after possibly following epsilon transitions?
 bool LookAheadLabel(Label label) const;

 // RATIONAL LOOKAHEAD: The next methods allow looking ahead for an
 // arbitrary rational set of strings, specified by an FST and a state
 // from which to begin the matching. If the lookahead FST is a
 // transducer, this looks on the side different from the matcher
 // 'match_type' (cf. composition).

 // Are there paths P from 's' in the lookahead FST that can be read from
 // the cur. matcher state?
 bool LookAheadFst(const Fst<Arc>& fst, StateId s);

 // Gives an estimate of the combined weight of the paths P in the
 // lookahead and matcher FSTs for the last call to LookAheadFst.
 // A trivial implementation returns Weight::One(). Non-trivial
 // implementations are useful for weight-pushing in composition.
 Weight LookAheadWeight() const;

 // Is there is a single non-epsilon arc found in the lookahead FST
 // that begins P (after possibly following any epsilons) in the last
 // call LookAheadFst? If so, return true and copy it to '*arc', o.w.
 // return false. A trivial implementation returns false. Non-trivial
 // implementations are useful for label-pushing in composition.
 bool LookAheadPrefix(Arc *arc);

 // Optionally pre-specifies the lookahead FST that will be passed
 // to LookAheadFst() for possible precomputation. If copy is true,
 // then 'fst' is a copy of the FST used in the previous call to
 // this method (useful to avoid unnecessary updates).
 void InitLookAheadFst(const Fst<Arc>& fst, bool copy = false);
};
```

The following lookahead matchers are defined in the OpenFst library:

Name                     | Description                                            |     |
------------------------ | ------------------------------------------------------ | ---
`ILabelLookAheadMatcher` | Look-ahead to first non-epsilon input label on a path  | [`LabelLookAheadMatcher`](https://www.openfst.org/doxygen/fst/html/classfst_1_1LabelLookAheadMatcher.html)
`OLabelLookAheadMatcher` | Look-ahead to first non-epsilon output label on a path | [`LabelLookAheadMatcher`](https://www.openfst.org/doxygen/fst/html/classfst_1_1LabelLookAheadMatcher.html)
`ArcLookAheadMatcher`    | Look-ahead to first transition on a path               | [`ArcLookAheadMatcher`](https://www.openfst.org/doxygen/fst/html/classfst_1_1ArcLookAheadMatcher.html)

There are [FST types](advanced_usage.md#fst-types) that are provided with these
matchers. When these are used in composition, no special options need to be
passed; the appropriate matcher and filter are selected automatically.

The ilabel (olabel) lookahead matcher has some special properties. It currently
requires that there are no input (output) epsilon cycles. Further, it may
relabel the input (output) alphabet in order to efficiently look-ahead. The
class `LabelLookAheadRelabeler` (in `<fst/lookahead-matcher.h>`) can be used to
obtain the mapping between the old and new alphabet
(`LabelLookAheadRelabeler::RelabelPairs`) and to relabel and sort other FSTs
with the new labeling to make them suitable for composition
(`LabelLookAheadRelabeler::Relabel`). Alternatively, the flag
`--save_relabel_ipairs` (`--save_relabel_opairs`) can be used to send the
relabeling information to a file when the lookahead matcher is constructed
(useful when `[fstconvert](advanced_usage.md#fst-types)` is used to create a
lookahead FST from the command line).

## Matchers

*Matchers* can find and iterate through requested labels at FST states; their
principal use is in composition matching. In the simplest form, these are just a
search or hash keyed on labels. More generally, they may implement matching
special symbols that represent sets of labels such as ρ (rest), σ (all) or φ
(fail), which can be used for more compact automata representations and faster
matching.

The Matcher interface is:

```txt
// Specifies matcher action.
enum MatchType {
   MATCH_INPUT, // Match input label.
   MATCH_OUTPUT, // Match output label.
   MATCH_NONE, // Match nothing.
   MATCH_UNKNOWN, // Match type unknown.
};
```

```cpp
template <class F>
class SomeMatcher {
  public:
   typedef F FST;
   typedef F::Arc Arc;
   typedef typename Arc::StateId StateId;
   typedef typename Arc::Label Label;
   typedef typename Arc::Weight Weight;

   // Required constructors.
   SomeMatcher(const F &fst, MatchType type);
   SomeMatcher(const SomeMatcher &matcher);

   // Returns the match type that can be provided (depending on
   // compatibility of the input FST). It is either
   // the requested match type, MATCH_NONE, or MATCH_UNKNOWN.
   // If 'test' is false, a constant time test is performed, but
   // MATCH_UNKNOWN may be returned. If 'test' is true,
   // a definite answer is returned, but may involve more costly
   // computation (e.g., visiting the Fst).
   MatchType Type(bool test) const;

   // Specifies the current state.
   void SetState(StateId s);

   // This finds matches to a label at the current state.
   // Returns true if a match found. kNoLabel matches any
   // 'non-consuming' transitions, e.g., epsilon transitions,
   // which do not require a matching symbol.
   bool Find(Label label);

   // These iterate through any matches found:
   // No more matches.
   bool Done() const;
   // Current arc (when !Done)
   const A& Value() const;
   // Advance to next arc (when !Done)
   void Next();

    // Indicates preference for being the side used for matching
    // in composition/intersection.
    ssize_t Priority(StateId s);

   // Return matcher FST.
   const F& GetFst() const;

   // This specifies the known Fst properties as viewed from this
   // matcher. It takes as argument the input Fst's known properties.
   uint64 Properties(uint64 props) const;
};
```

The following matchers are defined in the OpenFst library (see also the
[lookahead matcher](advanced_usage.md#look-ahead-matchers) topic).

Name                 | Description                                                              |     |
-------------------- | ------------------------------------------------------------------------ | ---
`SortedMatcher`      | Binary search on sorted input                                            | [`SortedMatcher`](https://www.openfst.org/doxygen/fst/html/classfst_1_1SortedMatcher.html)
`RhoMatcher<M>`      | ρ-symbol handling; templated on underlying matcher                       | [`RhoMatcher`](https://www.openfst.org/doxygen/fst/html/classfst_1_1RhoMatcher.html)
`SigmaMatcher<M>`    | σ-symbol handling; templated on underlying matcher                       | [`SigmaMatcher`](https://www.openfst.org/doxygen/fst/html/classfst_1_1SigmaMatcher.html)
`PhiMatcher<M>`      | φ-symbol handling; templated on underlying matcher                       | [`PhiMatcher`](https://www.openfst.org/doxygen/fst/html/classfst_1_1PhiMatcher.html)
`MultiEpsMatcher<M>` | Treats specified non-0 labels as non-consuming labels (in addition to 0) | [`MultiEpsMatcher`](https://www.openfst.org/doxygen/fst/html/classfst_1_1MultiEpsMatcher.html)
`ExplicitMatcher<M>` | Suppresses any implicit matches of non-consuming labels                  | [`ExplicitMatcher`](https://www.openfst.org/doxygen/fst/html/classfst_1_1ExplicitMatcher.html)

`SortedMatcher` expects the underlying FST be sorted on the appropriate side.
`Find(0)` matches any epsilons on the underlying FST explicitly (as if they were
any other symbol) but also returns an implicit self-loop (namely `Arc(kNoLabel,
0, Weight::One(), current_state)` if the `match_type` is `MATCH_INPUT` and
`Arc(0, kNoLabel, Weight::One(), current_state)` if the `match_type` is
`MATCH_OUTPUT`); in other words, an epsilon matches at every state without
moving forward on the matched FST, a natural interpretation. This behavior
implements epsilon-transition handling in composition, or, more generally, a
'non-consuming' match as with the `MultiEpsMatcher` (with `kNoLabel` informing
composition of such a match). A
[composition filter](advanced_usage.md#composition-filters) determines which of
these epsilon transitions are ultimately accepted. Any matcher used in
composition and related algorithms must implement these implicit matches for
correct epsilon handling. In some other uses, the implicit matches may not be
needed. In that case, an `ExplicitMatcher` can be used to conveniently suppress
them (or the user can recognize the `kNoLabel` loop and skip them).

The special symbols referenced above behave as described in this table:

&nbsp;       | Consumes no symbol | Consumes symbol
------------ | ------------------ | ---------------
Matches all  | ε                  | σ
Matches rest | φ                  | ρ

The ε symbol is assigned label `0` by [convention](conventions.md). The numeric
label of the other special symbols is determined by a constructor argument to
their respective matchers.

The ρ, σ and φ matchers augment the functionality of their underlying template
argument matcher. In this way, matchers can be cascaded (with special symbol
precedence determined by the order).

A design choice for these matchers is whether to *remove* the special symbol in
the result (used for the ρ, σ, and φ matchers) or *return* it (used for
epsilon-handling). The first case is equivalent to (but more efficient than)
applying special-symbol removal prior to composition (c.f.,
[epsilon removal](rm_epsilon.md)). This case requires that only one of the FSTs
in composition contain such symbols for any paired states. The second case
requires well-defined semantics and that composition proper identify and handle
any non-consuming symbols on each FST. (The result of `Find(kNoLabel)`
identifies on one FST, while the matcher's returning a `kNolabel` loop handles
the other, both described above.)

The template `Matcher<F>` selects the pre-designated matcher for `Fst` type `F`;
it is typically `SortedMatcher`. [Composition](compose.md) uses this matcher by
default. It can be [changed](advanced_usage.md#operation-options) by using the
version of `ComposeFst` that accepts `ComposeFstOptions`. Note two matchers
(usually of the same C++ type but different `MatchType`) are used in
composition -- one for each FST. Whether actual match queries are performed on
one or both FSTs depends on the matcher constructor arguments, the matcher
capabilities (queried by `Type()`) and composition itself.

An example access of an FST's matcher is [here](quick_tour.md#matcher). An
example use of a ρ matcher in composition is
[here](advanced_usage.md#operation-options); σ and φ matcher usage is similar.

## Mutable FSTs

A `MutableFst`
[`MutableFst`](https://www.openfst.org/doxygen/fst/html/classfst_1_1MutableFst.html)
is an [`ExpandedFst`](advanced_usage.md#expanded-fsts) that has additional
methods that specifiy how to set the start state, final weights,
[properties](advanced_usage.md#properties) and the input and output symbols, how
to add and delete states and arcs, as well as methods to copy and read the
mutable FST. In particular, a `MutableFst` class has the interface:

```cpp
template <class A>
class MutableFst : public ExpandedFst<class A> {
 public:
  typedef A Arc;
  typedef typename A::StateId StateId;
  typedef typename A::Weight Weight;

  // Set the initial state
  virtual void SetStart(StateId) = 0;
  // Set the initial state
  virtual void SetFinal(StateId, Weight) = 0;
  // Set property bits wrt mask
  virtual void SetProperties(uint64 props, uint64 mask) = 0;
  // Add a state, return its ID
  virtual StateId AddState() = 0;
  // Add an arc to state
  virtual void AddArc(StateId, const A &arc) = 0;
  // Delete some states
  virtual void DeleteStates(const vector<StateId>&) = 0;
  // Delete all states
  virtual void DeleteStates() = 0;
  // Delete some arcs at state
  virtual void DeleteArcs(StateId, size_t n) = 0;
  // Delete all arcs at state
  virtual void DeleteArcs(StateId) = 0;
  // Get a copy of this MutableFst
  virtual MutableFst<A> *Copy() const = 0;
  // Read an MutableFst from an input stream; returns NULL on error
  static MutableFst<A> *Read(istream &strm, const FstReadOptions &opts);
  // Read an MutableFst from a file; return NULL on error
  // Empty filename reads from standard input
  static MutableFst<A> *Read(const string &filename);
  // Set input label symbol table; NULL signifies not unspecified
  virtual void SetInputSymbols(const SymbolTable* isyms) = 0;
  // Set output label symbol table; NULL signifies not unspecified
  virtual void SetOutputSymbols(const SymbolTable* osyms) = 0;
};
```

`MutableFst` is an abstract class (note the pure virtual methods). An example is
`VectorFst`
[`VectorFst`](https://www.openfst.org/doxygen/fst/html/classfst_1_1VectorFst.html).

The companion [mutable arc iterator](advanced_usage.md#arc-iterators) class
provides access to and modification of the transitions of the FST

## Natural Orders

The *natural order* $\le$ associated with a [semiring](glossary.md#semiring)
is defined as $a \le b$ iff $a \oplus b = a$. In the OpenFst library, we
define the strict version of this order as:

```cpp
template <class W>
NaturalLess() {
  bool operator()(const W &w1, const W &w2) const {
    return (Plus(w1, w2) == w1) && w1 != w2;
  }
};
```

An order is *left monotonic* w.r.t a semring iff $a \le b \Rightarrow \forall
c, c \oplus a \le c \oplus b$ and $c \otimes a \le c \otimes b$; *right
monotonic* is defined similarly. An order is negative iff $1 \le 0$.

The natural order is a left (right) monotonic and negative partial order iff the
semiring is [idempotent](weight_requirements.md) and left (right)
[distributive](weight_requirements.md). It is a total order iff the semiring has
the [path property](weight_requirements.md). See Mohri, "Semiring Framework and
Algorithms for Shortest-Distance Problems", *Journal of Automata, Languages and
Combinatorics* 7(3):321-350, 2002.

This is the default total order (under the requirements above) that we use for
the shortest path and pruning algorithms. This order is the *natural* one to use
given that it generally needs to be total, monotonic and. negative: *total* so
that all weights can be compared, *monotonic* so there is a practical algorithm,
and *negative* so that the "free" weight 1 is preferred to the "disallowed"
weight 0.

## Operation Options

Many [FST operations](quick_tour.md#available-fst-operations) have versions that
accept options, especially option structures, that have not been documented in
this Wiki for brevity other than to mention some of the parameters that can be
changed. For example, most of the [delayed Fsts](quick_tour.md#delayed-fsts)
have constructors that accept options that control
[caching behavior](advanced_usage.md#caching).

Here is an example that selects minimal [caching](advanced_usage.md#caching) and
the [rho matcher](advanced_usage.md#matchers) (for fst2 ρ's) in composition::

```cpp
typedef RhoMatcher< SortedMatcher<StdFst> > RM;

ComposeFstOptions<StdArc, RM> opts;
opts.gc_limit = 0;
opts.matcher1 = new RM(fst1, MATCH_NONE, kNoLabel);
opts.matcher2 = new RM(fst2, MATCH_INPUT, SomeRhoLabel);

StdComposeFst cfst(fst1, fst2, opts);
```

Follow the links to the code under each operation's documentation for the
specific details.

## Properties

Each `Fst` has associated with it a set of stored properties that assert facts
about it. These are queried in an FST with the `Properties()` method and set in
a `MutableFst` with the `SetProperties()` method. OpenFst library operations use
these properties to optimize their performance. OpenFst library operations and
mutable FSTs attempt to preserve as much property information in their results
as possible without significant added computation.

Some properties are binary - they are either true or false. For each such
property, there is a single stored bit that is set if true and not set if false.
The binary `Fst` properties are:

Name        | Description
----------- | -----------
`kError`    | an [error](advanced_usage.md#error-handling) was detected while constructing/using the FST
`kExpanded` | Is an [`ExpandedFst`](advanced_usage.md#expanded-fsts)
`kMutable`  | Is a [`MutableFst`](advanced_usage.md#mutable-fsts)

Other properties are trinary - they are either true, false or unknown. For each
such property, there are two stored bits; one is set if true, the other is set
if false and neither is set if unknown.

Type          | Name                 | Description
------------- | -------------------- | -----------
Acceptor      | `kAcceptor`          | Input and output label are equal for each arc
&nbsp;        | `kNotAcceptor`       | Input and output label are not equal for some arc
Accessible    | `kAccessible`        | All states reachable from the initial state
&nbsp;        | `kNotAccessible`     | Not all states reachable from the initial state
&nbsp;        | `kCoAccessible`      | All states can reach a final state
&nbsp;        | `kNotCoAccessible`   | Not all states can reach a final state
Cyclic        | `kCyclic`            | Has cycles
&nbsp;        | `kAcyclic`           | Has no cycles
&nbsp;        | `kInitialCyclic`     | Has cycles containing the initial state
&nbsp;        | `KInitialAcyclic`    | Has no cycles containing the initial state
Deterministic | `kIDeterministic`    | Input labels are unique leaving each state
&nbsp;        | `kNonIDeterministic` | Input labels are not unique leaving some state
&nbsp;        | `kODeterministic`    | Output labels are unique leaving each state
&nbsp;        | `kNonODeterministic` | Output labels are not unique leaving some state
Epsilons      | `kEpsilons`          | Has input/output epsilons
&nbsp;        | `KNoEpsilons`        | Has no input/output epsilons
&nbsp;        | `kIEpsilons`         | Has input epsilons
&nbsp;        | `KNoIEpsilons`       | Has no input epsilons
&nbsp;        | `kOEpsilons`         | Has output epsilons
&nbsp;        | `KNoOEpsilons`       | Has no output epsilons
Sorted        | `kILabelSorted`      | Input labels sorted for each state
&nbsp;        | `kNotILabelSorted`   | Input labels not sorted for each state
&nbsp;        | `kOLabelSorted`      | Output labels sorted for each state
&nbsp;        | `kNotOLabelSorted`   | Output labels not sorted for each state
&nbsp;        | `kTopSorted`         | States topologically sorted
&nbsp;        | `kNotTopSorted`      | States not topologically sorted
Weighted      | `kWeighted`          | Non-trivial arc or final weights
&nbsp;        | `kNotWeighted`       | Only trivial arc and final weights

The call `fst.Properties(mask, false)` returns the stored property bits set in
the mask bits; some properties may be unknown. it is a constant-time operation.
The call `fst.Properties(mask, true)` returns the stored property bits set in
the mask bits after computing and updating any of those set in the mask that are
unknown. It is a linear-time ($O(V + E)$) operation if any of the requested
bits were unknown.

Note `fstinfo --test_properties=false` will show the stored properties bits,
while `fstinfo` or `fstinfo --test_properties=true` will compute unknown
properties.

## State Iterators

A state iterator
[`StateIterator`](https://www.openfst.org/doxygen/fst/html/classfst_1_1StateIterator.html)
is used to access the states of an FST. It has the form:

```cpp
template <class F>
class StateIterator {
  typedef typename F::Arc Arc;
  typedef typename Arc::StateId StateId;

 public:
  StateIterator(const &F fst);
  // End of iterator?
  bool Done() const;
  // Current state ID (when !Done)
  StateId Value() const;
  // Advance to next state (when !Done)
  void Next();
  // Return to initial position
  void Reset();
};
```

It is templated on the Fst class `F` to allow efficient specializations but
defaults to a generic version on the abstract base
[`Fst`](advanced_usage.md#base-fsts) class.

See [here](conventions.md) for conventions that state iterator use must respect.

An example use of a state iterator is shown
[here](quick_tour.md#state-iterators).

## State Mappers

*State mappers* are function objects used by the [StateMap](state_map.md)
operation to transform states. A state mapper has the form:

```cpp
// This determines how symbol tables are mapped.
enum MapSymbolsAction {
  // Symbols should be cleared in the result by the map.
  MAP_CLEAR_SYMBOLS,

  // Symbols should be copied from the input FST by the map.
  MAP_COPY_SYMBOLS,

  // Symbols should not be modified in the result by the map itself.
  // (They may set by the mapper).
  MAP_NOOP_SYMBOLS
};

class SomeStateMapper {
  public:
   // Assumes input arc type is A and result arc type is B
   typedef A FromArc;
   typedef B ToArc;

   // Typical constructor.
   SomeStateMapper(const Fst<A> &fst);
   // Required copy constructor that allows updating Fst argument.
   SomeStateMapper(const SomStateMapper &mapper, const Fst<A&ft; *fst = 0);

   // Specifies initial state of result
   B::StateId Start() const;
   // Specifies state's final weight in result
   B::Weight Final(B::StateId s) const;

   // These methods iterate through a state's arcs in result
   // Specifies state to iterator over
   void SetState(B::StateId s);
   // End of arcs?
   bool Done() const;
   // Current arc
   const B &Value() const;
   // Advance to next arc (when !Done)
   void Next();

   // Specifies input symbol table action the mapper requires (see above).
   MapSymbolsAction InputSymbolsAction() const;
   // Specifies output symbol table action the mapper requires (see above).
   MapSymbolsAction OutputSymbolsAction() const;
   // This specifies the known properties of an Fst mapped by this
   // mapper. It takes as argument the input Fst's known properties
   uint64 Properties(uint64 props) const;
};
```

The following state mappers are defined in the OpenFst library:

Name                  | Description                                               |     |
--------------------- | --------------------------------------------------------- | ---
`ArcSumMapper`        | Sums weights of identically labeled multi-arcs            | [`ArcSumMapper`](https://www.openfst.org/doxygen/fst/html/classfst_1_1ArcSumMapper.html)
`ArcUniqueMapper`     | Keeps one of identically labelled and weighted multi-arcs | [`ArcUniqueMapper`](https://www.openfst.org/doxygen/fst/html/classfst_1_1ArcUniqueMapper.html)
`IdentityStateMapper` | Maps to self                                              | [`IdentityStateMapper`](https://www.openfst.org/doxygen/fst/html/classfst_1_1IdentityStateMapper.html)

Another specialized state mapper is used to implement [ArcSort](arc_sort.md).

## State Queues

State queues are used by, among others, the [shortest path](shortest_path.md)
and [shortest distance](shortest_distance.md) algorithms and by the
[Visit](advanced_usage.md#visitors) operation. A `state queue` has the form:

```cpp
template <class StateId>
class SomeQueue {
 public:
   // Ctr: may need args (e.g., Fst, comparator) for some queues
   SomeQueue(...);
   // Returns the head of the queue
   StateId Head() const;
   // Inserts a state
   void Enqueue(StateId s);
   // Removes the head of the queue
   void Dequeue();
   // Updates ordering of state s when weight changes, if necessary
   void Update(StateId s);
   // Does the queue contain no elements?
   bool Empty() const;
   // Remove all states from queue
   void Clear();
};
```

Pre-defined state queues include:

Queue                       | Description                                                        |     |
--------------------------- | ------------------------------------------------------------------ | ---
`AutoQueue`                 | Automatically-selected from Fst properties                         | [`AutoQueue`](https://www.openfst.org/doxygen/fst/html/classfst_1_1AutoQueue.html)
`FifoQueue`                 | First-In, first-Out                                                | [`FifoQueue`](https://www.openfst.org/doxygen/fst/html/classfst_1_1FifoQueue.html)
`LifoQueue`                 | Last-In, first-Out                                                 | [`LifoQueue`](https://www.openfst.org/doxygen/fst/html/classfst_1_1LifoQueue.html)
`NaturalAStarQueue`         | A* (under natural order with provided estimate)                    | [`NaturalAStarQueue`](https://www.openfst.org/doxygen/fst/html/classfst_1_1NaturalAStarQueue.html)
`NaturalPruneQueue`         | Pruning meta-queue (within provided threshold under natural order) | [`NaturalPruneQueue`](https://www.openfst.org/doxygen/fst/html/classfst_1_1NaturalPruneQueue.html)
`NaturalShortestFirstQueue` | Priority (least weight under natural order)                        | [`NaturalShortestFirstQueue`](https://www.openfst.org/doxygen/fst/html/classfst_1_1NaturalShortestFirstQueue.html)
`SccQueue`                  | Component graph top-ordered meta-queue                             | [`SccQueue`](https://www.openfst.org/doxygen/fst/html/classfst_1_1SccQueue.html)
`StateOrderQueue`           | State-ID ordered                                                   | [`StateOrderQueue`](https://www.openfst.org/doxygen/fst/html/classfst_1_1StateOrderQueue.html)
`TopOrderQueue`             | Topologically ordered                                              | [`TopOrderQueue`](https://www.openfst.org/doxygen/fst/html/classfst_1_1TopOrderQueue.html)

Some queues accept [arc filters](advanced_usage.md#arc-filters) to control which
transitions are explored.

## State Tables

*State tables* determine the bijective mapping between state tuples (e.g. in
[composition](compose.md) triples of two FST states and a
[composition filter state](advanced_usage.md#composition-filters)) and their
corresponding state IDs. They are classes, templated on state tuples, of the
form:

```cpp
template <class T>
class SomeStateTable {
   typedef typename T StateTuple;

   // Required constructors.
   SomeStateTable();
   // Lookup state ID by tuple. If it doesn't exist, then add it.
   StateId FindState(const StateTuple &);
   // Lookup state tuple by state ID.
   const StateTuple<StateId> &Tuple(StateId) const;
 };
```

A state tuple has the form:

```cpp
template <class S>
struct SomeStateTuple {
   typedef typename S StateId;

   // Required constructor.
   SomeStateTuple();
   // Data
   ...
};
```

A specific state tuple is a `ComposeStateTuple` that has data members `StateId
state_id1`, `StateId state_id2`, and `FilterState filter_state`.

The following state tables are defined in the OpenFst library:

Name                    | Description                             |     |
----------------------- | --------------------------------------- | ---
`HashStateTable`        | Hash map implementation                 | [`HashStateTable`](https://www.openfst.org/doxygen/fst/html/classfst_1_1HashStateTable.html)
`CompactHashStateTable` | Hash set implementation                 | [`CompactHashStateTable`](https://www.openfst.org/doxygen/fst/html/classfst_1_1CompactHashStateTable.html)
`VectorStateTable`      | Vector implementation                   | [`VectorStateTable`](https://www.openfst.org/doxygen/fst/html/classfst_1_1VectorStateTable.html)
`VectorHashStateTable`  | Vector and hash set implementation      | [`VectorHashStateTable`](https://www.openfst.org/doxygen/fst/html/classfst_1_1VectorHashStateTable.html)
`ErasableStateTable`    | Deque implementation - permits erasures | [`ErasableStateTable`](https://www.openfst.org/doxygen/fst/html/classfst_1_1ErasableStateTable.html)

Different state tables provide different time and space tradeoffs for
applications.

Composition state tables are defined using state tables with
`ComposeStateTuple`. They are the principal data structure used by composition
other than the result [cache](advanced_usage.md#caching).

The following composition state tables are defined in the OpenFst library:

Name                         | State Table             | Description                                                     |     |
---------------------------- | ----------------------- | --------------------------------------------------------------- | ---
`GenericComposeStateTable`   | `CompactHashStateTable` | General-purpose choice                                          | [`GenericComposeStateTable`](https://www.openfst.org/doxygen/fst/html/classfst_1_1GenericComposeStateTable.html)
`ProductComposeStateTable`   | `VectorStateTable`      | Efficient when the composition state space is densely populated | [`ProductComposeStateTable`](https://www.openfst.org/doxygen/fst/html/classfst_1_1ProductComposeStateTable.html)
`StringDetComposeStateTable` | `VectorStateTable`      | Efficient when FST1 is a string and FST2 is deterministic       | [`StringDetComposeStateTable`](https://www.openfst.org/doxygen/fst/html/classfst_1_1StringDetComposeStateTable.html)
`DetStringComposeStateTable` | `VectorStateTable`      | Efficient when FST1 is deterministic and FST2 is a string       | [`DetStringComposeStateTable`](https://www.openfst.org/doxygen/fst/html/classfst_1_1DetStringComposeStateTable.html)
`EraseableComposeStateTable` | `ErasableStateTable`    | Allows composition state tuple erasure                          | [`ErasableComposeStateTable`](https://www.openfst.org/doxygen/fst/html/classfst_1_1ErasableComposeStateTable.html)

`GenericComposeStateTable` is the default composition state table. It can be
[changed](advanced_usage.md#operation-options) by using the version of
`ComposeFst` that accepts `ComposeFstOptions`.

## Symbol Tables

*Symbol tables* store the bijective mapping between textual labels, used in
[reading and printing](quick_tour.md#symbol-tables) an FST textual file, and
their integer assignment, used in the FST's internal representation. Symbol
tables are usually read in with [fstcompile](quick_tour.md#fstcompile), can be
stored by the FST, and used to print out the FST with
[fstprint](quick_tour.md#symbol-tables),. Here are a examples of symbol table
manipulation:

```bash
// Various ways to reading symbol tables
StdFst *fst = StdFst::Read("some.fst");
SymbolTable *isyms = fst->InputSymbolTable();
SymbolTable *osyms = fst->OutputSymbolTable();

SymbolTable *syms = SymbolTable::ReadText("some.syms");

// Adding and accessing symbols and keys
syms->AddSymbol("kumquat", 7);
int64 key = syms->Find("kumquat");
string symbol = syms->Find(7);

// Various ways of writing symbol tables
fst->SetInputSymbols(isyms);
fst->SetOutputSymbols(osyms);
fst->Write("some.fst"):

syms->WriteText("some.syms");
```

## User-defined Arcs and Weights

A user may define his own weight type so long as it meets the necessary
[requirements](weight_requirements.md).

A user may define his own arc type so long as has the right
[form](advanced_usage.md#arcs). Some Fst I/O with new arc types requires
[registration](advanced_usage.md#input-output).

## User-defined FST Classes

## Visitors

The simplest way to traverse an FST is in state order using a
[state iterator](quick_tour.md#state-iterators).

A very general traversal method is to use:

```txt
Visit(fst, visitor, queue);
```

where the `visitor` object specfies the actions taken in the traversal while the
[state queue](advanced_usage.md#state-queues) object specifies the traversal
order. A `visitor` has the form:

```cpp
// Visitor Interface - class determines actions taken during a visit.
// If any of the boolean member functions return false, the visit is
// aborted by first calling FinishState() on all unfinished (grey)
// states and then calling FinishVisit().

template <class Arc>
class SomeVisitor {
public:
   typedef typename Arc::StateId StateId;

   SomeVisitor(T *return_data);
   // Invoked before visit
   void InitVisit(const Fst<Arc> &fst);
   // Invoked when state discovered (2nd arg is visitation root)
   bool InitState(StateId s, StateId root);
   // Invoked when arc to white/undiscovered state examined
   bool WhiteArc(StateId s, const Arc &a);
   // Invoked when arc to grey/unfinished state examined
   bool GreyArc(StateId s, const Arc &a);
   // Invoked when arc to black/finished state examined
   bool BlackArc(StateId s, const Arc &a);
   // Invoked when state finished
   void FinishState(StateId s);
   // Invoked after visit
   void FinishVisit();
};
```

While a depth-first search can be implemented using `Visit()` with the
`LifoQueue()`, it is often better to use the more specialized `DFSVisit()` in
[`<fst/dfs-visit.h>`](https://www.openfst.org/doxygen/fst/html/dfs-visit_8h_source.html)
since it is somewhat more space-efficient and the specialized visitor interface
described there has additional funcitionality for a DFS.

Pre-defined FST visitors include:

Visitor           | Type     | Description                                                                                                                   |     |
----------------- | -------- | ----------------------------------------------------------------------------------------------------------------------------- | ---
`CopyVisitor`     | Visit    | Copies in a queue-specified order                                                                                             | [`CopyVisitor`](https://www.openfst.org/doxygen/fst/html/classfst_1_1CopyVisitor.html)
`SccVisitor`      | DfsVisit | Finds strongly-connected components, [accessibility](glossary.md#accessible) and [coaccessibility](glossary.md#co-accessible) | [`SccVisitor`](https://www.openfst.org/doxygen/fst/html/classfst_1_1SccVisitor.html)
`TopOrderVisitor` | DfsVisit | Finds topological order                                                                                                       | [`TopOrderVisitor`](https://www.openfst.org/doxygen/fst/html/classfst_1_1TopOrderVisitor.html)

The visit operations optionally accept
[arc filters](advanced_usage.md#arc-filters) to control which transitions are
explored.

## Weights

A `Weight` is a type that is used to represent the cost of taking transitions in
an FST.

The following basic weight templates are defined in the OpenFst library:

Semiring      | Name                            | Set                                   | $\oplus$ (Plus)                          | $\otimes$ (Times)                           | $0$ (Zero)          | $1$ (One)           | Notes                                                                                            |     |
------------- | ------------------------------- | ------------------------------------- | ------------------------------------------ | --------------------------------------------- | --------------------- | --------------------- | ------------------------------------------------------------------------------------------------ | ---
Expectation   | `ExpectationWeight<W1, W2>`     | $W_1 \times W_2$                    | $\oplus_{W_1} \times \oplus_{W_2}$       | [$\otimes$expectation](expectation_plus.md) | $(0_{W_1},0_{W_2})$ | $(1_{W_1},0_{W_2})$ |                                                                                                  | [`ProductWeight`](https://www.openfst.org/doxygen/fst/html/classfst_1_1ProductWeight.html)
Lexicographic | `LexicographicWeight<W1, W2>`   | $W_1 \times W_2$                    | $\min$                                   | $\otimes_{W_1} \times \otimes_{W_2}$        | $(0_{W_1},0_{W_2})$ | $(1_{W_1},1_{W_2})$ | min: lexicographic order w.r.t. `W1` and `W2` [natural orders](advanced_usage.md#natural-orders) | [`LexicographicWeight`](https://www.openfst.org/doxygen/fst/html/classfst_1_1LexicographicWeight.html)
Log           | `LogWeightTpl<T>`               | $[-\infty, \infty]$                 | $-\log(e^{-x} + e^{-y})$                 | $+$                                         | $\infty$            | $0$                 | `T`: floating point                                                                              | [`LogWeightTpl`](https://www.openfst.org/doxygen/fst/html/classfst_1_1LogWeightTpl.html)
MinMax        | `MinMaxWeightTpl<T>`            | $[-\infty, \infty]$                 | $\min$                                   | $\max$                                      | $\infty$            | $-\infty$           | `T`: floating point                                                                              | [`MinMaxWeightTpl`](https://www.openfst.org/doxygen/fst/html/classfst_1_1MinMaxWeightTpl.html)
Power         | `PowerWeight<W, n>`             | $W^n$                               | $\oplus_{W^n}$                           | $\otimes_{W^n}$                             | $0_{W^n}$           | $1_{W^n}$           |                                                                                                  | [`PowerWeight`](https://www.openfst.org/doxygen/fst/html/classfst_1_1PowerWeight.html)
Product       | `ProductWeight<W1, W2>`         | $W_1 \times W_2$                    | $\oplus_{W_1} \times \oplus_{W_2}$       | $\otimes_{W_1} \times \otimes_{W_2}$        | $(0_{W_1},0_{W_2})$ | $(1_{W_1},1_{W_2})$ |                                                                                                  | [`ProductWeight`](https://www.openfst.org/doxygen/fst/html/classfst_1_1ProductWeight.html)
Real          | `RealWeightTpl<T>`              | $[0, \infty)$                       | $+$                                      | $\times$                                    | $0$                 | $1$                 | `T`: floating point                                                                              | [`RealWeightTpl`](https://www.openfst.org/doxygen/fst/html/classfst_1_1RealWeightTpl.html)
SignedLog     | `SignedLogWeightTpl<T>`         | $\{-1,1\} \times [-\infty, \infty]$ | [$\oplus$signed_log](signed_log_plus.md) | $(\times,+)$                                | $(1, \infty)$       | $(1, 0)$            | `T`: floating point                                                                              | [`SignedLogWeightTpl`](https://www.openfst.org/doxygen/fst/html/classfst_1_1SignedLogWeightTpl.html)
SparsePower   | `SparsePowerWeight<W>`          | $W^n$                               | $\oplus_{W^n}$                           | $\otimes_{W^n}$                             | $0_{W^n}$           | $1_{W^n}$           | `n`: arbitrary                                                                                   | [`SparsePowerWeight`](https://www.openfst.org/doxygen/fst/html/classfst_1_1SparsePowerWeight.html)
String        | `StringWeight<L, STRING_LEFT>`  | $L^* \cup \{\infty\}$               | longest com. prefix                        | $\cdot$                                     | $\infty$            | $\epsilon$          | `L`: signed integral                                                                             | [`StringWeight`](https://www.openfst.org/doxygen/fst/html/classfst_1_1StringWeight.html)
&nbsp;        | `StringWeight<L, STRING_RIGHT>` | $L^* \cup \{\infty\}$               | longest com. suffix                        | $\cdot$                                     | $\infty$            | $\epsilon$          | `L`: signed integral                                                                             | [`StringWeight`](https://www.openfst.org/doxygen/fst/html/classfst_1_1StringWeight.html)
Tropical      | `TropicalWeightTpl<T>`          | $[-\infty, \infty]$                 | $\min$                                   | $+$                                         | $\infty$            | $0$                 | `T`: floating point                                                                              | [`TropicalWeightTpl`](https://www.openfst.org/doxygen/fst/html/classfst_1_1TropicalWeightTpl.html)

The following weight types have been defined in the OpenFst library in terms of
the above:

Name                    | Type
----------------------- | --------------------------------------
`GallicWeight<L, W, S>` | `ProductWeight<StringWeight<L, S>, W>`
`LogWeight`             | `LogWeightTpl<float>`
`Log64Weight`           | `LogWeightTpl<double>`
`MinMaxWeight`          | `MinMaxWeightTpl<float>`
`RealWeight`            | `RealWeightTpl<float>`
`Real64Weight`          | `RealWeightTpl<double>`
`SignedLogWeight`       | `SignedLogWeightTpl<float>`
`SignedLog64Weight`     | `SignedLogWeightTpl<double>`
`TropicalWeight`        | `TropicalWeightTpl<float>`

Composite weights, such as `ProductWeight` and `LexicographicWeight`, can use
[command line flags](advanced_usage.md#command-line-flags) to control their
textual formatting. `FLAGS_fst_weight_separator` is printed between the weights
(default: `","`). `FLAGS_fst_weight_parentheses` (default: `""`) brackets the
weight; if you create nested composite weights, they need to be printed with
non-empty brackets (e.g. `"()"`) to ensure correct parsing if read back in.
These affect only textual (not binary) I/O.

Additional weight information:

*   [Corresponding arc types](advanced_usage.md#arcs)
*   [Elementary weight information](quick_tour.md#weights)
*   [User-defined weights](advanced_usage.md#user-defined-arcs-and-weights)
*   [Weight type requirements](weight_requirements.md)
