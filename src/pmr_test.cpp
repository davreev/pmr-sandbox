#include <cstdio>

#include <chrono>
#include <charconv>
#include <memory_resource>
#include <vector>
#include <string>
#include <unordered_map>

namespace pmr = std::pmr;

namespace
{

struct DebugMemoryResource : public pmr::memory_resource
{
    pmr::memory_resource* upstream;
    std::size_t num_allocs{};
    std::size_t num_deallocs{};
    std::size_t curr_bytes{};
    std::size_t max_bytes{};

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

void vector_test_1(pmr::memory_resource* memory)
{
    constexpr int n = 100000;

    if (memory != nullptr)
    {
        pmr::vector<int> vec{memory};

        for (int i = 0; i < n; ++i)
            vec.push_back(i);
    }
    else
    {
        std::vector<int> vec{};

        for (int i = 0; i < n; ++i)
            vec.push_back(i);
    }
}

void vector_test_2(pmr::memory_resource* memory)
{
    constexpr int n = 1000;
    constexpr int m = 100;

    if (memory != nullptr)
    {
        pmr::vector<pmr::vector<int>> vecs{memory};

        for (int i = 0; i < n; ++i)
        {
            pmr::vector<int> vec{memory};

            for (int j = 0; j < m; ++j)
                vec.push_back(j);

            vecs.push_back(std::move(vec));
        }
    }
    else
    {
        std::vector<std::vector<int>> vecs{};

        for (int i = 0; i < n; ++i)
        {
            std::vector<int> vec{};

            for (int j = 0; j < m; ++j)
                vec.push_back(j);

            vecs.push_back(std::move(vec));
        }
    }
}

void unordered_map_test_1(pmr::memory_resource* memory)
{
    constexpr int n = 10000;
    char buf[64]{};

    if (memory != nullptr)
    {
        pmr::unordered_map<pmr::string, int> map{memory};

        for (int i = 0; i < n; ++i)
        {
            std::to_chars(buf, buf + std::size(buf), i);
            map[buf] = i;
        }
    }
    else
    {
        std::unordered_map<std::string, int> map{};

        for (int i = 0; i < n; ++i)
        {
            std::to_chars(buf, buf + std::size(buf), i);
            map[buf] = i;
        }
    }
}

void unordered_map_test_2(pmr::memory_resource* memory)
{
    constexpr int n = 100;
    constexpr int m = 100;
    char buf[64]{};

    if (memory != nullptr)
    {
        pmr::unordered_map<pmr::string, pmr::unordered_map<pmr::string, int>> maps{memory};

        for (int i = 0; i < n; ++i)
        {
            pmr::unordered_map<pmr::string, int> map{memory};

            for (int j = 0; j < m; ++j)
            {
                std::to_chars(buf, buf + std::size(buf), j);
                map[buf] = j;
            }

            std::to_chars(buf, buf + std::size(buf), i);
            maps[buf] = std::move(map);
        }
    }
    else
    {
        std::unordered_map<std::string, std::unordered_map<std::string, int>> maps{};

        for (int i = 0; i < n; ++i)
        {
            std::unordered_map<std::string, int> map{};

            for (int j = 0; j < m; ++j)
            {
                std::to_chars(buf, buf + std::size(buf), j);
                map[buf] = j;
            }

            std::to_chars(buf, buf + std::size(buf), i);
            maps[buf] = std::move(map);
        }
    }
}

void do_tests(pmr::memory_resource* memory)
{
    auto do_test = [=](void (*test)(pmr::memory_resource*), const char* context) {
        using Clock = std::chrono::high_resolution_clock;
        using Duration = std::chrono::milliseconds;

        const auto start = Clock::now();
        constexpr int n = 10;

        for (int i = 0; i < n; ++i)
            test(memory);

        const auto elapsed = std::chrono::duration_cast<Duration>(Clock::now() - start);
        std::printf("%s (%lld ms)\n", context, static_cast<long long>(elapsed.count()));
    };

    do_test(vector_test_1, "vector test 1");
    do_test(vector_test_2, "vector test 2");
    do_test(unordered_map_test_1, "unordered map test 1");
    do_test(unordered_map_test_2, "unordered map test 2");
}

void report(DebugMemoryResource* memory)
{
    if (memory != nullptr)
    {
        std::printf("num allocs: %lld\n", memory->num_allocs);
        std::printf("num deallocs: %lld\n", memory->num_deallocs);
        std::printf("max bytes: %lld\n", memory->max_bytes);
    }
    else
    {
        std::printf("num allocs: ?\n");
        std::printf("num deallocs: ?\n");
        std::printf("max bytes: ?\n");
    }

    std::printf("\n");
}

void no_resource_test()
{
    std::printf("no resource\n---\n");
    do_tests(nullptr);
    report(nullptr);
}

void default_resource_test()
{
    std::printf("default resource\n---\n");
    DebugMemoryResource db_mem{pmr::new_delete_resource()};
    do_tests(&db_mem);
    report(&db_mem);
}

void buffer_resource_test()
{
    std::printf("buffer resource\n---\n");
    DebugMemoryResource db_mem{pmr::new_delete_resource()};

    {
        pmr::monotonic_buffer_resource buf_mem{&db_mem};
        do_tests(&buf_mem);
    }

    report(&db_mem);
}

void pool_resource_test()
{
    std::printf("pool resource\n---\n");
    DebugMemoryResource db_mem{pmr::new_delete_resource()};

    {
        pmr::unsynchronized_pool_resource pool_mem{&db_mem};
        do_tests(&pool_mem);
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
        do_tests(&buf_mem);
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
        do_tests(&pool_mem);
    }

    report(&db_mem);
}

} // namespace

int main()
{
    no_resource_test();

    // These use polymorphic memory resources (std::pmr)
    {
        default_resource_test();
        pool_resource_test();
        buffer_resource_test();
        buffer_backed_pool_resource_test();
        pool_backed_buffer_resource_test();
    }

    return 0;
}
