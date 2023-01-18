cc_library(
    name = "triangular",
    hdrs = [
        "triangular.h",
    ],
    srcs = [
        "triangular.cpp",
        "triangularlistenkey.cpp",
        "triangularinit.cpp",
        "triangularbalance.cpp",
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
        "//lib/binance:binancerestapiwrapper",
        "//lib/binance:binancewebsocketwrapper",
    ],
    visibility = ["//visibility:public"],
)