cc_library(
    name = "utils",
    hdrs = [
        "utils.h",
    ],
    srcs = [
        "utils.cpp",
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
        "//define:define",
    ],
    visibility = ["//visibility:public"],
)