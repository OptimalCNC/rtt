#include <cstdlib>
#include <cstring>
#include <iostream>

#include <rtt/os/fosi_internal_interface.hpp>

#if CONFIG_XENO_VERSION_MAJOR >= 3
namespace {
int join_calls = 0;
int delete_calls = 0;
RT_TASK* joined_task = 0;
RT_TASK* deleted_task = 0;
}

extern "C" int rt_task_join(RT_TASK* task)
{
    ++join_calls;
    joined_task = task;
    return 0;
}

extern "C" int rt_task_delete(RT_TASK* task)
{
    ++delete_calls;
    deleted_task = task;
    return 0;
}
#endif

int main()
{
#if CONFIG_XENO_VERSION_MAJOR < 3
    return 0;
#else
    RTOS_TASK task = {};
    const char task_name[] = "joined-xenomai3-task";
    task.name = static_cast<char*>(std::malloc(sizeof(task_name)));
    if (!task.name) {
        std::cerr << "failed to allocate task name" << std::endl;
        return 1;
    }
    std::memcpy(task.name, task_name, sizeof(task_name));

    RTT::os::rtos_task_delete(&task);

    if (join_calls != 1 || joined_task != &task.xenotask) {
        std::cerr << "expected exactly one join of the task descriptor" << std::endl;
        return 1;
    }
    if (delete_calls != 0) {
        std::cerr << "rt_task_delete was called after successful rt_task_join"
                  << std::endl;
        return 1;
    }
    if (deleted_task) {
        std::cerr << "unexpected task descriptor passed to rt_task_delete"
                  << std::endl;
        return 1;
    }
    if (task.name) {
        std::cerr << "RTT task metadata was not released" << std::endl;
        return 1;
    }
    return 0;
#endif
}
