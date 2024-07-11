if(TARGET Eigen3::Eigen)
    return()
endif()

include(FetchContent)

FetchContent_Declare(
    eigen
    GIT_REPOSITORY https://gitlab.com/davreev/eigen.git
    GIT_TAG 919d2ff6e317e0f2576d20b0e4cf34088edb47e4 # Branch: feature/memory-resource
)

FetchContent_GetProperties(eigen)
if(NOT eigen_POPULATED)
    FetchContent_Populate(eigen)
endif()

add_library(Eigen3_Eigen INTERFACE)
add_library(Eigen3::Eigen ALIAS Eigen3_Eigen)

target_include_directories(
    Eigen3_Eigen
    SYSTEM # Suppresses warnings from third party headers
    INTERFACE
        "${eigen_SOURCE_DIR}"
)