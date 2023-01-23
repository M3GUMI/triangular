cc_library(
    name = "defined",
    hdrs = [
        "defined.h",
    ],
    srcs = [
        "defined.cpp",
    ],
    includes = ["."],
    linkopts = [
        "-lboost_system",
        "-lpthread",
        "--std=c++11",
        "-lssl",
        "-lcrypto",
        "-DASIO_STANDALONE"
    ],
    deps = [
    ],
    visibility = ["//visibility:public"],
)