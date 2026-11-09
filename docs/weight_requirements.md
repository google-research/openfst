# FST Weight Requirements

A `Weight` class must have binary functions `Plus` and `Times` and static member
functions `Zero()` and `One()` (often denoted $\oplus$, $\otimes$, $0$,
and $1$ in [semiring](glossary.md#semiring) notation), forming (at least) a
left or right semiring with the following properties:

*   `Plus`: associative, commutative, and has `Zero()` as its identity.
*   `Times`: associative, has `One()` as its identity, distributes w.r.t.
    `Plus`, and has `Zero()` as an annihilator:
    $\text{Times}(\text{Zero}(), a) = \text{Zero}()$ and
    $\text{Times}(a, \text{Zero}()) = \text{Zero}()$.

A left semiring distributes on the left; a right semiring distributes on the
right.

In addition, the following must be defined for a `Weight`:

*   `Member`: predicate on set membership.
*   `NoWeight`: static member function that returns an element that is not a set
    member; used to signal an error.
*   `>>`: reads textual representation of a weight.
*   `<<`: prints textual representation of a weight.
*   `Read(istream &)`: reads binary representation of a weight.
*   `Write(ostream &)`: writes binary representation of a weight.
*   `Hash`: maps weight to `size_t`.
*   `ApproxEqual`: approximate equality (for inexact weights).
*   `Quantize`: quantizes w.r.t. delta (for inexact weights).
*   `Divide`: for all $a, b, a', b', c$:
    *   *Left semiring*: if $\text{Times}(a, b) = c$,
        $b' = \text{Divide}(c, a, \mathtt{DIVIDE\_LEFT})$, and
        $b'.\text{Member}()$, then $\text{Times}(a, b') = c$.
    *   *Right semiring*: if $\text{Times}(a, b) = c$,
        $a' = \text{Divide}(c, b, \mathtt{DIVIDE\_RIGHT})$, and
        $a'.\text{Member}()$, then $\text{Times}(a', b) = c$.
    *   *Commutative semiring*:
        $\text{Divide}(c, a) = \text{Divide}(c, a, \mathtt{DIVIDE\_ANY})$
        $= \text{Divide}(c, a, \mathtt{DIVIDE\_LEFT})$
        $= \text{Divide}(c, a, \mathtt{DIVIDE\_RIGHT})$, and if
        $\text{Times}(a, b) = c$,
        $b' = \text{Divide}(c, a, \mathtt{DIVIDE\_ANY})$, and
        $b'.\text{Member}()$, then
        $\text{Times}(a, b') = \text{Times}(b', a) = c$.
*   `ReverseWeight`: the type of the corresponding reverse weight. Typically the
    same type as `Weight` for a (both left and right) semiring. For the left
    string semiring, it is the right string semiring.
*   `Reverse`: a mapping from `Weight` to `ReverseWeight` s.t. for all $a, b$:

    *   $\text{Reverse}(\text{Reverse}(a)) = a$.
    *   $\text{Reverse}(\text{Plus}(a, b)) =$
        $\text{Plus}(\text{Reverse}(a), \text{Reverse}(b))$.
    *   $\text{Reverse}(\text{Times}(a, b)) =$
        $\text{Times}(\text{Reverse}(b), \text{Reverse}(a))$.

    Typically the identity mapping in a (both left and right) semiring. In the
    left string semiring, it maps to the reverse string in the right string
    semiring.
*   `Properties`: static member function returning a bitmask of properties that
    hold:

    *   `kLeftSemiring`: indicates weights form a left semiring: for all
        $a, b, c$, $\text{Times}(c, \text{Plus}(a, b)) =$
        $\text{Plus}(\text{Times}(c, a), \text{Times}(c, b))$.
    *   `kRightSemiring`: indicates weights form a right semiring: for all
        $a, b, c$, $\text{Times}(\text{Plus}(a, b), c) =$
        $\text{Plus}(\text{Times}(a, c), \text{Times}(b, c))$.
    *   `kCommutative`: for all $a, b$,
        $\text{Times}(a, b) = \text{Times}(b, a)$.
    *   `kIdempotent`: for all $a$, $\text{Plus}(a, a) = a$.
    *   `kPath`: for all $a, b$, $\text{Plus}(a, b) = a$ or
        $\text{Plus}(a, b) = b$.
