cc_library(
    name = "api",
    hdrs = [
        "api.h",
    ],
    srcs = [
        "api.cpp",
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