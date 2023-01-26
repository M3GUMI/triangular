cc_library(
    name = "ws",
    hdrs = [
        "websocket_wrapper.h",
        "binance_order_wrapper.h",
        "binance_depth_wrapper.h",
    ],
    srcs = [
        "websocket_wrapper.cpp",
        "binance_order_wrapper.cpp",
        "binance_depth_wrapper.cpp",
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
        "//utils:utils",
        "//lib/api/http:http",
    ],
    visibility = ["//visibility:public"],
)
