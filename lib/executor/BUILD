cc_library(
    name = "executor",
    hdrs = [
        "executor.h",
    ],
    srcs = [
        "executor.cpp",
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
        "//lib/arbitrage/triangular:triangular_arbitrage",
        "//utils:utils",
    ],
    visibility = ["//visibility:public"],
)