load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library", "cc_test")
#load("@rules_foreign_cc//foreign_cc:defs.bzl", "cmake", "configure_make")

INCLUDE_FALGS = [
    #"-I//src/config",
]
WARNING_FLAGS = [
    "-Wall",
    "-Wextra",
    "-Werror",
    "-Wno-unused-parameter",
    "-Wnon-virtual-dtor",
] + select({
    "@bazel_tools//src/conditions:darwin": ["-Wunused-const-variable"],
    "//conditions:default": ["-Wunused-const-variable=1"],
})
# DEBUG_FLAGS = ["-O0", "-g"]
DEBUG_FLAGS = ["-O0", "-g"]
RELEASE_FLAGS = ["-O2"]
FAST_FLAGS = ["-O1"]

def _service_copts():
    return select({
        "@service//bazel:service_build_as_release": RELEASE_FLAGS,
        "@service//bazel:service_build_as_debug": DEBUG_FLAGS,
        "@service//bazel:service_build_as_fast": FAST_FLAGS,
        "//conditions:default": FAST_FLAGS,
    }) + WARNING_FLAGS + INCLUDE_FALGS


def service_cc_binary(
        linkopts = [],
        copts = [],
        deps = [],
        **kargs):
    cc_binary(
        linkopts = linkopts,
        copts = copts +_service_copts(),
        deps = deps,
        **kargs
    )

def service_cc_library(
        linkopts = [],
        copts = [],
        deps = [],
        **kargs):
    cc_library(
        linkopts = linkopts,
        copts = copts+_service_copts(),
        deps = deps,
        **kargs
    )

def service_cc_test(
        linkopts = [],
        copts = [],
        deps = [],
        linkstatic = True,
        **kwargs):
    cc_test(
        # -lm for tcmalloc
        linkopts = linkopts + ["-lm"],
        copts = _service_copts() + copts,
        deps = deps + [
            # use tcmalloc same as release bins. make them has same behavior on mem.
            # "@com_github_gperftools_gperftools//:gperftools",
            "@com_google_googletest//:gtest_main",
        ],
        # static link for tcmalloc
        linkstatic = True,
        **kwargs
    )