# OpenFst Extensions

The following extensions to OpenFst are available. These are built only if the
configure flags are provided as below. Also provided are the include files and
the libraries to be used in client code.

## Compact FSTs

configure               | include               | lib
----------------------- | --------------------- | --------------------------
`--enable-compact-fsts` | `<fst/compact-fst.h>` | `fst/libfstcompact.{a,so}`

Compact FSTs use space-efficient representations of specialized FSTs such as
acceptors, strings or unweighted FSTS. This extension has libraries that
[register](advanced_usage.md#input-output) `CompactFst` for `uint8`, `uint16`,
and `uint64` representable total arcs. `CompactFst` for `uint32` is registered
in `libfst.{a,so}`. See [here](advanced_usage.md#fst-types) for more details,
including how to load dynamically.

## Const FSTs

configure             | include             | lib
--------------------- | ------------------- | ------------------------
`--enable-const-fsts` | `<fst/const-fst.h>` | `fst/libfstconst.{a,so}`

A `ConstFst` is a general-purpose immutable FST. This extension has libraries
that [register](advanced_usage.md#input-output) `ConstFst` for `uint8`,
`uint16`, and `uint64` representable total arcs. `ConstFst` for `uint32` is
registered in `libfst.{a,so}`. See [here](advanced_usage.md#fst-types) for more
details, including how to load dynamically.

## FST Archives (FARs)

configure      | include                         | lib
-------------- | ------------------------------- | ----------------------
`--enable-far` | `<fst/extensions/far/farlib.h>` | `fst/libfstfar.{a,so}`

A *finite-state archive (FAR)* is used to store an indexable collection of FSTs
in a single file. Utilities are provided to create FARs from FSTs, to iterate
over FARs, and to extract all or specific FSTs from FARs.

Operation         | Usage                                               | Description
----------------- | --------------------------------------------------- | -----------
farcreate         | farcreate a.fst b.fst ... out.far                   | creates a finite-state archive from input FSTs
farcompilestrings | farcompilestrings [-symbols=in.syms] in.txt out.far | compiles a set of strings as FSTs and stores them in a finite-state archive.
farextract        | farextract in.far                                   | extracts FSTs from a finite-state archive
farinfo           | farinfo in.far                                      | prints some basic information about the FSTs in an FST archive
FarReader         | FarReader<Arc>::Open("in.far");                     | returns class that iterates through an existing archive of FSTs
FarWriter         | FarWriter<Arc>::Create("out.far")                   | returns class that creates an archive of FSTs.

See the source code for additional information including options.

## FST Compression

configure           | include            | lib
------------------- | ------------------ | ---------------------------
`--enable-compress` | `<fst/compress.h>` | `fst/libfstcompress.{a,so}`

This extension enables [LZA](https://arxiv.org/abs/1502.07288) compression of
FSTs, a Lempel-Ziv-based compression scheme specifically for automata.

## Linear FSTs

configure              | include                                         | lib
---------------------- | ----------------------------------------------- | ---
`--enable-linear-fsts` | `<fst/linear-fst.h>`, `<fst/loglinear-apply.h>` | `fst/linear_tagger-fst.{a,so}`, `fst/linear_classifier-fst.{a,so}`

A linear FST is an immutable FST that stores a linear (e.g. CRF, structured
perceptron, maxent) model. Currently only input/output n-gram features are
supported. There are two variants of linear FSTs, namely linear tagger FSTs and
linear classifier FSTs.

A linear tagger FST can be compiled from the textual representation of the
vocabulary and the model (feature weights) using `fstlinear`.

The vocabulary file consists of lines where each is a single vocabulary record,
in the following format,

```txt
INPUT <whitespaces> LIST <whitespaces> LIST
```

where,

*   `INPUT` is an input symbol other than the epsilon symbol (`<eps>` by
    default).
*   The special OOV symbol (`<unk>` by default) can appear as `INPUT`, so that
    features can be specified for OOV words.
*   A `LIST` is a `|` -delimited list of symbols without any whitespace. Another
    delimiter can be used by specifying the `-delimiter` flag, as long as it is
    a single character, non-whitespace symbol. An empty list must be explicitly
    written as `<empty>` (or any other symbol specified as the `-empty_symbol`
    flag).
*   The first `LIST` is a list of feature symbols associated with the input
    symbol, for example the symbol itself, the 3-letter suffix of the input, a
    boolean value indicating whether the input is a numeral, etc. This list must
    *not* be empty.
*   The second `LIST` is a list of possible output symbols. When the list is
    empty, this field has a special meaning that the input can pair with all the
    possible output symbols. The set of "all the possible symbols" is the union
    of all output symbols appearing in the third field of all the vocabulary
    records and in the output n-gram of all the feature weights.

Below is an example vocabulary file

```txt
i   0:i   N
am   0:am   V
doing   1:ing   V|N
something   0:something|1:ing   <empty>
<unk>   0:<unk>   <empty>
```

As we can see from the example, it is not required for a word to have all its
possible feature symbols listed out --- as long as those that appear in non-zero
feature weights are listed.

Feature weights need to be grouped into several model files by the feature
template, i.e. all feature weights in a single model file should come from the
same feature template. A model file starts with a line that contains a
non-negative integer which indicates the size of look-ahead window of all the
features in this file. The look-ahead window is the number of input symbols
after the current input position that the decoder needs to know in order to
evaluate feature values for the current input position. For example, a feature
template "current word, next word, current tag" has a look-ahead window of 1;
the emission feature in HMM "current word, current tag" has a look-ahead window
of 0; the transition feature in HMM "previous tag, current tag" also has a
look-ahead window of 0. Note the look-ahead window only applies to the input; on
the output side there is never look ahead. Look-ahead shall not be used
arbitrarily because they incur a rather significant performance penalty.

Following the window size are the lines of actual feature n-grams in this format
below,

```txt
LIST <whitespaces> LIST <whitespaces> NUMBER
```

where

*   `LIST` is in the same format as those in the vocabulary file.
*   The first list is the sequence of input feature symbol n-gram.
*   The second list is the sequence of output n-gram.
*   The two list can be in different sizes and may be empty. However, the size
    of the input sequence must be at least as large as the look-ahead window;
    also for a feature to be useful, the output sequence should not be empty
    (for discriminative models).
*   You may use special symbols that is not part of the feature symbol
    vocabulary or the output symbol vocabulary as sentence boundary symbols
    (e.g. the defaults are `<s>` and `</s>`; you may use a single symbol as
    both). There can be arbitrary number of such symbols in the input. But there
    can only be at most one start-of-sentence and at most one end-of-sentence in
    the output.
*   The number is the weight of this feature, in library-default weight type.

For example, below is a model file of features from the feature template
"current word, next word, current tag", using the vocabulary from the previous
example,

```txt
1
0:am|0:something   V   -1
0:<unk>|0:something   N   -0.5
0:something|0:<unk>   N   -0.75
```

Here is another example with transition features (`NULL` is used as the
boundary)

```txt
0
<empty>   V|V   -1
<empty>   V|N   -2
<empty>   N|V   -2
<empty>   N|N   -1
<empty>   NULL|V -1
<empty>   NULL|N -2
<empty>   V|NULL -2
<empty>   N|NULL -1
```

Suppose our vocabulary file is `vocab.txt`, and the model files are
`model.1.txt`, `model.2.txt`, , `model.N.txt`, invoke the `fstlinear` command
with the following flags to compile a log-linear FST and write the result to
`out.fst`,

```bash
fstlinear -vocab=vocab.txt -out=out.fst -start_symbol=NULL -end_symbol=NULL model.1.txt model.2.txt [
] model.N.txt
```

If the output path is not specified, the output is written to STDOUT by default.

The produced binary FST file can then be used with other commands, such as
`fstcompose`. There is also a `fstloglinearapply` command, which treats the FST
as a log-linear model and normalizes the output sequence for every unique input
sequence. In other words, `fstloglinearapply` treats the input weight as the
(potentially unnormalized) prior probability and obtains the conditional
probability from the linear tagger FST for each unique input sequence and
combines the two to obtain the joint probability of all combinations of
input/output sequences.

A linear classifier FST is similar to a linear tagger FST, with the exception
that it assigns a single class label as the output to each path in the input. It
can also be built with the `fstlinear` command, with the `-classifier` flag on.
`fstloglinearapply` can also be used with a linear classifier FST. There are a
few restrictions on the model files,

*   Each model file must have future 0 because look-ahead is not useful in the
    setting of classification.
*   Each output n-gram must be a unigram, i.e. the output class label.

At the same time, the restriction on feature groups is relaxed. Any feature can
be put into the same group as long as the input n-grams use the same word-level
mapping (i.e. n-grams of words can all go in the same group; n-grams of 3-letter
prefixes can all go in the same group; etc.).

## Look-Ahead FSTs

configure                 | include               | lib
------------------------- | --------------------- | ----------------------------
`--enable-lookahead-fsts` | `<fst/matcher-fst.h>` | `fst/libfstlookahead.{a,so}`

A look-ahead FST is an immutable FST that has a
[lookahead matcher](advanced_usage.md#look-ahead-matchers), selected for more
efficient composition in a particular application. When used in composition, no
special options need to be passed; the appropriate matcher and filter are
selected automatically. This extension has libraries that
[register](advanced_usage.md#input-output) label and transition lookahead FSTs.
See [here](advanced_usage.md#fst-types) for more details, including how to load
dynamically.

## Multi-push-down Transducers (MPDTs)

A multi-push-down transducer (MPDT) extension of the OpenFst library. See
[here](mpdt_extension.md).

## NGram FSTs

configure             | include
--------------------- | ------------------------------------
`--enable-ngram-fsts` | `<fst/extensions/ngram/ngram-fst.h>`

N-gram FSTs are special compact representations of finite state acceptors that
are structured as back-off language models. NGram FSTs are compatible with the
FSTs produced by [OpenGRM](http://www.opengrm.org/). The storage required by
NGram FSTs with standard arcs is approximately 8 bytes per arc plus 8 bytes per
state, plus a small fraction that depends also on the proportion of final
states.

See Jeffrey Sorensen and Cyril Allauzen,
["Unary Data Structures for Language Models"](https://www.openfst.org/twiki/pub/FST/FstExtensions/37218.pdf),
*Interspeech 2011*, International Speech Communication Association, pp.
1425-1428.

## Python Interface

A Python 3-based interface to the OpenFst library. See
[here](python_extension.md).

## Push-down Transducers (PDTs)

A push-down transducer (PDT) extension of the OpenFst library. See
[here](pdt_extension.md).
