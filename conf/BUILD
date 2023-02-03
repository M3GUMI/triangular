cc_library(
    name = "conf",
    hdrs = [
        "conf.h",
    ],
    srcs = [
        "conf.cpp",
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