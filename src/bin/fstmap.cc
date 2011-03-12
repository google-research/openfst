// fstmap.cc

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
//
// Copyright 2005-2010 Google, Inc.
// Author: riley@google.com (Michael Riley)
// Modified: jpr@google.com (Jake Ratkiewicz) to use FstClass
//
// \file
// Applies an operation to each arc of an FST.
//

#include <string>

#include <fst/script/map.h>

DEFINE_double(delta, fst::kDelta, "Comparison/quantization delta");
DEFINE_string(map_type, "identity",
              "Map operation, one of: \"identity\", \"invert\", "
              " \"plus (--weight)\", \"quantize (--delta)\", \"rmweight\", "
              "\"superfinal\", \"times (--weight)\"\n");
DEFINE_string(weight, "", "Weight parameter");

int main(int argc, char **argv) {
  namespace s = fst::script;
  using fst::script::FstClass;
  using fst::script::MutableFstClass;
  using fst::script::VectorFstClass;

  string usage = "Applies an operation to each arc of an FST.\n\n  Usage: ";
  usage += argv[0];
  usage += " [in.fst [out.fst]]\n";

  std::set_new_handler(FailedNewHandler);
  SetFlags(usage.c_str(), &argc, &argv, true);
  if (argc > 3) {
    ShowUsage();
    return 1;
  }

  string in_name = (argc > 1 && strcmp(argv[1], "-") != 0) ? argv[1] : "";
  string out_name = argc > 2 ? argv[2] : "";

  FstClass *ifst = FstClass::Read(in_name);
  if (!ifst) return 1;

  MutableFstClass *ofst = 0;
  if (ifst->Properties(fst::kMutable, false)) {
    ofst = static_cast<MutableFstClass *>(ifst);
  } else {
    ofst = new VectorFstClass(*ifst);
    delete ifst;
  }

  fst::MapType mt;
  s::WeightClass w = s::WeightClass::Zero();

  if (FLAGS_map_type == "identity") {
    mt = fst::IDENTITY_MAPPER;
  } else if (FLAGS_map_type == "invert") {
    mt = fst::INVERT_MAPPER;
  } else if (FLAGS_map_type == "plus") {
    w = FLAGS_weight.empty() ? s::WeightClass::Zero() :
        s::WeightClass(ofst->WeightType(), FLAGS_weight);
    mt = fst::PLUS_MAPPER;
  } else if (FLAGS_map_type == "quantize") {
    mt = fst::QUANTIZE_MAPPER;
  } else if (FLAGS_map_type == "rmweight") {
    mt = fst::RMWEIGHT_MAPPER;
  } else if (FLAGS_map_type == "superfinal") {
    mt = fst::SUPERFINAL_MAPPER;
  } else if (FLAGS_map_type == "times") {
    w = FLAGS_weight.empty() ? s::WeightClass::One() :
        s::WeightClass(ofst->WeightType(), FLAGS_weight);
    mt = fst::TIMES_MAPPER;
  } else {
    LOG(ERROR) << argv[0] << ": Unknown map type \""
               << FLAGS_map_type << "\"\n";
    return 1;
  }

  s::Map(ofst, mt, FLAGS_delta, w);

  ofst->Write(out_name);

  return 0;
}
