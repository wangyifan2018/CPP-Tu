std::thread 用于创建一个执行的线程实例，所以它是一切并发编程的基础，使用时需要包含 <thread> 头文件， 它提供了很多基本的线程操作，例如 get_id() 来获取所创建线程的线程 ID，使用 join() 来等待一个线程结束（与该线程汇合）等等，例如：
```C++
#include <iostream>
#include <thread>

int main() {
    std::thread t([](){
        std::cout << "hello world." << std::endl;
    });
    t.join();
    return 0;
}
```
