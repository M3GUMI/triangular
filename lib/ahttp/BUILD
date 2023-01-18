cc_library(
    name = "ahttp",
    hdrs = [
    ],
    srcs = [
        "config.hpp",
        "dns_reslove.hpp",
        "http_client.hpp",
        "http_conn.hpp",
        "http_parse.hpp",
        "http_request.hpp",
        "http_respone.hpp",
        "tcp_conn.hpp",
        "url.hpp"
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
    ],
    visibility = ["//visibility:public"],
)