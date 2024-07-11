cmake_minimum_required(VERSION 3.19)
project(
    pmr-sandbox
    VERSION 0.1.0
    LANGUAGES CXX
)

#
# Global config
#

set(CMAKE_CXX_STANDARD 17)

list(
    PREPEND CMAKE_MODULE_PATH
    "${CMAKE_CURRENT_LIST_DIR}/cmake/"
)

# Show FetchContent download progress
set(FETCHCONTENT_QUIET FALSE)

#
# Targets
#

include("deps/eigen")
include("deps/fmt")

add_library(common INTERFACE)
target_link_libraries(
    common
    INTERFACE
        fmt::fmt
)
target_compile_options(
    common
    INTERFACE
        -Wall -Wextra -Wpedantic -Werror
)

add_executable(
    pmr-test
    "src/pmr_test.cpp"
)
target_link_libraries(
    pmr-test
    PRIVATE
        common
)

add_executable(
    pmr-eigen-test
    "src/pmr_eigen_test.cpp"
    "src/eigen_memory_resource.cpp"
)
target_link_libraries(
    pmr-eigen-test
    PRIVATE
        common
        Eigen3::Eigen
)