cmake_minimum_required(VERSION 3.10)

project(MoneyTriangular)

file(GLOB_RECURSE ALL_FILES
        REMOVE_SOURCES "cmake-build*"
        *.cpp *.hpp *.h)
list(FILTER ALL_FILES EXCLUDE REGEX "cmake-build*")

add_executable(MoneyTriangular ${ALL_FILES})

target_include_directories(MoneyTriangular
        PUBLIC
        .)

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

