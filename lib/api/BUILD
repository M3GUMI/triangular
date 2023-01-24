cc_library(
    name = "binance_test",
    hdrs = [
        "binance.h",
    ],
    srcs = [
        "binance.cpp",
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
        "//lib/api/ws:ws",
        "//lib/api/http:http",
    ],
    visibility = ["//visibility:public"],
)