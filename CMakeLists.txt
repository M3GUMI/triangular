cmake_minimum_required(VERSION 3.10)

project(MoneyTriangular)

# todo:
# 1. 设置C++标准
# 2. 设置优化等级
# 3. 检查其余链接库是否需要，如rapidjson, asio等等
# 4. 分模块编译

set(ALL_FILES
        conf/conf.h
        conf/conf.cpp
        conf/strategy.h
        conf/strategy.cpp
        define/define.cpp
        define/define.h
        define/error.cpp
        define/error.h
        lib/ahttp/config.hpp
        lib/ahttp/dns_reslove.hpp
        lib/ahttp/http_client.hpp
        lib/ahttp/http_conn.hpp
        lib/ahttp/http_parse.hpp
        lib/ahttp/http_request.hpp
        lib/ahttp/http_respone.hpp
        lib/ahttp/tcp_conn.hpp
        lib/ahttp/url.hpp
        lib/api/http/base_api_wrapper.cpp
        lib/api/http/base_api_wrapper.h
        lib/api/http/binance_api_wrapper.cpp
        lib/api/http/binance_api_wrapper.h
        lib/api/http/client.h
        lib/api/ws/binance_depth_wrapper.cpp
        lib/api/ws/binance_depth_wrapper.h
        lib/api/ws/binance_order_wrapper.cpp
        lib/api/ws/binance_order_wrapper.h
        lib/api/ws/websocket_wrapper.cpp
        lib/api/ws/websocket_wrapper.h
        lib/arbitrage/triangular/triangular.cpp
        lib/arbitrage/triangular/triangular.h
        lib/arbitrage/triangular/ioc_triangular.cpp
        lib/arbitrage/triangular/ioc_triangular.cpp
        lib/arbitrage/triangular/maker_triangular.cpp
        lib/arbitrage/triangular/maker_triangular.h
        lib/capital_pool/capital_pool.cpp
        lib/capital_pool/capital_pool.h
        lib/executor/executor.cpp
        lib/executor/executor.h
        lib/libmix/AnyValue.h
        lib/libmix/Closure.h
        lib/libmix/ClosureHelper.h
        lib/libmix/Common.h
        lib/libmix/DateTime.h
        lib/libmix/DllCall.h
        lib/libmix/EZType_traits.h
        lib/libmix/libmix.h
        lib/libmix/ScopeGuard.h
        lib/libmix/Strings.h
        lib/libmix/ThreadPool.h
        lib/libmix/Timer.h
        lib/libmix/Tree.hpp
        lib/pathfinder/pathfinder.cpp
        lib/pathfinder/pathfinder.h
        lib/pathfinder/node.cpp
        lib/pathfinder/node.h
        lib/pathfinder/graph.cpp
        lib/pathfinder/graph.h
        main.cpp
        utils/utils.cpp
        utils/utils.h
        lib/order/order.h
        lib/order/order.cpp lib/arbitrage/triangular/new_triangular.cpp lib/arbitrage/triangular/new_triangular.h)

add_executable(MoneyTriangular ${ALL_FILES})

target_include_directories(MoneyTriangular
        PUBLIC
        .)

set(TARGET_LOGGER spdlog)
target_link_libraries(MoneyTriangular LINK_PUBLIC ${TARGET_LOGGER})
add_subdirectory(
        third-party/spdlog
)

find_package(OpenSSL REQUIRED)
if (OPENSSL_FOUND)
    target_include_directories(MoneyTriangular
            PUBLIC
            ${OPENSSL_INCLUDE_DIR})
    target_link_libraries(MoneyTriangular
            PUBLIC
            OpenSSL::SSL
            OpenSSL::Crypto)
    message(STATUS "Found OpenSSL ${OPENSSL_VERSION}")
else ()
    message(STATUS "OpenSSL Not Found")
endif ()

