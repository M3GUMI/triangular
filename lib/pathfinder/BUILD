cc_library(
    name = "pathfinder",
    hdrs = [
        "pathfinder.h",
    ],
    srcs = [
        "pathfinder.cpp",
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
        "//lib/api:api",
    ],
    visibility = ["//visibility:public"],
)