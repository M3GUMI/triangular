cc_library(
    name = "define",
    hdrs = [
        "define.h",
        "error.h",
    ],
    srcs = [
        "define.cpp",
        "error.cpp",
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