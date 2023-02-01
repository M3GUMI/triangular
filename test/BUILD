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
        "//lib/executor:executor",
        "//lib/capital_pool:capital_pool",
        "//lib/api:api",
    ],
)