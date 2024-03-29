细心的读者可能会对前一小节中生产者消费者模型的例子可能存在编译器优化导致程序出错的情况产生疑惑。 例如，布尔值 notified 没有被 volatile 修饰，编译器可能对此变量存在优化，例如将其作为一个寄存器的值， 从而导致消费者线程永远无法观察到此值的变化。这是一个好问题，为了解释清楚这个问题，我们需要进一步讨论 从 C++ 11 起引入的内存模型这一概念。我们首先来看一个问题，下面这段代码输出结果是多少？

```C++
#include <thread>
#include <iostream>

int main() {
    int a = 0;
    int flag = 0;

    std::thread t1([&]() {
        while (flag != 1);

        int b = a;
        std::cout << "b = " << b << std::endl;
    });

    std::thread t2([&]() {
        a = 5;
        flag = 1;
    });

    t1.join();
    t2.join();
    return 0;
}
```

从直观上看，t2 中 a = 5; 这一条语句似乎总在 flag = 1; 之前得到执行，而 t1 中 while (flag != 1) 似乎保证了 std::cout << "b = " << b << std::endl; 不会再标记被改变前执行。从逻辑上看，似乎 b 的值应该等于 5。 但实际情况远比此复杂得多，或者说这段代码本身属于未定义的行为，因为对于 a 和 flag 而言，他们在两个并行的线程中被读写， 出现了竞争。除此之外，即便我们忽略竞争读写，仍然可能受 CPU 的乱序执行，编译器对指令的重排的影响， 导致 a = 5 发生在 flag = 1 之后。从而 b 可能输出 0。

# 原子操作
std::mutex 可以解决上面出现的并发读写的问题，但互斥锁是操作系统级的功能， 这是因为一个互斥锁的实现通常包含两条基本原理：

1. 提供线程间自动的状态转换，即『锁住』这个状态
2. 保障在互斥锁操作期间，所操作变量的内存与临界区外进行隔离

这是一组非常强的同步条件，换句话说当最终编译为 CPU 指令时会表现为非常多的指令（我们之后再来看如何实现一个简单的互斥锁）。 这对于一个仅需原子级操作（没有中间态）的变量，似乎太苛刻了。

关于同步条件的研究有着非常久远的历史，我们在这里不进行赘述。读者应该明白，现代 CPU 体系结构提供了 CPU 指令级的原子操作， 因此在 C++11 中多线程下共享变量的读写这一问题上，还引入了 std::atomic 模板，使得我们实例化一个原子类型，将一个 原子类型读写操作从一组指令，最小化到单个 CPU 指令。例如：

```C++
std::atomic<int> counter;
```

并为整数或浮点数的原子类型提供了基本的数值成员函数，举例来说， 包括 fetch_add, fetch_sub 等，同时通过重载方便的提供了对应的 +，- 版本。 比如下面的例子：
```C++
#include <atomic>
#include <thread>
#include <iostream>

std::atomic<int> count = {0};

int main() {
    std::thread t1([](){
        count.fetch_add(1);
    });
    std::thread t2([](){
        count++;        // 等价于 fetch_add
        count += 1;     // 等价于 fetch_add
    });
    t1.join();
    t2.join();
    std::cout << count << std::endl;
    return 0;
}
```
当然，并非所有的类型都能提供原子操作，这是因为原子操作的可行性取决于具体的 CPU 架构，以及所实例化的类型结构是否能够满足该 CPU 架构对内存对齐 条件的要求，因而我们总是可以通过 std::atomic<T>::is_lock_free 来检查该原子类型是否需支持原子操作，例如：
```C++
#include <atomic>
#include <iostream>

struct A {
    float x;
    int y;
    long long z;
};

int main() {
    std::atomic<A> a;
    std::cout << std::boolalpha << a.is_lock_free() << std::endl;
    return 0;
}
```

# 一致性模型
并行执行的多个线程，从某种宏观层面上讨论，可以粗略的视为一种分布式系统。 在分布式系统中，任何通信乃至本地操作都需要消耗一定时间，甚至出现不可靠的通信。

如果我们强行将一个变量 v 在多个线程之间的操作设为原子操作，即任何一个线程在操作完 v 后， 其他线程均能同步感知到 v 的变化，则对于变量 v 而言，表现为顺序执行的程序，它并没有由于引入多线程 而得到任何效率上的收益。对此有什么办法能够适当的加速呢？答案便是削弱原子操作的在进程间的同步条件。

从原理上看，每个线程可以对应为一个集群节点，而线程间的通信也几乎等价于集群节点间的通信。 削弱进程间的同步条件，通常我们会考虑四种不同的一致性模型：

1. 线性一致性：又称强一致性或原子一致性。它要求任何一次读操作都能读到某个数据的最近一次写的数据，并且所有线程的操作顺序与全局时钟下的顺序是一致的。
```
        x.store(1)      x.load()
T1 ---------+----------------+------>


T2 -------------------+------------->
                x.store(2)
```
在这种情况下线程 T1, T2 对 x 的两次写操作是原子的，且 x.store(1) 是严格的发生在 x.store(2) 之前，x.store(2) 严格的发生在 x.load() 之前。 值得一提的是，线性一致性对全局时钟的要求是难以实现的，这也是人们不断研究比这个一致性更弱条件下其他一致性的算法的原因。


2. 顺序一致性：同样要求任何一次读操作都能读到数据最近一次写入的数据，但未要求与全局时钟的顺序一致。
```
        x.store(1)  x.store(3)   x.load()
T1 ---------+-----------+----------+----->


T2 ---------------+---------------------->
              x.store(2)

或者

        x.store(1)  x.store(3)   x.load()
T1 ---------+-----------+----------+----->


T2 ------+------------------------------->
      x.store(2)
```

3. 因果一致性：它的要求进一步降低，只需要有因果关系的操作顺序得到保障，而非因果关系的操作顺序则不做要求。
```
      a = 1      b = 2
T1 ----+-----------+---------------------------->


T2 ------+--------------------+--------+-------->
      x.store(3)         c = a + b    y.load()

或者

      a = 1      b = 2
T1 ----+-----------+---------------------------->


T2 ------+--------------------+--------+-------->
      x.store(3)          y.load()   c = a + b

亦或者

     b = 2       a = 1
T1 ----+-----------+---------------------------->


T2 ------+--------------------+--------+-------->
      y.load()            c = a + b  x.store(3)
```
上面给出的三种例子都是属于因果一致的，因为整个过程中，只有 c 对 a 和 b 产生依赖，而 x 和 y 在此例子中表现为没有关系（但实际情况中我们需要更详细的信息才能确定 x 与 y 确实无关）

4. 最终一致性：是最弱的一致性要求，它只保障某个操作在未来的某个时间节点上会被观察到，但并未要求被观察到的时间。因此我们甚至可以对此条件稍作加强，例如规定某个操作被观察到的时间总是有界的。当然这已经不在我们的讨论范围之内了。
```
    x.store(3)  x.store(4)
T1 ----+-----------+-------------------------------------------->


T2 ---------+------------+--------------------+--------+-------->
         x.read      x.read()           x.read()   x.read()
```
在上面的情况中，如果我们假设 x 的初始值为 0，则 T2 中四次 x.read() 结果可能但不限于以下情况：
```
3 4 4 4 // x 的写操作被很快观察到
0 3 3 4 // x 的写操作被观察到的时间存在一定延迟
0 0 0 4 // 最后一次读操作读到了 x 的最终值，但此前的变化并未观察到
0 0 0 0 // 在当前时间段内 x 的写操作均未被观察到，
        // 但未来某个时间点上一定能观察到 x 为 4 的情况
```

# 内存顺序
为了追求极致的性能，实现各种强度要求的一致性，C++11 为原子操作定义了六种不同的内存顺序 std::memory_order 的选项，表达了四种多线程间的同步模型：

1. 宽松模型：在此模型下，单个线程内的原子操作都是顺序执行的，不允许指令重排，但不同线程间原子操作的顺序是任意的。类型通过 std::memory_order_relaxed 指定。我们来看一个例子：
```C++
std::atomic<int> counter = {0};
std::vector<std::thread> vt;
for (int i = 0; i < 100; ++i) {
    vt.emplace_back([&](){
        counter.fetch_add(1, std::memory_order_relaxed);
    });
}

for (auto& t : vt) {
    t.join();
}
std::cout << "current counter:" << counter << std::endl;
```

2. 释放/消费模型：在此模型中，我们开始限制进程间的操作顺序，如果某个线程需要修改某个值，但另一个线程会对该值的某次操作产生依赖，即后者依赖前者。具体而言，线程 A 完成了三次对 x 的写操作，线程 B 仅依赖其中第三次 x 的写操作，与 x 的前两次写行为无关，则当 A 主动 x.release() 时候（即使用 std::memory_order_release），选项 std::memory_order_consume 能够确保 B 在调用 x.load() 时候观察到 A 中第三次对 x 的写操作。我们来看一个例子：
```C++
// 初始化为 nullptr 防止 consumer 线程从野指针进行读取
std::atomic<int*> ptr(nullptr);
int v;
std::thread producer([&]() {
    int* p = new int(42);
    v = 1024;
    ptr.store(p, std::memory_order_release);
});
std::thread consumer([&]() {
    int* p;
    while(!(p = ptr.load(std::memory_order_consume)));

    std::cout << "p: " << *p << std::endl;
    std::cout << "v: " << v << std::endl;
});
producer.join();
consumer.join();
```

3. 释放/获取模型：在此模型下，我们可以进一步加紧对不同线程间原子操作的顺序的限制，在释放 std::memory_order_release 和获取 std::memory_order_acquire 之间规定时序，即发生在释放（release）操作之前的所有写操作，对其他线程的任何获取（acquire）操作都是可见的，亦即发生顺序（happens-before）。

可以看到，std::memory_order_release 确保了它之前的写操作不会发生在释放操作之后，是一个向后的屏障（backward），而 std::memory_order_acquire 确保了它之前的写行为不会发生在该获取操作之后，是一个向前的屏障（forward）。对于选项 std::memory_order_acq_rel 而言，则结合了这两者的特点，唯一确定了一个内存屏障，使得当前线程对内存的读写不会被重排并越过此操作的前后：

我们来看一个例子：
```C++
std::vector<int> v;
std::atomic<int> flag = {0};
std::thread release([&]() {
    v.push_back(42);
    flag.store(1, std::memory_order_release);
});
std::thread acqrel([&]() {
    int expected = 1; // must before compare_exchange_strong
    while(!flag.compare_exchange_strong(expected, 2, std::memory_order_acq_rel))
        expected = 1; // must after compare_exchange_strong
    // flag has changed to 2
});
std::thread acquire([&]() {
    while(flag.load(std::memory_order_acquire) < 2);

    std::cout << v.at(0) << std::endl; // must be 42
});
release.join();
acqrel.join();
acquire.join();
```
在此例中我们使用了 compare_exchange_strong 比较交换原语（Compare-and-swap primitive），它有一个更弱的版本，即 compare_exchange_weak，它允许即便交换成功，也仍然返回 false 失败。其原因是因为在某些平台上虚假故障导致的，具体而言，当 CPU 进行上下文切换时，另一线程加载同一地址产生的不一致。除此之外，compare_exchange_strong 的性能可能稍差于 compare_exchange_weak，但大部分情况下，鉴于其使用的复杂度而言，compare_exchange_weak 应该被有限考虑。

4. 顺序一致模型：在此模型下，原子操作满足顺序一致性，进而可能对性能产生损耗。可显式的通过 std::memory_order_seq_cst 进行指定。最后来看一个例子：
```C++
std::atomic<int> counter = {0};
std::vector<std::thread> vt;
for (int i = 0; i < 100; ++i) {
    vt.emplace_back([&](){
        counter.fetch_add(1, std::memory_order_seq_cst);
    });
}

for (auto& t : vt) {
    t.join();
}
std::cout << "current counter:" << counter << std::endl;
```
这个例子与第一个宽松模型的例子本质上没有区别，仅仅只是将原子操作的内存顺序修改为了 memory_order_seq_cst，有兴趣的读者可以自行编写程序测量这两种不同内存顺序导致的性能差异。

