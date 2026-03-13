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
// The FST script interface permits users to interact with FSTs without knowing
// their arc type. It does this by mapping compile-time polymorphism (in the
// form of a arc-templated FST types) onto a shared virtual interface. It also
// supports arc extension via a DSO interface. Due to the overhead of virtual
// dispatch and registered function lookups, the script API is somewhat slower
// then library API provided by types like StdVectorFst, but has the advantage
// that it is designed not to crash (and to provide useful debugging
// information) upon common user errors like passing invalid indices or
// attempting comparison of incompatible FSTs. It is used both by the FST
// binaries and the Python extension.
//
// This header includes all of the FST script functionality.

#ifndef OPENFST_SCRIPT_FSTSCRIPT_H_
#define OPENFST_SCRIPT_FSTSCRIPT_H_

// Major classes
#include "openfst/script/arciterator-class.h"
// Operations.
#include "openfst/script/arcsort.h"
#include "openfst/script/closure.h"
#include "openfst/script/compile.h"
#include "openfst/script/compose.h"
#include "openfst/script/concat.h"
#include "openfst/script/connect.h"
#include "openfst/script/convert.h"
#include "openfst/script/decode.h"
#include "openfst/script/determinize.h"
#include "openfst/script/difference.h"
#include "openfst/script/disambiguate.h"
#include "openfst/script/draw.h"
#include "openfst/script/encode.h"
#include "openfst/script/epsnormalize.h"
#include "openfst/script/equal.h"
#include "openfst/script/equivalent.h"
#include "openfst/script/fst-class.h"
// Flag-to-enum parsers.
#include "openfst/script/info.h"
#include "openfst/script/intersect.h"
#include "openfst/script/invert.h"
#include "openfst/script/isomorphic.h"
#include "openfst/script/map.h"
#include "openfst/script/minimize.h"
#include "openfst/script/print.h"
#include "openfst/script/project.h"
#include "openfst/script/prune.h"
#include "openfst/script/push.h"
#include "openfst/script/randequivalent.h"
#include "openfst/script/randgen.h"
#include "openfst/script/relabel.h"
#include "openfst/script/replace.h"
#include "openfst/script/reverse.h"
#include "openfst/script/reweight.h"
#include "openfst/script/rmepsilon.h"
// Templates like Operation<> and Apply<>.
#include "openfst/script/script-impl.h"
#include "openfst/script/shortest-distance.h"
#include "openfst/script/shortest-path.h"
#include "openfst/script/stateiterator-class.h"
#include "openfst/script/synchronize.h"
#include "openfst/script/topsort.h"
#include "openfst/script/union.h"
#include "openfst/script/verify.h"

#define REGISTER_FST_OPERATIONS(Arc)                                \
  REGISTER_FST_OPERATION(ArcSort, Arc, FstArcSortArgs);             \
  REGISTER_FST_OPERATION(Closure, Arc, FstClosureArgs);             \
  REGISTER_FST_OPERATION(CompileInternal, Arc, FstCompileArgs);     \
  REGISTER_FST_OPERATION(Compose, Arc, FstComposeArgs);             \
  REGISTER_FST_OPERATION(Concat, Arc, FstConcatArgs1);              \
  REGISTER_FST_OPERATION(Concat, Arc, FstConcatArgs2);              \
  REGISTER_FST_OPERATION(Concat, Arc, FstConcatArgs3);              \
  REGISTER_FST_OPERATION(Connect, Arc, MutableFstClass);            \
  REGISTER_FST_OPERATION(Convert, Arc, FstConvertArgs);             \
  REGISTER_FST_OPERATION(Decode, Arc, FstDecodeArgs);               \
  REGISTER_FST_OPERATION(Determinize, Arc, FstDeterminizeArgs);     \
  REGISTER_FST_OPERATION(Difference, Arc, FstDifferenceArgs);       \
  REGISTER_FST_OPERATION(Disambiguate, Arc, FstDisambiguateArgs);   \
  REGISTER_FST_OPERATION(Draw, Arc, FstDrawArgs);                   \
  REGISTER_FST_OPERATION(Encode, Arc, FstEncodeArgs);               \
  REGISTER_FST_OPERATION(EpsNormalize, Arc, FstEpsNormalizeArgs);   \
  REGISTER_FST_OPERATION(Equal, Arc, FstEqualArgs);                 \
  REGISTER_FST_OPERATION(Equivalent, Arc, FstEquivalentArgs);       \
  REGISTER_FST_OPERATION(Info, Arc, FstInfoArgs);                   \
  REGISTER_FST_OPERATION(InitArcIteratorClass, Arc,                 \
                         InitArcIteratorClassArgs);                 \
  REGISTER_FST_OPERATION(InitMutableArcIteratorClass, Arc,          \
                         InitMutableArcIteratorClassArgs);          \
  REGISTER_FST_OPERATION(InitStateIteratorClass, Arc,               \
                         InitStateIteratorClassArgs);               \
  REGISTER_FST_OPERATION(Intersect, Arc, FstIntersectArgs);         \
  REGISTER_FST_OPERATION(Invert, Arc, MutableFstClass);             \
  REGISTER_FST_OPERATION(Isomorphic, Arc, FstIsomorphicArgs);       \
  REGISTER_FST_OPERATION(Map, Arc, FstMapArgs);                     \
  REGISTER_FST_OPERATION(Minimize, Arc, FstMinimizeArgs);           \
  REGISTER_FST_OPERATION(Print, Arc, FstPrintArgs);                 \
  REGISTER_FST_OPERATION(Project, Arc, FstProjectArgs);             \
  REGISTER_FST_OPERATION(Prune, Arc, FstPruneArgs1);                \
  REGISTER_FST_OPERATION(Prune, Arc, FstPruneArgs2);                \
  REGISTER_FST_OPERATION(Push, Arc, FstPushArgs1);                  \
  REGISTER_FST_OPERATION(Push, Arc, FstPushArgs2);                  \
  REGISTER_FST_OPERATION(RandEquivalent, Arc, FstRandEquivalentArgs); \
  REGISTER_FST_OPERATION(RandGen, Arc, FstRandGenArgs);             \
  REGISTER_FST_OPERATION(Relabel, Arc, FstRelabelArgs1);            \
  REGISTER_FST_OPERATION(Relabel, Arc, FstRelabelArgs2);            \
  REGISTER_FST_OPERATION(Replace, Arc, FstReplaceArgs);             \
  REGISTER_FST_OPERATION(Reverse, Arc, FstReverseArgs);             \
  REGISTER_FST_OPERATION(Reweight, Arc, FstReweightArgs);           \
  REGISTER_FST_OPERATION(RmEpsilon, Arc, FstRmEpsilonArgs);         \
  REGISTER_FST_OPERATION(ShortestDistance, Arc, FstShortestDistanceArgs1); \
  REGISTER_FST_OPERATION(ShortestDistance, Arc, FstShortestDistanceArgs2); \
  REGISTER_FST_OPERATION(ShortestDistance, Arc, FstShortestDistanceArgs3); \
  REGISTER_FST_OPERATION(ShortestPath, Arc, FstShortestPathArgs);   \
  REGISTER_FST_OPERATION(Synchronize, Arc, FstSynchronizeArgs);     \
  REGISTER_FST_OPERATION(TopSort, Arc, FstTopSortArgs);             \
  REGISTER_FST_OPERATION(Union, Arc, FstUnionArgs1);                \
  REGISTER_FST_OPERATION(Union, Arc, FstUnionArgs2);                \
  REGISTER_FST_OPERATION(Verify, Arc, FstVerifyArgs)


#endif  // OPENFST_SCRIPT_FSTSCRIPT_H_
