# Copyright 2022 Ant Group Co., Ltd.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.


package(default_visibility = ["//visibility:public"])
load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library", "cc_test")

cc_library(
    name = "apsi_lib",
    hdrs = glob(["include/*.h"]),
    strip_include_prefix = "include",
    # hdrs = [
    #         "//include/apsi_wrapper.h"
    #     ],
    srcs = [
            "lib/libapsi-0.11.a",
            "lib/libseal-4.1.a",
            "lib/libzstd.a",
            "lib/libz.a",
            "lib/libflatbuffers.a",
            "lib/liblog4cplus.a",
            "lib/libkuku-2.1.a",
            "lib/libjsoncpp.a",
            #pthread
        ],
)

cc_library(
    name = "apsi_lib_withoutseal",
    hdrs = glob(["include/*.h"]),
    strip_include_prefix = "include",
    # hdrs = [
    #         "//include/apsi_wrapper.h"
    #     ],
    srcs = [
            "lib/libapsi-0.11.a",
            # "lib/libseal-4.1.a",
            "lib/libzstd.a",
            "lib/libz.a",
            "lib/libflatbuffers.a",
            "lib/liblog4cplus.a",
            "lib/libkuku-2.1.a",
            "lib/libjsoncpp.a",
            #pthread
        ],
)

cc_library(
    name = "apsi_lib_tt",
    hdrs = glob(["include/*.h"]),
    strip_include_prefix = "include",
    # hdrs = [
    #         "//include/apsi_wrapper.h"
    #     ],
    srcs = [
            "lib/libapsi-0.11.a",
            # "lib/libseal-4.1.a",
            "lib/liblog4cplus.a",
            "lib/libjsoncpp.a",
            #pthread
        ],
    deps = [
        "@com_github_facebook_zstd//:zstd",
        "@com_github_microsoft_gsl//:gsl",
        "@com_github_microsoft_kuku//:kuku",
        "@com_github_microsoft_seal//:seal",
        "@com_google_flatbuffers//:flatbuffers",
        "@zlib",
    ],
)
