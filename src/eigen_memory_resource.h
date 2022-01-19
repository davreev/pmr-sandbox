#include <memory_resource>

namespace dr
{

std::pmr::memory_resource* get_eigen_memory_resource();
void set_eigen_memory_resource(std::pmr::memory_resource* memory);

} // namespace dr

#define EIGEN_MEMORY_RESOURCE dr::get_eigen_memory_resource()
#define EIGEN_NO_MALLOC // Aborts if Eigen tries to allocate without our resource