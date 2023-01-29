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
        "//lib/api:api",
    ],
)