1. 请编写一个简单的线程池，提供如下功能：
```C++
ThreadPool p(4); // 指定四个工作线程

// 将任务在池中入队，并返回一个 std::future
auto f = pool.enqueue([](int life) {
    return meaning;
}, 42);

// 从 future 中获得执行结果
std::cout << f.get() << std::endl;
```

2. 请使用 std::atomic<bool> 实现一个互斥锁。

