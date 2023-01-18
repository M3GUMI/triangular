cc_library(
    name = "libmix",
    hdrs = glob(["*.hpp", "*.h"]),
    includes = ["."],
    linkopts = [
        "-lboost_system",
        "-lpthread",
        "--std=c++11",
        "-lssl",
        "-lcrypto",
    ],
    deps = [
    ],
    visibility = ["//visibility:public"],
)