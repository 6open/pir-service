load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")
load("@bazel_tools//tools/build_defs/repo:utils.bzl", "maybe")
load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository")
SECRETFLOW_GIT = "https://github.com/secretflow"

YACL_COMMIT_ID = "b3f50c6dbcd141d8dfd3e1dc6b4d54885fd5ac92"

NV_GIT_ADDRESS = "192.168.50.7"
APSI_COMMIT_ID = "3fcb69720606dc1319dc8d3d29c802453da4f1af"

def service_deps():
    _com_nvgit_apsi()
    _com_nvgit_spu()
    _com_google_tcmalloc()
    _com_google_absl()
 
def _com_nvgit_apsi():
    maybe(
        http_archive,
        name = "nvclouds_apsi",
        type = "tar.gz",
        strip_prefix  = "apsi_api-master",
        #sha256sum apsi_api-master.tar.gz
        sha256 = "86e23c0ce249cf6e72a6c88077f2800e54bdcb1c397dd824788423be16b07cc9",
        build_file = "@service//bazel:apsi.BUILD",
        urls = [
            "http://192.168.50.7/apsi/apsi_api/-/archive/master/apsi_api-master.tar.gz",
        ]
    )
    # maybe(
    #     git_repository,
    #     name = "nvclouds_apsi",
    #     remote = "http://{}/apsi/apsi_api.git".format(NV_GIT_ADDRESS),
    #     build_file = "@service//bazel:apsi.BUILD",
    #     commit = "3fcb69720606dc1319dc8d3d29c802453da4f1af",
    # )

def _com_nvgit_spu():
    # maybe(
    #     git_repository,
    #     name = "spulib",
    #     remote = "http://{}/linjunmian/spu.git".format(NV_GIT_ADDRESS),
    #     # branch = "realmain",
    #     commit = "5503ef1fd6ff392ffebf2ea269eb8a5366da28c4",
    #     # commit = "5503ef1fd6ff392ffebf2ea269eb8a5366da28c4",
    # )
    maybe(
        http_archive,
        name = "spulib",
        type = "tar.gz",
        strip_prefix  = "spu-58sync",
        #sha256sum apsi_api-master.tar.gz
        sha256 = "50c557745dd64e93c9b3153629962789f4c76c67f3b66bf24c3ac91e92bc6214",
        urls = [
            "http://192.168.50.7/linjunmian/spu/-/archive/58sync/spu-58sync.tar.gz",
        ]
    )
    
def _com_google_tcmalloc():
    maybe(
        git_repository,
        name = "com_google_tcmalloc",
        remote = "https://github.com/google/tcmalloc.git",
        commit = "1feaffe42c7e3fbf7e51fb4664ad4247500af94e",
    )

def _com_google_absl():
    maybe(
        http_archive,
        name = "com_google_absl",
        strip_prefix = "abseil-cpp-34e29aae4fe9d296d57268809dfb78a34e705233",
        sha256 = "99de440e97f390cfa3f14d6fee5accaf0cfd6072561f610bf4db680c3abcbee6",
        urls = ["https://github.com/abseil/abseil-cpp/archive/34e29aae4fe9d296d57268809dfb78a34e705233.zip"],
    )