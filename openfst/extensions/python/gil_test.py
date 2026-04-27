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
"""Tests GIL releasing.

The tests autoscale; it is very hard to manually hit the right level of
complexity even on a single machine.
"""

import threading
import time

from absl import logging
from absl.testing import absltest
from openfst import pywrapfst as fst


# Thresholds.
MIN_OP_DURATION = 0.05
HEARTBEAT_SLEEP = 0.001
MIN_HEARTBEATS = 2


class GilReleaseTest(absltest.TestCase):

  @classmethod
  def setUpClass(cls):
    """This finds the Goldilocks size for the hard-to-scale algorithms."""
    super().setUpClass()
    # Scales composition (linearly).
    cls.grid_a = None
    for fanout in [10, 20, 40, 80, 160]:
      temp_grid = cls._build_fanout_grid(2000, fanout)
      start = time.time()
      fst.compose(temp_grid, temp_grid)
      duration = time.time() - start
      if duration > MIN_OP_DURATION:
        cls.grid_a = temp_grid
        cls.grid_b = temp_grid.copy()
        logging.info("Goldilocks compose fanout: %d", fanout)
        break
    # Scales determinization (linearly).
    cls.nfa = None
    for n in range(10, 30):
      temp_nfa = cls._build_shifter_nfa(n)
      start = time.time()
      fst.determinize(temp_nfa)
      duration = time.time() - start
      if duration > MIN_OP_DURATION:
        cls.nfa = temp_nfa
        logging.info("Goldilocks determinize n_shift: %d", n)
        break
    # Scales minimization (exponentially).
    for depth in range(10, 22):
      tree = cls._build_tree(depth)
      temp_dfa = fst.determinize(tree)
      start = time.time()
      test_min = temp_dfa.copy()
      test_min.minimize()
      duration = time.time() - start
      if duration > MIN_OP_DURATION:
        cls.dfa = temp_dfa
        logging.info("Goldilocks minimize tree depth: %d", depth)
        break
    # Statically constructs heavyweight FSTs.
    cls.linear = cls._build_linear(1_000_000)
    cls.eps_fst = cls._build_epsilon_dense(1000)

  @staticmethod
  def _build_epsilon_dense(n: int):
    f = fst.VectorFst()
    states = [f.add_state() for _ in range(n)]
    f.set_start(states[0])
    for i in range(n):
      for j in range(i + 1, min(i + 100, n)):
        f.add_arc(states[i], fst.Arc(0, 0, 0.1, states[j]))
      if i < n - 1:
        f.add_arc(states[i], fst.Arc(1, 1, 0.1, states[i + 1]))
    f.set_final(states[-1], 0.0)
    return f

  @staticmethod
  def _build_fanout_grid(states: int, fanout: int):
    f = fst.VectorFst()
    for i in range(states + 1):
      f.add_state()
    f.set_start(0)
    for i in range(states):
      for _ in range(fanout):
        f.add_arc(i, fst.Arc(1, 1, 0.1, i + 1))
    f.set_final(states, 0.0)
    return f

  @staticmethod
  def _build_linear(n: int):
    f = fst.VectorFst()
    s = f.add_state()
    f.set_start(s)
    for i in range(n):
      nxt = f.add_state()
      f.add_arc(s, fst.Arc(i % 100, i % 100, 0.5, nxt))
      s = nxt
    f.set_final(s, 0.0)
    return f

  @staticmethod
  def _build_shifter_nfa(n: int):
    """Language: n-th symbol from end is '1'."""
    f = fst.VectorFst()
    s = f.add_state()
    f.set_start(s)
    f.add_arc(s, fst.Arc(1, 1, 0, s))
    f.add_arc(s, fst.Arc(2, 2, 0, s))
    prev = f.add_state()
    f.add_arc(s, fst.Arc(1, 1, 0, prev))
    for _ in range(n - 1):
      curr = f.add_state()
      f.add_arc(prev, fst.Arc(1, 1, 0, curr))
      f.add_arc(prev, fst.Arc(2, 2, 0, curr))
      prev = curr
    f.set_final(prev, 0.0)
    return f

  @staticmethod
  def _build_tree(layers: int):
    f = fst.VectorFst()
    root = f.add_state()
    f.set_start(root)
    curr = [root]
    for _ in range(layers):
      nxt = []
      for node in curr:
        for label in [1, 2]:
          child = f.add_state()
          f.add_arc(node, fst.Arc(label, label, 0.1, child))
          nxt.append(child)
      curr = nxt
    for node in curr:
      f.set_final(node, 0.0)
    return f

  def _verify_gil_release(self, func, *args, **kwargs):
    op_finished = threading.Event()

    def target():
      func(*args, **kwargs)
      op_finished.set()

    thread = threading.Thread(target=target)
    start_time = time.time()
    thread.start()
    heartbeats = 0
    while not op_finished.is_set():
      time.sleep(HEARTBEAT_SLEEP)
      heartbeats += 1
      if time.time() - start_time > 60.0:
        break
    thread.join()
    duration = time.time() - start_time
    if duration < MIN_OP_DURATION:
      self.skipTest(f"{func.__name__} too fast ({duration:.3f}s)")
    self.assertGreaterEqual(heartbeats, MIN_HEARTBEATS)
    logging.info(
        "%s: %d heartbeats in %.3fs", func.__name__, heartbeats, duration
    )

  def test_arcsort(self):
    self._verify_gil_release(self.linear.copy().arcsort)

  def test_compose(self):
    self._verify_gil_release(fst.compose, self.grid_a, self.grid_b)

  def test_determinize(self):
    self._verify_gil_release(fst.determinize, self.nfa)

  def test_invert(self):
    self._verify_gil_release(self.linear.copy().invert)

  def test_minimize(self):
    self._verify_gil_release(self.dfa.copy().minimize)

  def test_rmepsilon(self):
    self._verify_gil_release(self.eps_fst.copy().rmepsilon)

  def test_shortestpath(self):
    self._verify_gil_release(fst.shortestpath, self.linear)

  def test_topsort(self):
    self._verify_gil_release(self.linear.copy().topsort)


if __name__ == "__main__":
  absltest.main()
