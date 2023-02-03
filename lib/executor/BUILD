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
        "//lib/arbitrage/triangular:triangular_arbitrage",
        "//lib/pathfinder:pathfinder",
        "//lib/capital_pool:capital_pool",
        "//utils:utils",
        "//lib/api:api",
    ],
    visibility = ["//visibility:public"],
)