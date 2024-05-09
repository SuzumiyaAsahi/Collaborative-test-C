#include "poller.h"
#include <queue>
#include <csignal>
#include <functional>
#include <sys/time.h>
#include <sys/ucontext.h>
#include <ucontext.h>

// 用来记录进入Poller的Future的次序
static size_t _sequence = 1;

Poller::Poller() {
    cur_future = nullptr;
    use_timer = true;
}

Poller::~Poller() {}

void Poller::CloseTimer() {
    use_timer = false;
}

Future *Poller::cur_future = nullptr;

ucontext_t Poller::poller_ctx;

std::priority_queue<Future *, std::vector<Future *>, FutureCompare> Poller::ready_futures, Poller::running_futures;

void Poller::Async(std::function<void()> run, size_t _priority) {
    // 登记Future到Poller中，加入到准备队列中
    Future *fiber = new Future(run, this, _priority);
    ready_futures.push(fiber);
}

// 返回Poller的上下文
ucontext_t *Poller::PollerCtx() { return &poller_ctx; }

void Poller::Await() {
    if (use_timer) {
        // 设计计时器，第一次触发时间是1s，之后间隔1s触发一次，
        // 执行SignalHandler函数
        struct itimerval timer;
        timer.it_value.tv_sec = 1;
        timer.it_value.tv_usec = 0;
        timer.it_interval.tv_sec = 1;
        timer.it_interval.tv_usec = 0;
        signal(SIGALRM, Poller::SiganlHandler);
        setitimer(ITIMER_REAL, &timer, NULL);
    }
    while (true) {
        if (ready_futures.size() == 0) {
            continue;
        }

        // running_futures 从ready_futures中获得要调度的Futures
        running_futures = std::move(ready_futures);
        // 清空ready_futures
        ready_futures = std::priority_queue<Future *, std::vector<Future *>, FutureCompare>();

        std::priority_queue<Future *, std::vector<Future *>, FutureCompare> copy_running_futures = running_futures;
        // 依次调度running_futures,这里其实不复制也行，但是当时考虑到以后说不定有扩展
        // 就这样放这里的
        while (!copy_running_futures.empty()) {
            Future *future = copy_running_futures.top();
            copy_running_futures.pop();
            cur_future = future;

            //切换上下文，这里程序执行流就跳转到Future的上下文了，
            swapcontext(&poller_ctx, future->Ctx());

            // 待返回后检查Future是否执行完，如果执行完就删除
            cur_future = nullptr;
            if (future->IsFinished()) {
                delete future;
            }
        }
        // 清空running_futures准备进行下一波调度
        running_futures = std::priority_queue<Future *, std::vector<Future *>, FutureCompare>();
    }
}

// 没用上
void Poller::Yiled() {
    ready_futures.push(cur_future);
    swapcontext(cur_future->Ctx(), &poller_ctx);
}


Future::Future(std::function<void()> run, Poller *xfiber, size_t _priority) {
    // 设置优先级
    priority = _priority;
    // 设置顺次
    sequence = _sequence++;
    // 设置要执行的闭包函数
    my_task = run;
    // 设置当前的状态
    status = NOT_OK;
    // 获取上下文
    getcontext(&ctx);
    // 设置栈空间为128KB
    stack_size = 1024 * 128;
    stack = new char[stack_size];
    //设置当Future的任务执行之后返回的上下文， 设置为调度器Poller的上下文中，方便下一步的调度
    ctx.uc_link = xfiber->PollerCtx();
    ctx.uc_stack.ss_sp = stack;
    ctx.uc_stack.ss_size = stack_size;
    // 制作上下文，获得正式的上下文ctx
    makecontext(&ctx, (void (*)()) Future::Start, 1, this);
}

Future::~Future() {
    //删除栈空间
    delete stack;
    stack = nullptr;
    stack_size = 0;
}

void Future::Start(Future *fiber) {
    // 执行闭包函数
    fiber->my_task();
    // 闭包执行完了就可以设置Future状态为OK了
    fiber->status = OK;
}

bool Future::IsFinished() { return status == OK; }

ucontext_t *Future::Ctx() { return &ctx; }
