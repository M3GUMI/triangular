cc_library(
    name = "binancewebsocketwrapper",
    hdrs = [
        "binancewebsocketwrapper.h",
    ],
    srcs = [
        "binancewebsocketwrapper.cpp",
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

cc_library(
    name = "binancerestapiwrapper",
    hdrs = [
        "binancerestapiwrapper.h",
        "binancecommondef.h"
    ],
    srcs = [
        "binancerestapiwrapper.cpp",
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
        ":binancewebsocketwrapper",
        "//lib/ahttp:ahttp",
        "//lib/libmix:libmix",
    ],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "WebsocketClient",
    hdrs = [
        "WebsocketClient.h",
    ],
    srcs = [
        "WebsocketClient.cpp",
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
