cc_library(
    name = "capital_pool",
    hdrs = [
        "capital_pool.h",
    ],
    srcs = [
        "capital_pool.cpp",
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
        "//lib/api/http:http",
        "//utils:utils",
    ],
    visibility = ["//visibility:public"],
)