# Efficiency

By reading the [Quick Tour](quick_tour.md) and [Conventions](conventions.md) and
working through the [examples](examples.md), users typically can create
*correct* implementations of applications. What requires more experience is
creating *efficient* implementations.

## Efficient Representations

*   **FST Type:** The specific [FstType](advanced_usage.md#fst-types) can affect
    both time and space in an application.
    *   *Mutable:* `VectorFst` is recommended for general mutations, while
        `EditFst` can be used for small changes to an existing FST.
    *   *Immutable:* `ConstFst` uses similar space to `VectorFst` but is much
        faster to read and write and possibly to access since it has less
        fragmentation. The `CompactFst` types permit smaller memory and file
        representations of acceptors, of unweighted acceptors and transducers
        and weighted and unweighted strings. These types admit optional
        memory-mapping (coming soon).
    *   *Specialized:* For some applications, specialized FST types can be
        created that are tailored to the structure of the automaton. One
        relatively simple way to do this is to extend the `CompactFst` types by
        defining a new compactor. A more general, but complex, way is to define
        an entirely new FST type. For example, `NGramFst` can be used to very
        compactly represent n-gram language models. This type admits
        memory-mapping.
*   **Arc Type:** A particularly simple way to improve the representation size
    is to use an [`Arc`](advanced_usage.md#arcs) that has `StateId`, `Label`,
    and `Weight` types appropriately sized to a problem.
*   **Interface Type:** When creating [state](advanced_usage.md#state-iterators)
    or [arc iterators](advanced_usage.md#arc-iterators), the specific
    [interface type](quick_tour.md#calling-fst-operations-from-c) can greatly
    affect efficiency. Using the most specific FST type as the template argument
    of the iterators will access specializations that avoid virtual function and
    other overhead of more generic calls. E.g., prefer
    `ArcIterator<VectorFst<Arc>>(fst, s)` to `ArcIterator<Fst<Arc>>(fst, s)`,
    `ArcIterator<ExpandedFst<Arc>>(fst, s)`, or
    `ArcIterator<MutableFst<Arc>>(fst, s)` if performance is critical.

## Efficient Algorithms

#### General Issues

*   **Association Order:** Although an operation may be associative, efficiency
    may greatly vary depending on the order of application. For example, prefer
    `(A ∪ B) ∪ C` to `A ∪ (B ∪ C)` if `A` and `B` are small and `C` is large.
    Similar considerations apply to concatenation and intersection/composition.
*   **Optimization:** What optimizations should be applied is very
    problem-dependent. Epsilon removal, determinization and minimization of an
    unweighted automaton will give in the smallest, irredundant result. However
    this could be exponentially larger than the (non-deterministic) input, so
    applying some or none of these operations may be preferred. In the weighted
    and/or transducer case, the input may not even be determinizable although it
    can be determinized and minimized as an appropriately
    [encoded](encode_decode.md) automaton.
*   **Eager vs Delayed Computation:** If only a small portion of the result is
    visited (e.g., of a composition), then prefer
    [delayed](quick_tour.md#calling-fst-operations-from-c) computation (e.g.,
    invoke `ComposeFst`). On the other hand, if much of the result is visited,
    then prefer the eager version (e.g., invoke `Compose`) due to lower
    overhead.
*   **Caching:** Various operations such as the
    [delayed](quick_tour.md#calling-fst-operations-from-c) FST operations and
    the [`CompactFst`](advanced_usage.md#fst-types) classes may use
    [caching](advanced_usage.md#caching) to avoid recomputation on repeated
    access. The size of the cache and whether (LRU) garbage collection is
    performed is controlled by options. In some cases, caching can also be
    disabled per state by [arc iterator](quick_tour.md#arc-iterators) flags.
*   **Properties:** An FST maintains stored
    [property bits](advanced_usage.md#properties) such as whether it is an
    acceptor, acyclic, or input-label sorted. Many algorithms check for various
    of these properties, which will be computed in linear time if they are not
    already stored. This linear pass can thus be avoided if the needed bits are
    already set (e.g., by the user).

#### Algorithm-Specific Issues

*   **Composition:** The choice of the [matcher](advanced_usage.md#matchers),
    [filter](advanced_usage.md#composition-filters), and
    [state table](advanced_usage.md#state-tables), all which may be changed by
    options, can have significant affect on the efficiency of
    [composition](compose.md):
    *   *Matchers:*
        *   `SortedMatcher`: By default, the composition algorithm matches
            labels using binary search. This requires one or both FSTs to be
            appropriately label-sorted and this, in turn, requires property
            checking. If only one FST is sorted or, in fact, only one FST has
            the sorted property bit set, then that FST will be used for the
            binary search. In this case, it is important that this FST has the
            higher out-degree for efficient matching.
        *   `SigmaMatcher`, `RhoMatcher`: These matchers can be used to reduce
            time and space for matching *all* or *otherwise unmatched* labels at
            a state.
        *   `ArcLookAheadMatcher`, `LabelLookAheadMatcher`: These matchers can
            be used to reduce time by checking for a match with the following
            label or the following non-epsilon label on a path.
    *   *Filters:*
        *   `SequenceComposeFilter`, `AltSequenceComposeFilter`,
            `MatchComposeFilter`: These filters control how epsilons are matched
            and can give very different sized (if equivalent) composition
            results.
        *   `LookAheadFilter`: needed with lookahead matchers
    *   *State Tables:* By default, a hash mapping is used from state pair to
        composition state ID. Other data structures can give better performance
        in specialized cases. Several alternative state tables are provided by
        the library.
*   **Determinization:** The pruning thresholds that can be passed to
    [Determinize](determinize.md) can greatly help in the determinization of
    acyclic weighted automata. For example, suppose the input has already been
    pruned (e.g. with [Prune](prune.md)) to a weight threshold of θ. If
    `Determinize` is passed the same threshold, then it will be used *during*
    determinization to control it but with an effect the same as if `Prune` were
    called on the result. Using the same threshold will have an effect since
    determinization can change the structure of the result.
*   **Epsilon Removal:** By default, the [epsilon removal](rm_epsilon.md)
    algorithm reassigns/copies non-epsilon transitions that *follow* epsilon
    paths. An alternate choice is to reassign/copy non-epsilon transitions that
    *precede* epsilon paths, which can be requested with the `reverse=true`
    option. This latter choice is equivalent to performing the former on the
    [reverse](reverse.md) of the FST. The two methods can give very different
    sized results depending on the input.
*   **Shortest Path/Distance:** The shortest path/distance algorithms use a
    [queue discipline](advanced_usage.md#state-queues) to determine the order of
    expansion of states:
    *   `LifoQueue`: when the input is unweighted
    *   `StateOrderQueue`: when the input is topologically-ordered
    *   `TopOrderQueue`: when the input is acyclic, but not
        topologically-ordered
    *   `ShortestFirstQueue`: when the input is cyclic, but has no negative
        weights
    *   `FifoQueue`: otherwise
    *   ☞ Thus, the queue is selected based on the FST properties. This choice,
        with possible property testing, can be overridden by passing the queue
        as an option. With the `ShortestFirstQueue`, the shortest path algorithm
        can be passed the `first_path=true` option to stop when the first
        solution is discovered (correct if the weights are between 0 and 1).
        This is a case where delayed computation of the FST being searched may
        be preferred.
