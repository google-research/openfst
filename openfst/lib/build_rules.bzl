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

""" Portable library target definition.

This serves as a central place to put all target dependent stuff.
Targeting Android and Windows.
"""

load("@rules_cc//cc:defs.bzl", "cc_library")

# Load build tooling macros
def register_extension_info(*args, **kwargs):
    return None

COMMON_DEPS = [
]

def fst_cc_library(
        copts = [],
        deps = [],
        defines = [],
        **kwargs):
    portable_deps = [
        dep
        for dep in deps
        if dep not in COMMON_DEPS
    ]
    cc_library(
        deps = select({
            "@platforms//os:linux": deps,
            "//conditions:default": portable_deps,
        }),
        **kwargs
    )

register_extension_info(
    extension = fst_cc_library,
    label_regex_for_dep = "{extension_name}",
)
