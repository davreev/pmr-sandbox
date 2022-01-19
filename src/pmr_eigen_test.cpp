#include <chrono>
#include <cstdio>
#include <iostream>
#include <vector>

#include "eigen_memory_resource.h" // Must be included before Eigen headers
#include <Eigen/Dense>

namespace pmr = std::pmr;

namespace
{

struct DebugMemoryResource : public pmr::memory_resource
{
    pmr::memory_resource* upstream;
    int num_allocs{};
    int num_deallocs{};
    int max_bytes{};
    int curr_bytes{};

    DebugMemoryResource(pmr::memory_resource* upstream) :
        upstream{upstream} {}

    void* do_allocate(std::size_t bytes, std::size_t alignment) override
    {
        ++num_allocs;
        curr_bytes += bytes;
        if (curr_bytes > max_bytes) max_bytes = curr_bytes;
        return upstream->allocate(bytes, alignment);
    }

    void do_deallocate(void* ptr, std::size_t bytes, std::size_t alignment) override
    {
        ++num_deallocs;
        curr_bytes -= bytes;
        upstream->deallocate(ptr, bytes, alignment);
    }

    bool do_is_equal(const pmr::memory_resource& other) const noexcept override
    {
        return this == &other;
    };
};

void report(DebugMemoryResource* memory)
{
    if (memory != nullptr)
    {
        std::printf("num allocs: %d\n", memory->num_allocs);
        std::printf("num deallocs: %d\n", memory->num_deallocs);
        std::printf("curr bytes: %d\n", memory->curr_bytes);
        std::printf("max bytes: %d\n", memory->max_bytes);
    }
    else
    {
        std::printf("num allocs: ?\n");
        std::printf("num deallocs: ?\n");
        std::printf("curr bytes: ?\n");
        std::printf("max bytes: ?\n");
    }

    std::printf("\n");
}

void dense_assign_test()
{
    constexpr int n = 10;
    Eigen::MatrixXd A{n, n};

    constexpr int iters = 10000;
    for (int i = 0; i < iters; ++i)
    {
        Eigen::MatrixXd B{n, n};
        A = B;
    }
}

void dense_mult_test()
{
    // NOTE(dr): Timings appear to be dominated by matrix multiplication here as
    // it's O(n^3)

    constexpr int n = 10;
    Eigen::MatrixXd A{n, n};

    constexpr int iters = 10000;
    for (int i = 0; i < iters; ++i)
    {
        Eigen::MatrixXd B{n, n};
        A *= B;
    }
}

#if false

void dense_decomp_test()
{
    constexpr int n = 16;
    Eigen::MatrixXd A{n, n};
    A.setRandom();
    // std::cout << A << "\n\n";

    {
        Eigen::BDCSVD<Eigen::MatrixXd> svd{A};
        // std::cout << svd.singularValues() << "\n\n";
    }

    {
        Eigen::JacobiSVD<Eigen::MatrixXd> svd{A};
        // std::cout << svd.singularValues() << "\n\n";
    }

    {
        Eigen::PartialPivLU<Eigen::MatrixXd> lu{A};
        // std::cout << lu.matrixLU() << "\n\n";
    }

    {
        Eigen::HouseholderQR<Eigen::MatrixXd> qr{A};
        // std::cout << qr.matrixQR() << "\n\n";
    }
}

#endif

void do_tests()
{
    auto do_test = [=](void (*test)(), const char* context)
    {
        using Clock = std::chrono::high_resolution_clock;
        using Duration = std::chrono::milliseconds;

        const auto start = Clock::now();
        constexpr int n = 1000;

        for (int i = 0; i < n; ++i)
            test();

        const auto elapsed = std::chrono::duration_cast<Duration>(Clock::now() - start);
        std::printf("%s (%lld ms)\n", context, static_cast<long long>(elapsed.count()));
    };

    do_test(dense_assign_test, "dense assign test");
    do_test(dense_mult_test, "dense mult test");
    // do_test(dense_decomp_test, "dense decomp test");
}

void default_resource_test()
{
    std::printf("default resource\n---\n");
    DebugMemoryResource db_mem{pmr::new_delete_resource()};

    dr::set_eigen_memory_resource(&db_mem);
    do_tests();

    report(&db_mem);
}

void buffer_resource_test()
{
    std::printf("buffer resource\n---\n");
    DebugMemoryResource db_mem{pmr::new_delete_resource()};

    {
        pmr::monotonic_buffer_resource buf_mem{&db_mem};
        dr::set_eigen_memory_resource(&buf_mem);
        do_tests();
    }

    report(&db_mem);
}

void pool_resource_test()
{
    std::printf("pool resource\n---\n");
    DebugMemoryResource db_mem{pmr::new_delete_resource()};

    {
        pmr::unsynchronized_pool_resource pool_mem{&db_mem};
        dr::set_eigen_memory_resource(&pool_mem);
        do_tests();
    }

    report(&db_mem);
}

void pool_backed_buffer_resource_test()
{
    std::printf("pool backed buffer resource\n---\n");
    DebugMemoryResource db_mem{pmr::new_delete_resource()};

    {
        pmr::unsynchronized_pool_resource pool_mem{&db_mem};
        pmr::monotonic_buffer_resource buf_mem{&pool_mem};
        dr::set_eigen_memory_resource(&buf_mem);
        do_tests();
    }

    report(&db_mem);
}

void buffer_backed_pool_resource_test()
{
    std::printf("buffer backed pool resource\n---\n");
    DebugMemoryResource db_mem{pmr::new_delete_resource()};

    {
        pmr::monotonic_buffer_resource buf_mem{&db_mem};
        pmr::unsynchronized_pool_resource pool_mem{&buf_mem};
        dr::set_eigen_memory_resource(&pool_mem);
        do_tests();
    }

    report(&db_mem);
}

} // namespace

int main()
{
    default_resource_test();
    // buffer_resource_test();
    pool_resource_test();
    // pool_backed_buffer_resource_test();
    // buffer_backed_pool_resource_test();

    return 0;
}
