cc_library(
    name = "triangular_arbitrage",
    hdrs = [
        "triangular.h",
    ],
    srcs = [
        "triangular.cpp",
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
        "//lib/pathfinder:pathfinder",
        "//defined"
    ],
    visibility = ["//visibility:public"],
)