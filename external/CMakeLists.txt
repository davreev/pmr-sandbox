################################################################################
# Eigen

add_library(eigen INTERFACE)
add_library (Eigen3::Eigen ALIAS eigen)

target_include_directories(
    eigen 
    INTERFACE 
        "${CMAKE_CURRENT_LIST_DIR}/eigen"
)