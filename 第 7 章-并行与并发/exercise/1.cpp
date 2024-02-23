#include <iostream>
#include <vector>
#include <queue>
#include <thread>
#include <functional>
#include <future>
#include <mutex>
#include <condition_variable>

// 在这个线程池的实现中，调用 enqueue 方法将任务添加到线程池的任务队列中，但并不会立即执行。任务会等待线程池中的工作线程来执行。具体来说：
// 1. 当调用 enqueue 方法时，任务会被包装成 std::packaged_task 并放入任务队列中。
// 2. 工作线程会从任务队列中取出任务并执行它们。
// 3. 任务的执行是异步的，由线程池中的工作线程来完成。
// 4. 调用 f.get() 会阻塞当前线程，直到任务执行完成并返回结果。
// 因此，虽然调用 enqueue 方法会将任务添加到线程池中，但任务的执行是由线程池中的工作线程异步完成的。


class ThreadPool
{
public:
  ThreadPool(size_t numThreads) : stop(false)
  {
    for (size_t i = 0; i < numThreads; ++i)
    {
      workers.emplace_back([this]
                           {
                while (true) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(queueMutex);
                        condition.wait(lock, [this] { return stop || !tasks.empty(); });
                        if (stop && tasks.empty())
                            return;
                        task = std::move(tasks.front());
                        tasks.pop();
                    }
                    task();
                } });
    }
  }

  template <class F, class... Args>
  auto enqueue(F &&f, Args &&...args) -> std::future<std::invoke_result_t<F, Args...>>
  {
    using return_type = std::invoke_result_t<F, Args...>;
    auto task = std::make_shared<std::packaged_task<return_type()>>(std::bind(std::forward<F>(f), std::forward<Args>(args)...));
    std::future<return_type> res = task->get_future();
    {
      std::unique_lock<std::mutex> lock(queueMutex);
      if (stop)
        throw std::runtime_error("enqueue on stopped ThreadPool");
      tasks.emplace([task]()
                    { (*task)(); });
    }
    condition.notify_one();
    return res;
  }

  ~ThreadPool()
  {
    {
      std::unique_lock<std::mutex> lock(queueMutex);
      stop = true;
    }
    condition.notify_all();
    for (std::thread &worker : workers)
      worker.join();
  }

private:
  std::vector<std::thread> workers;
  std::queue<std::function<void()>> tasks;
  std::mutex queueMutex;
  std::condition_variable condition;
  bool stop;
};

int main()
{
  ThreadPool pool(4);

  auto f = pool.enqueue([](int life)
                        { return life; },
                        42);

  std::cout << f.get() << std::endl;

  return 0;
}