cc_library(
    name = "triangular_arbitrage",
    hdrs = [
        "triangular.h",
        "ioc_triangular.h",
        "maker_triangular.h",
    ],
    srcs = [
        "triangular.cpp",
        "ioc_triangular.cpp",
        "maker_triangular.cpp",
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
        "//lib/capital_pool:capital_pool",
        "//lib/api/http:http",
        "//define:define",
        "//utils:utils",
    ],
    visibility = ["//visibility:public"],
)