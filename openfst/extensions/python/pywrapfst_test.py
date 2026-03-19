# Copyright 2026 The OpenFst Authors.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# See www.openfst.org for extensive documentation on this weighted
# finite-state transducer library.
"""Tests for Python interface to FST scriptland."""

import filecmp
import itertools
import os
import pathlib
import pickle
import unittest

from absl.testing import absltest
from absl.testing import parameterized
from openfst import pywrapfst as fst


class PywrapFstTest(parameterized.TestCase):

  def setUp(self):
    super().setUp()
    self.tempdir = absltest.TEST_TMPDIR.value
    self.far_testdir = (
        pathlib.Path(".") /
        "openfst/extensions/far/testdata"
    )
    self.fst_testdir = (
        pathlib.Path(".") /
        "openfst/test/testdata"
    )
    # This FST is used anywhere any ole FST would do.
    self.f = fst.MutableFst.read(self.fst_testdir / "compile/fst.compiled")

  # Tests encode mappers.

  def testEncodeMapperPicklingRoundtrip(self):
    f = fst.MutableFst.read(self.fst_testdir / "fst-file/vector1.fst")
    g = f.copy()
    mapper = fst.EncodeMapper(f.arc_type())
    # Neither `encode_labels` nor `encode_weights` are set.
    self.assertFalse(mapper.flags())
    self.assertFalse(mapper.flags() & fst.EncodeMapperFlags.ENCODE_LABELS)
    self.assertFalse(mapper.flags() & fst.EncodeMapperFlags.ENCODE_WEIGHTS)
    f.encode(mapper)
    mapper = pickle.loads(pickle.dumps(mapper))
    g.encode(mapper)
    self.assertTrue(fst.equal(f, g))

  # Tests FSTs.

  def testFstRead(self):
    """Tests file reading."""
    compact = fst.Fst.read(self.fst_testdir / "fst-file/compact1.fst")
    self.assertEqual(compact.fst_type(), "compact_acceptor")
    self.assertStartsWith(repr(compact), "<compact_acceptor Fst at")
    const = fst.Fst.read(self.fst_testdir / "fst-file/const1.fst")
    self.assertEqual(const.fst_type(), "const")
    self.assertStartsWith(repr(const), "<const Fst at")
    self.assertIsNotNone(compact.num_states_if_known())
    ns = 0
    for _ in compact.states():
      ns += 1
    self.assertEqual(compact.num_states_if_known(), ns)

    # Destructive operations can't be performed on const FSTs because they don't
    # have the necessary member functions.
    with self.assertRaises(AttributeError):
      const.project("input")  # pytype: disable=attribute-error
    # Tests whether a Python Unicode string can be passed as an argument, a
    # necessary but not sufficient condition for Python 3 support.
    with self.assertRaises(fst.FstIOError):
      unused_f = fst.MutableFst.read(u"*Иöñéχıßþęη✝File*")

  def testFstFileSerializationRoundTrip(self):
    cases = {
        "fst-file/compact1.fst": "compact_acceptor",
        "fst-file/const1.fst": "const",
        "fst-file/vector1.fst": "vector"
    }
    for (source, expected_type) in cases.items():
      f = fst.Fst.read(self.fst_testdir / source)
      # Fsts FST type.
      self.assertEqual(f.fst_type(), expected_type)
      # Tests round trip serialization.
      sink = os.path.join(self.tempdir, "copy.fst")
      f.write(sink)
      g = fst.Fst.read(sink)
      self.assertTrue(fst.equal(f, g))
      # Cleans up.
      os.remove(sink)

  def testFstStringSerializationRoundTrip(self):
    cases = {
        "fst-file/compact1.fst": "compact_acceptor",
        "fst-file/const1.fst": "const",
        "fst-file/vector1.fst": "vector"
    }
    for (source, expected_type) in cases.items():
      with open(self.fst_testdir / source, "rb") as source:
        string = source.read()
        f = fst.Fst.read_from_string(string)
        # Tests FST type.
        self.assertEqual(f.fst_type(), expected_type)
        # Tests roundtrip serialization.
        g = fst.Fst.read_from_string(f.write_to_string())
        self.assertTrue(fst.equal(f, g))

  def testFstPicklingRoundtrip(self):
    cases = {
        "fst-file/compact1.fst": "compact_acceptor",
        "fst-file/const1.fst": "const",
        "fst-file/vector1.fst": "vector"
    }
    for (source, expected_type) in cases.items():
      f = fst.Fst.read(self.fst_testdir / source)
      self.assertEqual(f.fst_type(), expected_type)
      g = pickle.loads(pickle.dumps(f))
      self.assertTrue(fst.equal(f, g))
      self.assertEqual(type(f), type(g))

  def testFstCreate(self):
    """Tests FST creation."""
    arc_types = ("standard", "log", "log64")
    for arc_type in arc_types:
      f = fst.VectorFst(arc_type)
      self.assertEqual(f.arc_type(), arc_type)
    for arc_type in arc_types:
      with self.assertRaises(TypeError):
        unused_f = fst.VectorFst(arc_type.encode())  # pytype: disable=wrong-arg-types
    with self.assertRaises(fst.FstOpError):
      f = fst.VectorFst("nonexistent")

  def testFstCopy(self):
    """Tests FST copy operation."""
    f1 = self.f.copy()
    f2 = self.f.copy()
    # Both copies are acyclic...
    self.assertEqual(f1.properties(fst.ACYCLIC, True), fst.ACYCLIC)
    self.assertEqual(f2.properties(fst.ACYCLIC, True), fst.ACYCLIC)
    # Until we destructively compute the closure of f2, which makes it cyclic.
    # The original is unaffected.
    f2.closure()
    self.assertEqual(f1.properties(fst.ACYCLIC, True), fst.ACYCLIC)
    self.assertEqual(f2.properties(fst.CYCLIC, True), fst.CYCLIC)
    # Attempts to copy an errorful FST should itself produce an error.
    f1.set_properties(fst.ERROR, fst.ERROR)
    with self.assertRaises(fst.FstOpError):
      unused_fst = f1.copy()

  def testFstErrors(self):
    """Generates an illogical FST and check the error bits."""
    f = fst.MutableFst.read(self.fst_testdir / "minimize/m5.fst")
    # Read in a transducer.
    self.assertFalse(f.properties(fst.ACCEPTOR, True))
    # Difference is only defined for acceptors, so this returns a machine with
    # kError set.
    with self.assertRaises(fst.FstOpError):
      unused_fst = fst.difference(f, f)
    # All the following uses cheese names as values for string options.
    with self.assertRaises(fst.FstArgError):
      unused_fst = f.rmepsilon(queue_type="Edam")  # pytype: disable=wrong-arg-types
    with self.assertRaises(fst.FstArgError):
      unused_fst = fst.compose(f, f, compose_filter="Stinking Bishop")  # pytype: disable=wrong-arg-types
    with self.assertRaises(fst.FstArgError):
      unused_fst = fst.randgen(f, select="Fynbo")  # pytype: disable=wrong-arg-types
    with self.assertRaises(fst.FstArgError):
      pairs = enumerate((f, f, f), 1)
      unused_fst = fst.replace(pairs, call_arc_labeling="Ilchester")  # pytype: disable=wrong-arg-types

  def testFstConversionFailures(self):
    """Tests whether illogical conversions throw the correct exceptions."""
    f = fst.Fst.read(self.fst_testdir / "fst-file/const1.fst")
    with self.assertRaises(fst.FstOpError):
      unused_fst = fst.convert(f, fst_type="nonexistent")

  def testFstIndexErrors(self):
    """Tests out of range errors caused by improper access or mutation."""
    f = fst.VectorFst("standard")
    # _Fst access operations.
    with self.assertRaises(fst.FstIndexError):
      f.final(-1)
    with self.assertRaises(fst.FstIndexError):
      f.num_arcs(-1)
    with self.assertRaises(fst.FstIndexError):
      f.num_input_epsilons(-1)
    with self.assertRaises(fst.FstIndexError):
      f.num_output_epsilons(-1)
    # _MutableFst mutation operations.
    with self.assertRaises(fst.FstIndexError):
      f.add_arc(-1, fst.Arc(0, 0, fst.Weight("tropical", 0), 0))
    with self.assertRaises(fst.FstIndexError):
      f.delete_arcs(-1)
    with self.assertRaises(fst.FstIndexError):
      f.delete_states([-1])
    with self.assertRaises(fst.FstIndexError):
      f.reserve_arcs(-1, 10)
    with self.assertRaises(fst.FstIndexError):
      f.set_final(-1, fst.Weight("tropical", 0))

  # Tests symbol tables.

  def testSymbolTableBasics(self):
    """Cf. symbol-table_test."""
    # Tests reading of non-existent inputs.
    with self.assertRaises(fst.FstIOError):
      unused_s = fst.SymbolTable.read("nonexistent")
    with self.assertRaises(fst.FstIOError):
      unused_s = fst.SymbolTable.read_text("nonexistent")
    with self.assertRaises(fst.FstIOError):
      unused_f = fst.SymbolTable.read_fst("nonexistent", True)
    # Tests round trip with text files.
    source = fst.SymbolTable.read_text(
        self.fst_testdir / "symbol-table/phones.map"
    )
    sink_source = self.tempdir + "/phones.map"
    source.write_text(sink_source)
    sink = fst.SymbolTable.read_text(sink_source)
    self.assertEqual(source.available_key(), sink.available_key())
    self.assertEqual(source.checksum(), sink.checksum())
    self.assertEqual(source.labeled_checksum(), sink.labeled_checksum())
    self.assertEqual(source.num_symbols(), sink.num_symbols())
    # Tests read-write with binary files.
    source.write(sink_source)
    sink = fst.SymbolTable.read(sink_source)
    self.assertEqual(source.available_key(), sink.available_key())
    self.assertEqual(source.checksum(), sink.checksum())
    self.assertEqual(source.labeled_checksum(), sink.labeled_checksum())
    self.assertEqual(source.num_symbols(), sink.num_symbols())
    # Tests adding symbols.
    s1 = fst.SymbolTable()
    s2 = fst.SymbolTable()
    self.assertEqual(100, s1.add_symbol("A", 100))
    self.assertEqual(100, s1.add_symbol("A", 101))
    self.assertEqual(100, s1.find("A"))
    self.assertEqual(100, s2.add_symbol("A", 100))
    self.assertEqual(100, s2.add_symbol("B", 100))
    self.assertEqual(100, s2.find("A"))
    self.assertEqual(100, s2.find("B"))
    self.assertEqual("B", s2.find(100))
    # Tests get_nth_key.
    phones = source
    symbols = fst.SymbolTable("dense-phones")
    for i in range(phones.num_symbols()):
      self.assertEqual(i, symbols.num_symbols())
      orig_key = phones.get_nth_key(i)
      key = symbols.add_symbol(phones.find(orig_key), orig_key)
      self.assertEqual(orig_key, key)
    # Tests adding negative labels.
    symbols.add_symbol("neg", -2)
    self.assertEqual("neg", symbols.find(-2))
    self.assertEqual(-2, symbols.find("neg"))
    # Manually merges two symbol tables (with conflicts) into another.
    t1 = fst.SymbolTable.read_text(
        self.fst_testdir / "symbol-table-ops/t1.map"
    )
    t2 = fst.SymbolTable.read_text(
        self.fst_testdir / "symbol-table-ops/t2.map"
    )
    t1.add_table(t2)
    self.assertEqual(t1.num_symbols(), 6)
    self.assertEqual(t1.find("a"), 2)
    self.assertEqual(t1.find(2), "a")
    self.assertEqual(t1.find("b"), 3)
    self.assertEqual(t1.find(3), "b")
    # Tests membership queries.
    self.assertFalse(t1.member("nonexistent"))
    self.assertNotIn("nonexistent", t1)
    self.assertFalse(t1.member(1024))
    self.assertNotIn(1024, t1)
    # Tests queries for nonexistent keys and indices.
    self.assertEqual(fst.NO_SYMBOL, t1.find("nonexistent"))
    self.assertEqual("", t1.find(1024))
    # Tests Pythonic symbol table iteration.
    for unused_index, unused_symbol in t1:
      pass

  def testSymbolTableSpaceSymbolReadFailsWithoutOption(self):
    # This file uses space as a symbol, so fails with the default separators
    # of both space and tab.
    with self.assertRaises(fst.FstIOError):
      unused_symbols = fst.SymbolTable.read_text(
          self.fst_testdir / "symbol-table/space.map"
      )

  def testSymbolTableSpaceSymbolReadSucceedsWithOption(self):
    # This file uses space as a symbol, so we use tab as a separator.
    symbols = fst.SymbolTable.read_text(
        self.fst_testdir / "symbol-table/space.map",
        sep="\t",
    )
    self.assertEqual(symbols.num_symbols(), 3)
    self.assertEqual(symbols.find("x"), 0)
    self.assertEqual(symbols.find(" "), 1)
    self.assertEqual(symbols.find("y"), 2)

  def testSymbolTableNegativeLabelReadSucceeds(self):
    symbols = fst.SymbolTable.read_text(
        self.fst_testdir / "symbol-table/negative.map",
    )
    self.assertEqual(symbols.num_symbols(), 1)
    self.assertEqual(symbols.find("negative_label"), -2)
    self.assertEqual(symbols.find(-2), "negative_label")

  def testSymbolTableNegativeLabelWriteSucceeds(self):
    symbols = fst.SymbolTable()
    symbols.add_symbol("negative_label", -2)
    symbols.write_text(self.tempdir + "/negative.map")

  def testSymbolTableOps(self):
    """Cf. symbol-table-ops_test."""
    # Merges a symbol table
    t1 = fst.SymbolTable.read_text(
        self.fst_testdir / "symbol-table-ops/t1.map"
    )
    t2 = fst.SymbolTable.read_text(
        self.fst_testdir / "symbol-table-ops/t2.map"
    )
    merged = fst.merge_symbol_table(t1, t2)
    self.assertEqual(merged.find("a"), 2)
    self.assertEqual(merged.find("b"), 3)
    self.assertEqual(merged.find("z"), 5)
    # Compacts a symbol table.
    t1_res = fst.compact_symbol_table(
        fst.SymbolTable.read_text(
            self.fst_testdir / "symbol-table-ops/t1.map"
        )
    )
    self.assertEqual(t1_res.find("a"), 1)
    self.assertEqual(t1_res.find("b"), 2)
    self.assertEqual(t1_res.find("c"), 3)
    # Reads a symbol table from an on-disk FST.
    t3 = fst.SymbolTable.read_fst(self.fst_testdir / "symbol-table-ops/t3.fst",
                                  True)
    self.assertEqual(t3.find("a"), 1)
    t3 = fst.SymbolTable.read_fst(self.fst_testdir / "symbol-table-ops/t3.fst",
                                  False)
    self.assertEqual(t3.find("z"), 1)

  def testSymbolTableAccess(self):
    """Attempts to access and use symbol tables."""
    f = self.f.copy()
    # Access.
    self.assertEqual(f.mutable_input_symbols().name(), "isyms.map")
    self.assertEqual(f.mutable_output_symbols().name(), "osyms.map")
    self.assertEqual(f.mutable_input_symbols().find("a"), 1)
    self.assertEqual(f.mutable_output_symbols().find("y"), 2)
    # Mutation.
    f.set_input_symbols(fst.SymbolTable("empty"))
    f.set_output_symbols(fst.SymbolTable("empty"))
    syms = fst.SymbolTable.read_text(self.fst_testdir / "compile/isyms.map")
    f.set_input_symbols(syms)
    syms = fst.SymbolTable.read_text(self.fst_testdir / "compile/osyms.map")
    f.set_output_symbols(syms)
    self.assertEqual(f.mutable_input_symbols().find("a"), 1)
    self.assertEqual(f.mutable_output_symbols().find("y"), 2)
    # Copy mutation.
    f = fst.MutableFst.read(self.fst_testdir / "symbols/s2.fst")
    self.assertEqual(f.input_symbols().find("a"), 1)
    self.assertEqual(f.output_symbols().find(1), "a")
    syms = f.input_symbols().copy()
    syms.add_symbol("*BrandNewSymbol*")
    syms = f.output_symbols().copy()
    syms.add_symbol("*BrandNewSymbol*")

  def testSymbolTablePicklingRoundtrip(self):
    f = fst.SymbolTable.read_text(self.fst_testdir / "compile/isyms.map")
    g = pickle.loads(pickle.dumps(f))
    self.assertEqual(f.labeled_checksum(), g.labeled_checksum())

  # Tests FST compilation.

  def testCompilation(self):
    """Cf. compiler-main_test."""
    isyms = fst.SymbolTable.read_text(self.fst_testdir / "compile/isyms.map")
    osyms = fst.SymbolTable.read_text(self.fst_testdir / "compile/osyms.map")
    compiler = fst.Compiler(
        isymbols=isyms, osymbols=osyms, keep_isymbols=True, keep_osymbols=True)
    with open(self.fst_testdir / "compile/fst.txt", "r") as source:
      compiler.write(source.read())
    f_res = compiler.compile()
    self.assertTrue(fst.equal(self.f, f_res))

  def testCompilerFromString(self):
    """Passes string literals to Compiler.write and verifies the result."""
    compiler = fst.Compiler()
    compiler.write("0 1 2 3")
    compiler.write("0 2 3 4 5.6")
    compiler.write("1")
    compiler.write("2 3.4")
    f = compiler.compile()
    self.assertEqual(f.start(), 0)
    arcs = [a for a in f.arcs(f.start())]
    self.assertLen(arcs, 2)

    self.assertEqual(arcs[0].ilabel, 2)
    self.assertEqual(arcs[0].olabel, 3)
    self.assertEqual(arcs[0].weight, fst.Weight.one("tropical"))
    self.assertEqual(arcs[0].nextstate, 1)

    self.assertEqual(arcs[1].ilabel, 3)
    self.assertEqual(arcs[1].olabel, 4)
    self.assertEqual(arcs[1].weight, fst.Weight("tropical", 5.6))
    self.assertEqual(arcs[1].nextstate, 2)

    self.assertEqual(f.final(0), fst.Weight.zero("tropical"))
    self.assertEqual(f.final(1), fst.Weight.one("tropical"))
    self.assertEqual(f.final(2), fst.Weight("tropical", 3.4))

  # Tests weight wrapper.

  def testWeight(self):
    """Cf. script_test.cc."""
    for weight_type in ("tropical", "log", "log64"):
      w1 = fst.Weight(weight_type, 1)
      self.assertTrue(w1.member())
      w2 = fst.Weight(weight_type, 1)
      self.assertTrue(w2.member())
      # Tests comparison of normal weights.
      self.assertEqual(w1, w2)
      # Tests comparison of different initializations of "special" weights.
      w1 = fst.Weight.zero(weight_type)
      w2 = fst.Weight(weight_type, "Infinity")
      self.assertEqual(w1, w2)
      w1 = fst.Weight.one(weight_type)
      w2 = fst.Weight(weight_type, 0)
      self.assertEqual(w1, w2)
      # Tries to do arithmetic with non-matching weight types.
      for weight_type_prime in ("tropical", "log", "log64"):
        if weight_type == weight_type_prime:
          continue
        with self.assertRaises(fst.FstArgError):
          unused_w = fst.plus(w1, fst.Weight.one(weight_type_prime))
        with self.assertRaises(fst.FstArgError):
          unused_w = fst.times(w1, fst.Weight.one(weight_type_prime))
        with self.assertRaises(fst.FstArgError):
          unused_w = fst.divide(w1, fst.Weight.one(weight_type_prime))
      # Tests no_weight.
      w1 = fst.Weight.no_weight(weight_type)
      self.assertFalse(w1.member())
      w2 = fst.Weight.no_weight(weight_type)
      self.assertFalse(w2.member())
      self.assertEqual(str(w1), str(w2))
      # Tries to create nonexistent weight.
      with self.assertRaises(fst.FstArgError):
        unused_w = fst.Weight("nonexistent", 1)
      with self.assertRaises(fst.FstBadWeightError):
        unused_w = fst.Weight("tropical", "nonexistent")
    # Does arithmetic with weights.
    w1 = fst.Weight.one("tropical")
    self.assertEqual(w1.properties() & fst.IDEMPOTENT, fst.IDEMPOTENT)
    w2 = fst.Weight("tropical", 2)
    self.assertEqual(w1, fst.plus(w1, w2))
    self.assertEqual(w2, fst.times(w1, w2))
    w1 = fst.Weight.one("log")
    self.assertNotEqual(w1.properties() & fst.IDEMPOTENT, fst.IDEMPOTENT)
    w2 = fst.Weight("log", 2)
    self.assertNotEqual(w1, fst.plus(w1, w2))
    self.assertEqual(w2, fst.times(w1, w2))
    # Tries to pass bad weights to functions taking weight args.
    f = fst.VectorFst()
    i = f.add_state()
    with self.assertRaises(fst.FstBadWeightError):
      f.set_final(i, "nonexistent")

  def testWeightTypeMismatch(self):
    """Tests mutation operations using weights of the wrong type."""
    f = fst.VectorFst("standard")
    i = f.add_state()
    o = f.add_state()
    wrong_weight = fst.Weight.one("log64")
    with self.assertRaises(fst.FstOpError):
      f.copy().add_arc(i, fst.Arc(0, 0, wrong_weight, o))
    with self.assertRaises(fst.FstOpError):
      unused_fst = fst.determinize(f.copy(), weight=wrong_weight)
    with self.assertRaises(fst.FstOpError):
      unused_fst = fst.disambiguate(f.copy(), weight=wrong_weight)
    with self.assertRaises(fst.FstOpError):
      f.copy().prune(weight=wrong_weight)
    with self.assertRaises(fst.FstOpError):
      f.copy().rmepsilon(weight=wrong_weight)
    with self.assertRaises(fst.FstOpError):
      f.copy().set_final(i, wrong_weight)

  # Tests FST arc and state iteration.

  def testArcIteration(self):
    """Tests arc iteration."""
    # Arc iteration using the `arcs` method.
    arcs = list(self.f.arcs(self.f.start()))
    self.assertLen(arcs, self.f.num_arcs(self.f.start()))
    # Mutable arc iteration using the `mutable_arcs` method.
    f = self.f.copy()
    w = "2"
    it = f.mutable_arcs(f.start())
    arc = it.value().copy()
    arc.weight = w
    it.set_value(arc)
    self.assertEqual(str(it.value().weight), w)

  def testArcIterationFinality(self):
    arciter = self.f.arcs(self.f.start())
    self.assertEqual(arciter.flags(), fst.ArcIteratorFlags.ARC_VALUE_FLAGS)
    while not arciter.done():
      arciter.next()
    self.assertTrue(arciter.done())
    # Cowardly refuses to return an arc that doesn't exist.
    with self.assertRaises(fst.FstOpError):
      unused_a = arciter.value()

  def testArcIterationSetFlags(self):
    arciter = self.f.arcs(self.f.start())
    arciter.set_flags(
        fst.ArcIteratorFlags.ARC_VALUE_FLAGS
        | fst.ArcIteratorFlags.ARC_I_LABEL_VALUE,
        fst.ArcIteratorFlags.ARC_VALUE_FLAGS,
    )
    self.assertEqual(arciter.flags(), fst.ArcIteratorFlags.ARC_VALUE_FLAGS)

  def testMutableArcIteration(self):
    orig_arcs = list(self.f.arcs(self.f.start()))
    it = self.f.mutable_arcs(self.f.start())
    while not it.done():
      arc = it.value().copy()
      arc.nextstate = 0
      it.set_value(arc)
      it.next()
    for (orig_a, new_a) in zip(orig_arcs, self.f.arcs(self.f.start())):
      self.assertEqual(new_a.nextstate, 0)
      self.assertEqual(new_a.ilabel, orig_a.ilabel)
      self.assertEqual(new_a.olabel, orig_a.olabel)

  def testMutableArcIterationFinality(self):
    it = self.f.mutable_arcs(self.f.start())
    a = it.value()
    self.assertFalse(it.done())
    while not it.done():
      it.next()
    self.assertTrue(it.done())
    # Cowardly refuses to return an arc that doesn't exist.
    with self.assertRaises(fst.FstOpError):
      unused_a = it.value()
    # Cowardly refuses to allow setting an arc that doesn't exist.
    with self.assertRaises(fst.FstOpError):
      it.set_value(a)

  # Tests FAR reading and writing.

  def testFar(self):
    # This flexes pywrapfst handling of os.PathLike objects too.
    f1 = fst.MutableFst.read(self.far_testdir / "test1-01.fst")
    f2 = fst.MutableFst.read(self.far_testdir / "test1-02.fst")
    f3 = fst.MutableFst.read(self.far_testdir / "test1-03.fst")
    pairs = {"1": f1, "2": f2, "3": f3}
    # STTable.
    writer = fst.FarWriter.create(
        self.tempdir + "/test.far", arc_type=f1.arc_type(), far_type="sttable")
    self.assertStartsWith(repr(writer), "<sttable FarWriter at")
    writer.add("1", f1)
    writer.add("2", f2)
    writer.add("3", f3)
    del writer
    reader = fst.FarReader.open(self.tempdir + "/test.far")
    self.assertStartsWith(repr(reader), "<sttable FarReader at")
    self.assertEqual(reader.far_type(), "sttable")
    for i in range(2):
      if i > 0:
        reader.reset()
      self.assertFalse(reader.done())
      self.assertEqual(reader.get_key(), "1")
      self.assertTrue(fst.equal(reader.get_fst(), f1))
      reader.next()
      self.assertFalse(reader.done())
      self.assertEqual(reader.get_key(), "2")
      self.assertTrue(fst.equal(reader.get_fst(), f2))
      reader.next()
      self.assertFalse(reader.done())
      self.assertEqual(reader.get_key(), "3")
      self.assertTrue(fst.equal(reader.get_fst(), f3))
      reader.next()
      self.assertTrue(reader.done())
    self.assertTrue(reader.find("2"))
    self.assertFalse(reader.done())
    self.assertEqual(reader.get_key(), "2")
    # STTable with Pythonic API.
    writer = fst.FarWriter.create(
        self.tempdir + "/test.far", arc_type=f1.arc_type(), far_type="sttable")
    for (k, f) in sorted(pairs.items()):
      writer[k] = f
    del writer
    reader = fst.FarReader.open(self.tempdir + "/test.far")
    self.assertEqual(reader.far_type(), "sttable")
    for (k, f) in reader:
      self.assertTrue(fst.equal(pairs[k], f))
    # STList.
    writer = fst.FarWriter.create(
        self.tempdir + "/test.far", arc_type=f1.arc_type(), far_type="stlist")
    self.assertStartsWith(repr(writer), "<stlist FarWriter at")
    writer.add("1", f1)
    writer.add("2", f2)
    writer.add("3", f3)
    del writer
    reader = fst.FarReader.open(self.tempdir + "/test.far")
    self.assertStartsWith(repr(reader), "<stlist FarReader at")
    self.assertEqual(reader.far_type(), "stlist")
    self.assertFalse(reader.done())
    self.assertEqual(reader.get_key(), "1")
    self.assertTrue(fst.equal(reader.get_fst(), f1))
    reader.next()
    self.assertFalse(reader.done())
    self.assertEqual(reader.get_key(), "2")
    self.assertTrue(fst.equal(reader.get_fst(), f2))
    reader.next()
    self.assertFalse(reader.done())
    self.assertEqual(reader.get_key(), "3")
    self.assertTrue(fst.equal(reader.get_fst(), f3))
    reader.next()
    self.assertTrue(reader.done())
    # STList with Pythonic API.
    writer = fst.FarWriter.create(
        self.tempdir + "/test.far", arc_type=f1.arc_type(), far_type="stlist")
    for (k, f) in sorted(pairs.items()):
      writer[k] = f
    del writer
    reader = fst.FarReader.open(self.tempdir + "/test.far")
    self.assertEqual(reader.far_type(), "stlist")
    for (k, f) in reader:
      self.assertTrue(fst.equal(pairs[k], f))

  # Tests core FST operations.

  def testArcMap(self):
    """Cf. map-main_test."""
    # Identity mapping.
    m1 = fst.MutableFst.read(self.fst_testdir / "arc-map/m1.fst")
    m1_res = fst.arcmap(m1, map_type="identity")
    self.assertTrue(fst.equal(m1, m1_res))
    # Input epsilon mapping.
    m13 = fst.MutableFst.read(self.fst_testdir / "arc-map/m13.fst")
    m13_res = fst.arcmap(m1, map_type="input_epsilon")
    self.assertTrue(fst.equal(m13, m13_res))
    # Inversion mapping.
    m7 = fst.MutableFst.read(self.fst_testdir / "arc-map/m7.fst")
    m7_res = fst.arcmap(m1, map_type="invert")
    self.assertTrue(fst.equal(m7, m7_res))
    # Quantization mapping.
    m8 = fst.MutableFst.read(self.fst_testdir / "arc-map/m8.fst")
    m8_res = fst.arcmap(m1, map_type="quantize", delta=2)
    self.assertTrue(fst.equal(m8, m8_res))
    # Weight-removal mapping.
    m6 = fst.MutableFst.read(self.fst_testdir / "arc-map/m6.fst")
    m6_res = fst.arcmap(m1, map_type="rmweight")
    self.assertTrue(fst.equal(m6, m6_res))
    # Output epsilon mapping.
    m14 = fst.MutableFst.read(self.fst_testdir / "arc-map/m14.fst")
    m14_res = fst.arcmap(m14, map_type="output_epsilon")
    self.assertTrue(fst.equal(m14, m14_res))
    # Power mapping.
    m15 = fst.MutableFst.read(self.fst_testdir / "arc-map/m15.fst")
    m15_res = fst.arcmap(m1, map_type="power", power=.5)
    self.assertTrue(fst.equal(m15, m15_res))
    m16 = fst.MutableFst.read(self.fst_testdir / "arc-map/m16.fst")
    m16_res = fst.arcmap(m1, map_type="power", power=2)
    self.assertTrue(fst.equal(m16, m16_res))
    # Plus mapping.
    m9 = fst.MutableFst.read(self.fst_testdir / "arc-map/m9.fst")
    m9_res = fst.arcmap(m1, map_type="plus", weight=2)
    self.assertTrue(fst.equal(m9, m9_res))
    # Superfinal mapping.
    m4 = fst.MutableFst.read(self.fst_testdir / "arc-map/m4.fst")
    m4_res = fst.arcmap(m1, map_type="superfinal")
    self.assertTrue(fst.equal(m4, m4_res))
    # Times mapping.
    m10 = fst.MutableFst.read(self.fst_testdir / "arc-map/m10.fst")
    m10_res = fst.arcmap(m1, map_type="times", weight=2)
    self.assertTrue(fst.equal(m10, m10_res))
    # To-log mapping.
    m11 = fst.MutableFst.read(self.fst_testdir / "arc-map/m11.fst")
    m11_res = fst.arcmap(m1, map_type="to_log")
    self.assertTrue(fst.equal(m11, m11_res))
    # To-log64 mapping....
    m12 = fst.MutableFst.read(self.fst_testdir / "arc-map/m12.fst")
    m12_res = fst.arcmap(m1, map_type="to_log64")
    self.assertTrue(fst.equal(m12, m12_res))
    # To-std mapping.
    m1_res = fst.arcmap(m11, map_type="to_standard")
    self.assertTrue(fst.equal(m1, m1_res))
    m1_res = fst.arcmap(m12, map_type="to_standard")
    self.assertTrue(fst.equal(m1, m1_res))
    # Mapping with a nonexisent mapper type.
    with self.assertRaises(fst.FstArgError):
      unused_fst = fst.arcmap(m1, map_type="nonexistent")  # pytype: disable=wrong-arg-types

  def testArcSort(self):
    """Cf. arcsort-main_test."""
    # Input label sort.
    a1 = fst.MutableFst.read(self.fst_testdir / "arcsort/a1.fst")
    a2 = fst.MutableFst.read(self.fst_testdir / "arcsort/a2.fst")
    a2.arcsort("ilabel")
    self.assertTrue(fst.equal(a1, a2))
    # Output label sort.
    a1 = fst.MutableFst.read(self.fst_testdir / "arcsort/a1.fst")
    a2 = fst.MutableFst.read(self.fst_testdir / "arcsort/a2.fst")
    a1.arcsort("olabel")
    self.assertTrue(fst.equal(a1, a2))

  def testClosure(self):
    """Cf. closure-main_test."""
    # *-closure.
    c1 = fst.MutableFst.read(self.fst_testdir / "closure/c1.fst")
    c2 = fst.MutableFst.read(self.fst_testdir / "closure/c2.fst")
    c1.closure("star")
    self.assertTrue(fst.equal(c1, c2))
    # +-closure.
    c1 = fst.MutableFst.read(self.fst_testdir / "closure/c1.fst")
    c3 = fst.MutableFst.read(self.fst_testdir / "closure/c3.fst")
    c1.closure("plus")
    self.assertTrue(fst.equal(c1, c3))

  def testCompose(self):
    """Cf. compose-main_test."""
    c1 = fst.MutableFst.read(self.fst_testdir / "compose/c1.fst")
    c2 = fst.MutableFst.read(self.fst_testdir / "compose/c2.fst")
    c3 = fst.MutableFst.read(self.fst_testdir / "compose/c3.fst")
    for cfilter in ("alt_sequence", "match", "no_match", "null", "sequence",
                    "trivial"):
      c3_res = fst.compose(c1, c2, compose_filter=cfilter)
      self.assertTrue(fst.equal(c3, c3_res))
    # Composition with non-existent filter.
    with self.assertRaises(fst.FstArgError):
      unused_fst = fst.compose(c1, c2, compose_filter="nonexistent")  # pytype: disable=wrong-arg-types

  def testConcat(self):
    """Cf. concat-main_test."""
    c1 = fst.MutableFst.read(self.fst_testdir / "concat/c1.fst")
    c2 = fst.MutableFst.read(self.fst_testdir / "concat/c2.fst")
    c3 = fst.MutableFst.read(self.fst_testdir / "concat/c3.fst")
    c1.concat(c2)
    self.assertTrue(fst.equal(c1, c3))

  def testConnect(self):
    """Cf. connect-main_test."""
    c1 = fst.MutableFst.read(self.fst_testdir / "connect/c1.fst")
    c2 = fst.MutableFst.read(self.fst_testdir / "connect/c2.fst")
    c1.connect()
    self.assertTrue(fst.equal(c1, c2))

  def testConvert(self):
    """Cf. convert-main_test."""
    c1 = fst.MutableFst.read(self.fst_testdir / "convert/c1.fst")
    # Vector-to-const conversion.
    cnst = fst.convert(c1, "const")
    self.assertTrue(fst.equal(cnst, c1))
    self.assertEqual(cnst.fst_type(), "const")
    # Const-to-vector conversion.
    vec = fst.convert(cnst, "vector")
    self.assertTrue(fst.equal(vec, c1))
    self.assertTrue(vec.fst_type(), "vector")
    # Conversion to non-existent type.
    with self.assertRaises(fst.FstOpError):
      unused_fst = fst.convert(vec, "nonexistent")

  def testDeterminize(self):
    """Cf. determinize-main_test."""
    # Acceptor determinization.
    d1 = fst.MutableFst.read(self.fst_testdir / "determinize/d1.fst")
    d2 = fst.MutableFst.read(self.fst_testdir / "determinize/d2.fst")
    d2_res = fst.determinize(d1)
    self.assertTrue(fst.equal(d2_res, d2))
    # Transducer determinization.
    d3 = fst.MutableFst.read(self.fst_testdir / "determinize/d3.fst")
    d4 = fst.MutableFst.read(self.fst_testdir / "determinize/d4.fst")
    d4_res = fst.determinize(d3)
    self.assertTrue(fst.equal(d4_res, d4))
    # Pruned determinization.
    d5 = fst.MutableFst.read(self.fst_testdir / "determinize/d5.fst")
    d5_res = fst.determinize(d5, weight=.5, nstate=10, subsequential_label=0)
    self.assertTrue(fst.equal(d5, d5_res))
    # Determinization of non-existent type.
    with self.assertRaises(fst.FstArgError):
      unused_fst = fst.determinize(d1, det_type="nonexistent")  # pytype: disable=wrong-arg-types

  def testDifference(self):
    """Cf. difference-main_test."""
    d1 = fst.MutableFst.read(self.fst_testdir / "difference/d1.fst")
    d2 = fst.MutableFst.read(self.fst_testdir / "difference/d2.fst")
    d3 = fst.MutableFst.read(self.fst_testdir / "difference/d3.fst")
    d3_res = fst.difference(d1, d2, connect=False)
    self.assertTrue(fst.equal(d3, d3_res))

  def testDisambiguate(self):
    """Cf. disambiguate-main_test."""
    d1 = fst.MutableFst.read(self.fst_testdir / "disambiguate/d1.fst")
    d2 = fst.MutableFst.read(self.fst_testdir / "disambiguate/d2.fst")
    d2_res = fst.disambiguate(d1)
    self.assertTrue(fst.equal(d2, d2_res))

  def testEncodeAndDecode(self):
    e1 = fst.MutableFst.read(self.fst_testdir / "encode/e1.fst")
    e1_cd = fst.MutableFst.read(self.fst_testdir / "encode/e1_cd.fst")
    # Label encoding
    e1_res = e1.copy()
    encoder = fst.EncodeMapper(e1_res.arc_type(), encode_labels=True)
    self.assertEqual(encoder.flags(), fst.EncodeMapperFlags.ENCODE_LABELS)
    e1_res.encode(encoder)
    e1_res.decode(encoder)
    self.assertTrue(fst.equal(e1, e1_cd))
    # Weight encoding.
    e1_res = e1.copy()
    encoder = fst.EncodeMapper(e1_res.arc_type(), encode_weights=True)
    self.assertEqual(encoder.flags(), fst.EncodeMapperFlags.ENCODE_WEIGHTS)
    e1_res.encode(encoder)
    e1_res.decode(encoder)
    self.assertTrue(fst.equal(e1_res, e1_cd))
    # Label and weight encoding.
    e1_res = e1.copy()
    encoder = fst.EncodeMapper(
        e1_res.arc_type(), encode_labels=True, encode_weights=True)
    self.assertEqual(
        encoder.flags(),
        fst.EncodeMapperFlags.ENCODE_LABELS
        | fst.EncodeMapperFlags.ENCODE_WEIGHTS,
    )
    e1_res.encode(encoder)
    e1_res.decode(encoder)
    self.assertTrue(fst.equal(e1_res, e1_cd))

  def testEpsNormalize(self):
    """Cf. epsnormalize-main_test."""
    # Input normalization.
    e1 = fst.MutableFst.read(self.fst_testdir / "epsnormalize/e1.fst")
    e2 = fst.MutableFst.read(self.fst_testdir / "epsnormalize/e2.fst")
    e2_res = fst.epsnormalize(e1)
    self.assertTrue(fst.equal(e2, e2_res))
    # Output normalization.
    e3 = fst.MutableFst.read(self.fst_testdir / "epsnormalize/e3.fst")
    e4 = fst.MutableFst.read(self.fst_testdir / "epsnormalize/e4.fst")
    e4_res = fst.epsnormalize(e3, "output")
    self.assertTrue(fst.equal(e4, e4_res))

  def testEquivalent(self):
    """Cf. equivalent_test."""
    e1 = fst.MutableFst.read(self.fst_testdir / "equivalent/e1.fst")
    e2 = fst.MutableFst.read(self.fst_testdir / "equivalent/e2.fst")
    e3 = fst.MutableFst.read(self.fst_testdir / "equivalent/e3.fst")
    for (a, b) in itertools.combinations((e1, e2, e3), 2):
      self.assertTrue(fst.equivalent(a, b))
    e4 = fst.MutableFst.read(self.fst_testdir / "equivalent/e4.fst")
    for a in (e1, e2, e3):
      self.assertFalse(fst.equivalent(a, e4))

  @parameterized.named_parameters(("_e2", "e2"), ("_e3", "e3"))
  def testEquivalentRaisesErrorOnDifferentSymbols(self, cmp_fst_name):
    """If argument FST symbols do not match, raise an exception."""
    e1 = fst.MutableFst.read(self.fst_testdir / "equivalent/e1.fst")
    e_cmp = fst.MutableFst.read(
        self.fst_testdir / f"equivalent/{cmp_fst_name}.fst"
    )
    isyms = e1.input_symbols().copy()
    isyms.add_symbol("z")
    e1.set_input_symbols(isyms)
    self.assertTrue(fst.equivalent(e1, e1))
    self.assertRaises(fst.FstOpError, fst.equivalent, e_cmp, e1)
    self.assertRaises(fst.FstOpError, fst.equivalent, e1, e_cmp)

  @parameterized.named_parameters(("_e2", "e2"), ("_e3", "e3"))
  def testEquivalentRaisesErrorOnEpsilon(self, cmp_fst_name):
    """If argument FST has an epsilon arc, raise an exception."""
    e1 = fst.MutableFst.read(self.fst_testdir / "equivalent/e1.fst")
    e_cmp = fst.MutableFst.read(
        self.fst_testdir / f"equivalent/{cmp_fst_name}.fst"
    )
    e1.add_arc(1, fst.Arc(0, 0, 0, 0))
    self.assertEqual(
        e1.properties(fst.FstProperties.EPSILONS, True),
        fst.FstProperties.EPSILONS,
    )
    self.assertRaises(fst.FstOpError, fst.equivalent, e1, e1)
    self.assertRaises(fst.FstOpError, fst.equivalent, e_cmp, e1)
    self.assertRaises(fst.FstOpError, fst.equivalent, e1, e_cmp)

  @parameterized.named_parameters(("_e2", "e2"), ("_e3", "e3"))
  def testEquivalentRaisesErrorOnNfsa(self, cmp_fst_name):
    """If argument FST is not input deterministic, raise an exception."""
    e1 = fst.MutableFst.read(self.fst_testdir / "equivalent/e1.fst")
    e_cmp = fst.MutableFst.read(
        self.fst_testdir / f"equivalent/{cmp_fst_name}.fst"
    )
    e1.add_arc(1, fst.Arc(1, 1, 0, 0))
    self.assertEqual(
        e1.properties(fst.FstProperties.NON_I_DETERMINISTIC, True),
        fst.FstProperties.NON_I_DETERMINISTIC,
    )
    self.assertRaises(fst.FstOpError, fst.equivalent, e1, e1)
    self.assertRaises(fst.FstOpError, fst.equivalent, e_cmp, e1)
    self.assertRaises(fst.FstOpError, fst.equivalent, e1, e_cmp)

  @parameterized.named_parameters(("_e2", "e2"), ("_e3", "e3"))
  def testEquivalentRaisesErrorOnTransducer(self, cmp_fst_name):
    """If argument FST is not an acceptor, raise an exception."""
    e1 = fst.MutableFst.read(self.fst_testdir / "equivalent/e1.fst")
    e_cmp = fst.MutableFst.read(
        self.fst_testdir / f"equivalent/{cmp_fst_name}.fst"
    )
    e1.add_arc(1, fst.Arc(2, 1, 0, 0))
    self.assertEqual(
        e1.properties(fst.FstProperties.NOT_ACCEPTOR, True),
        fst.FstProperties.NOT_ACCEPTOR,
    )
    self.assertRaises(fst.FstOpError, fst.equivalent, e1, e1)
    self.assertRaises(fst.FstOpError, fst.equivalent, e_cmp, e1)
    self.assertRaises(fst.FstOpError, fst.equivalent, e1, e_cmp)

  def testIntersect(self):
    """Cf. intersect-main_test."""
    i1 = fst.MutableFst.read(self.fst_testdir / "intersect/i1.fst")
    i2 = fst.MutableFst.read(self.fst_testdir / "intersect/i2.fst")
    i3 = fst.MutableFst.read(self.fst_testdir / "intersect/i3.fst")
    i3_res = fst.intersect(i1, i2)
    self.assertTrue(i3, i3_res)

  def testInvert(self):
    """Cf. invert-main_test."""
    i1 = fst.MutableFst.read(self.fst_testdir / "invert/i1.fst")
    i2 = fst.MutableFst.read(self.fst_testdir / "invert/i2.fst")
    i2.invert()
    self.assertTrue(fst.equal(i1, i2))

  def testMinimize(self):
    """Cf. minimize-main_test."""
    # Minimization of acyclic acceptor.
    m1 = fst.MutableFst.read(self.fst_testdir / "minimize/m1.fst")
    m1.minimize()
    acyclic_min = fst.MutableFst.read(
        self.fst_testdir / "minimize/acyclic_min.fst"
    )
    self.assertTrue(fst.equal(m1, acyclic_min))
    # Minimization of cyclic acceptor.
    m2 = fst.MutableFst.read(self.fst_testdir / "minimize/m2.fst")
    m2.minimize()
    cyclic_min = fst.MutableFst.read(
        self.fst_testdir / "minimize/cyclic_min.fst"
    )
    self.assertTrue(fst.equal(m2, cyclic_min))
    # Minimization of weighted acyclic acceptor.
    m3 = fst.MutableFst.read(self.fst_testdir / "minimize/m3.fst")
    m3.minimize()
    weighted_acyclic_min = fst.MutableFst.read(
        self.fst_testdir / "minimize/weighted_acyclic_min.fst")
    self.assertTrue(fst.equal(m3, weighted_acyclic_min))
    # Minimization of weighted cyclic acceptor.
    m4 = fst.MutableFst.read(self.fst_testdir / "minimize/m4.fst")
    m4.minimize()
    weighted_cyclic_min = fst.MutableFst.read(
        self.fst_testdir / "minimize/weighted_cyclic_min.fst")
    self.assertTrue(fst.equal(m4, weighted_cyclic_min))
    # Minimization of weighted acyclic transducer.
    m5 = fst.MutableFst.read(self.fst_testdir / "minimize/m5.fst")
    m5.minimize()
    transducer_acyclic_min = fst.MutableFst.read(
        self.fst_testdir / "minimize/transducer_acyclic_min.fst")
    self.assertTrue(fst.equal(m5, transducer_acyclic_min))
    # Minimization of weighted cyclic transducer.
    m6 = fst.MutableFst.read(self.fst_testdir / "minimize/m6.fst")
    m6.minimize()
    transducer_cyclic_min = fst.MutableFst.read(
        self.fst_testdir / "minimize/transducer_cyclic_min.fst")
    self.assertTrue(fst.equal(m6, transducer_cyclic_min))

  def testProject(self):
    """Cf. project-main_test."""
    # Input projection.
    p1_input = fst.MutableFst.read(self.fst_testdir / "project/p1.fst")
    p1_input.project("input")
    p2 = fst.MutableFst.read(self.fst_testdir / "project/p2.fst")
    self.assertTrue(fst.equal(p1_input, p2))
    # Output projection.
    p1_output = fst.MutableFst.read(self.fst_testdir / "project/p1.fst")
    p1_output.project("output")
    p3 = fst.MutableFst.read(self.fst_testdir / "project/p3.fst")
    self.assertTrue(fst.equal(p1_output, p3))

  def testPrune(self):
    """Cf. prune-main_test."""
    p1 = fst.MutableFst.read(self.fst_testdir / "prune/p1.fst")
    p2 = fst.MutableFst.read(self.fst_testdir / "prune/p2.fst")
    p2_res = fst.prune(p1, weight=.5)
    self.assertTrue(fst.equal(p2, p2_res))

  def testPush(self):
    """Cf. push-main_test."""
    # Constructive pushing.
    p1 = fst.MutableFst.read(self.fst_testdir / "push/p1.fst")
    p2 = fst.MutableFst.read(self.fst_testdir / "push/p2.fst")
    p2_res = fst.push(p1, push_weights=True, reweight_type="to_initial")
    self.assertTrue(fst.equal(p2, p2_res))
    p3 = fst.MutableFst.read(self.fst_testdir / "push/p3.fst")
    p3_res = fst.push(p1, push_weights=True, reweight_type="to_final")
    self.assertTrue(fst.equal(p3, p3_res))
    p4 = fst.MutableFst.read(self.fst_testdir / "push/p4.fst")
    p4_res = fst.push(p1, push_labels=True, reweight_type="to_initial")
    self.assertTrue(fst.equal(p4, p4_res))
    p5 = fst.MutableFst.read(self.fst_testdir / "push/p5.fst")
    p5_res = fst.push(p1, push_labels=True, reweight_type="to_final")
    self.assertTrue(fst.equal(p5, p5_res))
    # Destructive pushing.
    p1_m = fst.MutableFst.read(self.fst_testdir / "push/p1.fst")
    p1_m.push(reweight_type="to_initial")
    self.assertTrue(fst.equal(p1_m, p2))
    p1_m = fst.MutableFst.read(self.fst_testdir / "push/p1.fst")
    p1_m.push(reweight_type="to_final")
    self.assertTrue(fst.equal(p1_m, p3))

  def testRandEquivalent(self):
    e1 = fst.MutableFst.read(self.fst_testdir / "equivalent/e1.fst")
    e2 = fst.MutableFst.read(self.fst_testdir / "equivalent/e2.fst")
    self.assertTrue(fst.randequivalent(e1, e2, npath=25, seed=218))
    e5 = fst.MutableFst.read(self.fst_testdir / "equivalent/e5.fst")
    e6 = fst.MutableFst.read(self.fst_testdir / "equivalent/e6.fst")
    self.assertTrue(fst.randequivalent(e5, e6, npath=25, seed=218))

  # TODO: Investigate.
  # OSS notes: Below we expect the test to fail, which is the current behavior
  # on Linux. But on macOS machines the test actually passes.
  # b/476053592: Fails due to std::uniform_int_distribution.
  @unittest.expectedFailure
  def testRandGen(self):
    """Cf. randgen_test."""
    r1 = fst.MutableFst.read(self.fst_testdir / "randgen/r1.fst")
    r2 = fst.MutableFst.read(self.fst_testdir / "randgen/r2.fst")
    r2_res = fst.randgen(r1, seed=2)
    self.assertTrue(fst.equal(r2, r2_res))

  def testRelabel(self):
    """Cf. relabel-main_test."""
    ## SymbolTable-based relabeling.
    # Input symbol relabeling.
    r1 = fst.MutableFst.read(self.fst_testdir / "relabel/r1.fst")
    r2 = fst.MutableFst.read(self.fst_testdir / "relabel/r2.fst")
    in2 = fst.SymbolTable.read_text(self.fst_testdir / "relabel/in2.map")
    r1.relabel_tables(new_isymbols=in2)
    self.assertTrue(fst.equal(r1, r2))
    # Output symbol relabeling.
    r1 = fst.MutableFst.read(self.fst_testdir / "relabel/r1.fst")
    r3 = fst.MutableFst.read(self.fst_testdir / "relabel/r3.fst")
    out3 = fst.SymbolTable.read_text(self.fst_testdir / "relabel/out3.map")
    r1.relabel_tables(new_osymbols=out3)
    self.assertTrue(fst.equal(r1, r3))
    ## Pairs-based relabeling.
    # Input symbol relabeling.
    r4 = fst.MutableFst.read(self.fst_testdir / "relabel/r4.fst")
    r5 = fst.MutableFst.read(self.fst_testdir / "relabel/r5.fst")
    r4.relabel_pairs(ipairs=[(1, 2), (2, 1)])
    self.assertTrue(fst.equal(r4, r5))
    # Output symbol relabeling.
    r4 = fst.MutableFst.read(self.fst_testdir / "relabel/r4.fst")
    r6 = fst.MutableFst.read(self.fst_testdir / "relabel/r6.fst")
    r4.relabel_pairs(opairs=[(1, 2), (2, 3), (3, 4), (4, 1)])
    self.assertTrue(fst.equal(r4, r6))

  def testReplace(self):
    """Cf. replace-main_test."""
    g1 = fst.MutableFst.read(self.fst_testdir / "replace/g1.fst")
    g2 = fst.MutableFst.read(self.fst_testdir / "replace/g2.fst")
    g3 = fst.MutableFst.read(self.fst_testdir / "replace/g3.fst")
    g4 = fst.MutableFst.read(self.fst_testdir / "replace/g4.fst")
    g = fst.MutableFst.read(self.fst_testdir / "replace/g_out.fst")
    pairs = enumerate((g1, g2, g3, g4), 1)
    g_res = fst.replace(pairs, call_arc_labeling="neither")
    self.assertTrue(fst.equal(g, g_res))

  def testReweight(self):
    """Cf. reweight-main_test."""
    # These weights will be interpreted as tropical, given the arc and weight
    # types of the FSTs below.
    potentials = (2., 3., -1.)
    # Reweighting towards initial state.
    r1 = fst.MutableFst.read(self.fst_testdir / "reweight/r1.fst")
    r1.reweight(potentials)
    r2 = fst.MutableFst.read(self.fst_testdir / "reweight/r2.fst")
    self.assertTrue(fst.equal(r1, r2))
    # Reweighting towards final state.
    r1 = fst.MutableFst.read(self.fst_testdir / "reweight/r1.fst")
    r1.reweight(potentials, reweight_type="to_final")
    r3 = fst.MutableFst.read(self.fst_testdir / "reweight/r3.fst")
    self.assertTrue(fst.equal(r1, r3))

  def testReverse(self):
    """Cf. reverse-main_test."""
    r1 = fst.MutableFst.read(self.fst_testdir / "reverse/r1.fst")
    r2 = fst.MutableFst.read(self.fst_testdir / "reverse/r2.fst")
    r2_res = fst.reverse(r1)
    self.assertTrue(fst.equal(r2, r2_res))

  def testRmEpsilon(self):
    r1_m = fst.MutableFst.read(self.fst_testdir / "rmepsilon/r1.fst")
    r2_m = fst.MutableFst.read(self.fst_testdir / "rmepsilon/r2.fst")
    r1_m.rmepsilon()
    r2_m.rmepsilon()
    self.assertTrue(fst.equal(r1_m, r2_m))
    r1_m = fst.MutableFst.read(self.fst_testdir / "rmepsilon/r1.fst")
    r4 = fst.MutableFst.read(self.fst_testdir / "rmepsilon/r4.fst")
    r1_m.rmepsilon(weight=1., nstate=10)
    self.assertTrue(fst.equal(r1_m, r4))

  def testShortestDistance(self):
    """Cf. shortest-distance-main_test."""
    sd1 = fst.MutableFst.read(self.fst_testdir / "shortest-distance/sd1.fst")
    sd1_wt = sd1.weight_type()
    sd1_dis = [fst.Weight(sd1_wt, i) for i in [0, 3, 5, 7]]
    self.assertEqual(sd1_dis, fst.shortestdistance(sd1))
    sd2 = fst.MutableFst.read(self.fst_testdir / "shortest-distance/sd2.fst")
    sd2_wt = sd2.weight_type()
    sd2_dis = [fst.Weight(sd2_wt, i) for i in [0, 3, 5, 7]]
    self.assertEqual(sd2_dis, fst.shortestdistance(sd2))

  def testShortestPath(self):
    """Cf. shortest-path-main_test."""
    sp1 = fst.MutableFst.read(self.fst_testdir / "shortest-path/sp1.fst")
    sp2 = fst.MutableFst.read(self.fst_testdir / "shortest-path/sp2.fst")
    sp3 = fst.MutableFst.read(self.fst_testdir / "shortest-path/sp3.fst")
    sp4 = fst.MutableFst.read(self.fst_testdir / "shortest-path/sp4.fst")
    sp5 = fst.MutableFst.read(self.fst_testdir / "shortest-path/sp5.fst")
    sp9 = fst.MutableFst.read(self.fst_testdir / "shortest-path/sp9.fst")
    sp2_res = fst.shortestpath(sp1)
    self.assertTrue(fst.equal(sp2, sp2_res))
    sp3_res = fst.shortestpath(sp1, nshortest=4)
    self.assertTrue(fst.equal(sp3, sp3_res))
    sp4_res = fst.shortestpath(sp1, nshortest=4, unique=True)
    self.assertTrue(fst.equal(sp4, sp4_res))
    sp9_res = fst.shortestpath(
        sp5, nshortest=4, unique=False, nstate=20, weight=1.)
    self.assertTrue(fst.equal(sp9, sp9_res))

  def testStateMap(self):
    """Cf. map-main_test."""
    # Arc-sum mapping.
    a1 = fst.MutableFst.read(self.fst_testdir / "state-map/a1.fst")
    a2 = fst.MutableFst.read(self.fst_testdir / "state-map/a2.fst")
    a2_res = fst.statemap(a1, "arc_sum")
    self.assertTrue(fst.equal(a2, a2_res))
    # Arc-unique mapping.
    b1 = fst.MutableFst.read(self.fst_testdir / "state-map/b1.fst")
    b2 = fst.MutableFst.read(self.fst_testdir / "state-map/b2.fst")
    b2_res = fst.statemap(b1, "arc_unique")
    self.assertTrue(fst.equal(b2, b2_res))

  def testSynchronize(self):
    """Cf. synchronize-main-test."""
    s1 = fst.MutableFst.read(self.fst_testdir / "synchronize/s1.fst")
    s2 = fst.MutableFst.read(self.fst_testdir / "synchronize/s2.fst")
    s2_res = fst.synchronize(s1)
    self.assertTrue(fst.equal(s2, s2_res))

  def testTopSort(self):
    """Cf. topsort-main_test."""
    t1_m = fst.MutableFst.read(self.fst_testdir / "topsort/t1.fst")
    t2 = fst.MutableFst.read(self.fst_testdir / "topsort/t2.fst")
    t1_m.topsort()
    self.assertTrue(fst.equal(t1_m, t2))

  def testUnion(self):
    """Cf. union-main_test."""
    u1_m = fst.MutableFst.read(self.fst_testdir / "union/u1.fst")
    u2 = fst.MutableFst.read(self.fst_testdir / "union/u2.fst")
    u3 = fst.MutableFst.read(self.fst_testdir / "union/u3.fst")
    u1_m.union(u2)
    self.assertTrue(fst.equal(u1_m, u3))

  # Tests FST drawing and printing operations.

  def testDraw(self):
    """Cf. draw-main_test."""
    f = fst.MutableFst.read(self.fst_testdir / "draw/draw.fst")
    sink_source = self.tempdir + "/draw-5p.dot"
    f.draw(sink_source)
    self.assertTrue(
        filecmp.cmp(
            self.fst_testdir / "draw/draw-5p.dot", sink_source, shallow=False
        )
    )

  def testPrinting(self):
    """Cf. print-main_test."""
    f = fst.MutableFst.read(self.fst_testdir / "print/p1.fst")
    # Print without input/output symbol tables.
    result = f.print()
    with open(self.fst_testdir / "print/p1-nosyms.txt", "rb") as source:
      output = source.read().decode("utf8")
    self.assertEqual(result, output)
    # Print with both input and output symbol tables.
    syms = fst.SymbolTable.read_text(self.fst_testdir / "print/p1.map")
    result = f.print(isymbols=syms, osymbols=syms)
    with open(self.fst_testdir / "print/p1-syms.txt", "rb") as source:
      output = source.read().decode("utf8")
    self.assertEqual(result, output)
    # Print with pre-attached input and output symbol tables.
    f = fst.MutableFst.read(self.fst_testdir / "print/p2.fst")
    result = f.print()
    with open(self.fst_testdir / "print/p2.txt", "rb") as source:
      output = source.read().decode("utf8")
    self.assertEqual(result, output)

  def testFstPropertiesTruthiness(self):
    self.assertTrue(fst.ACCEPTOR)
    self.assertFalse(fst.ACCEPTOR & fst.NOT_ACCEPTOR)
    self.assertTrue(fst.ACCEPTOR | fst.NOT_ACCEPTOR)
    self.assertEqual(fst.ACCEPTOR, fst.ACCEPTOR | fst.ACCEPTOR)
    self.assertTrue(fst.ACCEPTOR | (fst.ACCEPTOR & fst.NOT_ACCEPTOR))
    self.assertTrue(fst.FST_PROPERTIES & fst.ACCEPTOR)

  def testFstPropertiesDontInteropWithInt(self):
    with self.assertRaises(TypeError):
      unused_result = fst.FST_PROPERTIES & 3  # pytype: disable=unsupported-operands
    with self.assertRaises(TypeError):
      unused_result = fst.FST_PROPERTIES | 3  # pytype: disable=unsupported-operands

  def testFstPropertiesDontSupportMinus(self):
    with self.assertRaises(TypeError):
      unused_result = fst.FST_PROPERTIES - fst.ACCEPTOR  # pytype: disable=unsupported-operands

  def testWeightPropertiesTruthiness(self):
    self.assertTrue(fst.WeightProperties.SEMIRING)
    self.assertFalse(
        fst.WeightProperties.LEFT_SEMIRING & fst.WeightProperties.RIGHT_SEMIRING
    )
    self.assertTrue(
        fst.WeightProperties.LEFT_SEMIRING | fst.WeightProperties.RIGHT_SEMIRING
    )
    self.assertEqual(
        fst.WeightProperties.LEFT_SEMIRING,
        fst.WeightProperties.LEFT_SEMIRING | fst.WeightProperties.LEFT_SEMIRING,
    )
    self.assertTrue(
        fst.WeightProperties.LEFT_SEMIRING
        | (
            fst.WeightProperties.LEFT_SEMIRING
            & fst.WeightProperties.RIGHT_SEMIRING
        )
    )
    self.assertTrue(
        fst.WeightProperties.SEMIRING & fst.WeightProperties.LEFT_SEMIRING
    )

  def testWeightPropertiesDontInteropWithInt(self):
    with self.assertRaises(TypeError):
      unused_result = fst.WeightProperties.SEMIRING & 3  # pytype: disable=unsupported-operands
    with self.assertRaises(TypeError):
      unused_result = fst.WeightProperties.SEMIRING | 3  # pytype: disable=unsupported-operands

  def testWeightPropertiesDontSupportMinus(self):
    with self.assertRaises(TypeError):
      unused_result = (
          fst.WeightProperties.SEMIRING - fst.WeightProperties.LEFT_SEMIRING  # pytype: disable=unsupported-operands
      )

  def testArcIteratorFlagsTruthiness(self):
    self.assertTrue(fst.ArcIteratorFlags.ARC_I_LABEL_VALUE)
    self.assertFalse(
        fst.ArcIteratorFlags.ARC_I_LABEL_VALUE
        & fst.ArcIteratorFlags.ARC_O_LABEL_VALUE
    )
    self.assertTrue(
        fst.ArcIteratorFlags.ARC_I_LABEL_VALUE
        | fst.ArcIteratorFlags.ARC_O_LABEL_VALUE
    )
    self.assertEqual(
        fst.ArcIteratorFlags.ARC_I_LABEL_VALUE,
        fst.ArcIteratorFlags.ARC_I_LABEL_VALUE
        | fst.ArcIteratorFlags.ARC_I_LABEL_VALUE,
    )
    self.assertTrue(
        fst.ArcIteratorFlags.ARC_I_LABEL_VALUE
        | (
            fst.ArcIteratorFlags.ARC_I_LABEL_VALUE
            & fst.ArcIteratorFlags.ARC_O_LABEL_VALUE
        )
    )
    self.assertTrue(
        fst.ArcIteratorFlags.ARC_VALUE_FLAGS
        & fst.ArcIteratorFlags.ARC_I_LABEL_VALUE
    )

  def testArcIteratorFlagsDontInteropWithInt(self):
    with self.assertRaises(TypeError):
      unused_result = fst.ArcIteratorFlags.ARC_VALUE_FLAGS & 3  # pytype: disable=unsupported-operands
    with self.assertRaises(TypeError):
      unused_result = fst.ArcIteratorFlags.ARC_VALUE_FLAGS | 3  # pytype: disable=unsupported-operands

  def testArcIteratorFlagsDontSupportMinus(self):
    with self.assertRaises(TypeError):
      unused_result = (
          fst.ArcIteratorFlags.ARC_VALUE_FLAGS  # pytype: disable=unsupported-operands
          - fst.ArcIteratorFlags.ARC_I_LABEL_VALUE
      )

  def testEncoderMapperFlagsTruthiness(self):
    self.assertTrue(fst.EncodeMapperFlags.ENCODE_LABELS)
    self.assertFalse(
        fst.EncodeMapperFlags.ENCODE_LABELS
        & fst.EncodeMapperFlags.ENCODE_WEIGHTS
    )
    self.assertTrue(
        fst.EncodeMapperFlags.ENCODE_LABELS
        | fst.EncodeMapperFlags.ENCODE_WEIGHTS
    )
    self.assertEqual(
        fst.EncodeMapperFlags.ENCODE_LABELS,
        fst.EncodeMapperFlags.ENCODE_LABELS
        | fst.EncodeMapperFlags.ENCODE_LABELS,
    )
    self.assertTrue(
        fst.EncodeMapperFlags.ENCODE_LABELS
        | (
            fst.EncodeMapperFlags.ENCODE_LABELS
            & fst.EncodeMapperFlags.ENCODE_WEIGHTS
        )
    )
    self.assertTrue(
        (
            fst.EncodeMapperFlags.ENCODE_WEIGHTS
            | fst.EncodeMapperFlags.ENCODE_LABELS
        )
        & fst.EncodeMapperFlags.ENCODE_LABELS
    )

  def testEncoderMapperFlagsDontInteropWithInt(self):
    with self.assertRaises(TypeError):
      unused_result = fst.EncodeMapperFlags.ENCODE_WEIGHTS & 3  # pytype: disable=unsupported-operands
    with self.assertRaises(TypeError):
      unused_result = fst.EncodeMapperFlags.ENCODE_WEIGHTS | 3  # pytype: disable=unsupported-operands

  def testEncoderMapperFlagsDontSupportMinus(self):
    with self.assertRaises(TypeError):
      unused_result = (
          fst.EncodeMapperFlags.ENCODE_WEIGHTS  # pytype: disable=unsupported-operands
          - fst.EncodeMapperFlags.ENCODE_WEIGHTS
      )

if __name__ == "__main__":
  absltest.main()
