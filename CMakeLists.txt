cmake_minimum_required(VERSION 3.19)
project(pmr-sandbox)

set(CMAKE_CXX_STANDARD 17)

# Set global compiler options

if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /bigobj")
endif()

# Create targets

add_subdirectory(external)

add_executable(
    pmr-test
    src/pmr_test.cpp
)

add_executable(
    pmr-eigen-test
    src/pmr_eigen_test.cpp
    src/eigen_memory_resource.cpp
)

target_link_libraries(
    pmr-eigen-test
    PRIVATE
        Eigen3::Eigen
)