cc_library(
    name = "http",
    hdrs = [
        "base_api_wrapper.h",
        "binance_api_wrapper.h",
    ],
    srcs = [
        "base_api_wrapper.cpp",
        "binance_api_wrapper.cpp",
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
        "//lib/ahttp:ahttp",
        "//lib/libmix:libmix",
        "//define:define",
        "//utils:utils",
    ],
    visibility = ["//visibility:public"],
)