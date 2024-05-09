#pragma once

#include <functional>
#include <list>
#include <queue>
#include <sys/ucontext.h>
#include <ucontext.h>
#include <vector>
#include "status.h"

class Poller;
class Future;
struct FutureCompare {
    bool operator()(const Future* left, const Future* right) const;
};


class Future {
public:
    // 默认优先级都是1 ，即最小级别。
    Future(std::function<void()> run, Poller *xfiber, size_t _priority = 1);

    ~Future();

    // 每一个Future的启动函数，调用上面构造函数传入的闭包
    static void Start(Future *fiber);

    // 返回当前Future的上下文
    ucontext_t *Ctx();

    // 判断当前Future是否完成，如果完成返回1, 否则返回0
    bool IsFinished();

    // 有两种状态， OK 和 NOT_OK 两种状态，OK为完成， NOT_OK为未完成
    STATUS status;

    // 当前Future的上下文
    ucontext_t ctx;

    // 当前Future相关联的函数，这里用闭包传递
    std::function<void()> my_task;

    // 当前Future的栈指针
    char *stack;

    // 当前Future的栈大小
    size_t stack_size;

    // 当前Future进入Poller的位次，主要是处理当优先级一致时Future的调度顺序问题
    size_t sequence;

    // 当前Future的优先级
    size_t priority;
};

class Poller {
public:
    Poller();

    ~Poller();

    // 注册Future到Poller中
    void Async(std::function<void()> run, size_t _priority = 1);

    // 关闭定时器
    void CloseTimer();

    // 执行异步调度
    void Await();

    // 没用上，Future自己放弃当前调度时使用，例如在等待Web请求阻塞时可以使用，放弃
    // 当前调度，需要与Linux内核的epoll配套使用。
    void Yiled();

    // 返回调度器Poller的运行时
    ucontext_t *PollerCtx();

    // 处理定时器触发情况
    static void SiganlHandler(int sig) {
        // 将当前运行的Future加入到就绪队列中
        ready_futures.push(Poller::cur_future);
        // 切换上下文，将上下文由当前运行的Future的上下文切换到Poller的上下文，
        // 准备调度下一个Future
        swapcontext(Poller::cur_future->Ctx(), &Poller::poller_ctx);
    }

    // 当前正在运行的Future的上下文
    static Future *cur_future;

    // 调度器Poller自己的上下文
    static ucontext_t poller_ctx;

    // 调度队列，采用优先级队列，优先策略为priority越大优先级越高，
    // 优先级相同的情况下，sequence越小优先级越高
    static std::priority_queue<Future* , std::vector<Future*>, FutureCompare> ready_futures, running_futures;

    // 是否使用计时器，默认情况下为true，即使用计时器
    bool use_timer;
};