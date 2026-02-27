# Multi-Pushdown Transducer Library (MPDTs)

The multi-pushdown transducer (MPDT) extension extends the
[Pushdown Transducer Library extension](pdt_extension.md) to allow for multiple
stacks. As with the PDT extension, an MPDT is encoded as an FST where some
transitions are labeled with open or close parentheses, with each mated pair of
parentheses being assigned to one of the stacks. To be interpreted as an MPDT,
parentheses within a stack must balance on a path.

configure       | include
--------------- | -----------------------------------------------
`--enable-mpdt` | (coming soon) `<fst/extensions/mpdt/mpdtlib.h>`

An MPDT is encoded as an FST where some transitions are labeled with open or
close parentheses, with each mated pair of parentheses being assigned to one of
the stacks. To be interpreted as an MPDT, parentheses within a stack must
balance on a path. The subset of the transducer labels that correspond to
parenthesis (open, closed) pairs is designated from the C++-library in a
`vector<pair<Label, Label>>`, and the assignments are given as a correponding
`vector<typename Arc::Label>`. From the command line the parentheses and
assignments are passed from a file of lines of label pairs, with a third column
showing the stack affiliation (in a 1-based enumeration) of each pair (using the
flag `--mpdt_parentheses`).

The implementation supports multiple stacks, but the library only exposes two
stacks. In addition there are three push-pop disciplines, `READ_RESTRICT`,
`WRITE_RESTRICT`, `NO_RESTRICT`. With `READ_RESTRICT`, one can only read (i.e.
pop) from the first empty stack. So if the first stack is not empty, one can
continue to write (push) to the second stack, but one cannot pop. With
`WRITE_RESTRICT`, one can only write (i.e. push) to the first empty stack. With
`NO_RESTRICT`, as the name implies, there are no restrictions on read-write
operations. Note that a `NO_RESTRICT` machine is Turing-equivalent. The default
configuration is `READ_RESTRICT`.

The following operations, many which have FST analogues (but are distinguished
in C++ by having a `vector<pair<Label, Label>>` parenthesis pair argument), are
provided for MPDTs:

Operation | Usage                                                                             | Description                                              | Complexity
--------- | --------------------------------------------------------------------------------- | -------------------------------------------------------- | ----------
Compose   | `Compose(a_pdt, parens, b_fst, &c_pdt);`                                          | compose an MPDT and an FST with MPDT result (Bar-Hillel) | Same as FST [composition](compose.md)
&nbsp;    | `Compose(a_fst, b_mpdt, parens, assignments, &c_mpdt);`                           |                                                          |
&nbsp;    | `mpdtcompose -mpdt_parentheses=mpdt.parens a.mpdt b.fst >c.mpdt`                  |                                                          |
&nbsp;    | `mpdtcompose -mpdt_parentheses=mpdt.parens -left_mpdt=false a.fst b.mpdt >c.mpdt` |                                                          |
Expand    | `Expand(a_mpdt, parens, assignments, &b_fst);`                                    | expands a (bounded-stack) MPDT as an FST                 | Time, Space: $O(e^{O(V + E)})$ for the default 2-stack read-restrict configuration
&nbsp;    | `mpdtexpand -mpdt_parentheses=mpdt.parens a.mpdt >b.fst`                          |                                                          |
Info      | `mpdtinfo -mpdt_parentheses=mpdt.parens a.mpdt`                                   | prints out information about an MPDT                     |
Reverse   | `Reverse(a_mpdt, parens, &b_mpdt);`                                               | reverses an MPDT; also reverses the stack assignments    | Time, Space: $O(V + E)$
&nbsp;    | `mpdtreverse -mpdt_parentheses=mpdt.parens a.mpdt >b.mpdt`                        |                                                          |

There are also delayed versions of these algorithms where possible. See the
header files for additional information including options. Note with this
FST-based representation of MPDTs, many FST operations (e.g., `Concat`,
`Closure`, `Rmepsilon`, `Union`) work equally well on MPDTs as FSTs.

As an example of this representation and these algorithms, consider an MPDT that
implements the *ww* copy language over the alphabet {1, 2}.

```bash
fstcompile >mpdt.fst <<EOF
0   1   1   1
0   2   2   2
0   3   0   0
1   0   3   3
2   0   4   4
3   4   5   5
3   5   6   6
3   6   0   0
4   3   7   7
5   3   8   8
6   7   0   1
6   8   0   2
6
7   6   9   9
8   6   10   10
EOF
```

with parentheses:

```bash
cat >mpdt.parens <<EOF
3   5   1
4   6   1
7   9   2
8   10   2
EOF
```

and input:

```bash
fstcompile >input.fst <<EOF
0   1   1   1
1   2   2   2
2   3   2   2
3   4   2   2
4   5   2   2
5   6   1   1
6   7   1   1
7   8   2   2
8   9   2   2
9
EOF
```

Then the following pipeline:

```bash
mpdtcompose --left_mpdt=false --mpdt_parentheses=parens input.fst redup.fst |
mpdtexpand --mpdt_parentheses=parens |
fstproject --project_output |
fstrmepsilon |
fsttopsort |
fstprint --acceptor
```

will produce the following output:

```txt
0   1   1
1   2   2
2   3   2
3   4   2
4   5   2
5   6   1
6   7   1
7   8   2
8   9   2
9   10   1          # Copy starts here
10   11   2
11   12   2
12   13   2
13   14   2
14   15   1
15   16   1
16   17   2
17   18   2
18
```
