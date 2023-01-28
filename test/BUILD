cc_binary(
    name = "test",
    srcs = [
        "test.cpp"
    ],
    includes = ["."],
    linkopts = [
        "-lboost_system",
        "-lpthread",
        "--std=c++11",
        "-lssl",
        "-lcrypto",
    ],
    deps = [
        "//lib/triangular:triangular",
        "//lib/binance:WebsocketClient",
        "//lib/executor:executor",
        "//lib/api:binance_test",
    ],
)

cc_binary(
    name = "test_http",
    srcs = [
        "test_http.cpp"
    ],
    includes = ["."],
    linkopts = [
        "-lboost_system",
        "-lpthread",
        "--std=c++11",
        "-lssl",
        "-lcrypto",
    ],
    deps = [
        "//lib/executor:executor",
        "//lib/api/http:http",
    ],
)