#include "eigen_memory_resource.hpp"

namespace dr
{
namespace
{

struct
{
    std::pmr::memory_resource* memory{std::pmr::get_default_resource()};
} state;

} // namespace

std::pmr::memory_resource* get_eigen_memory_resource() { return state.memory; }
void set_eigen_memory_resource(std::pmr::memory_resource* memory) { state.memory = memory; }

} // namespace dr