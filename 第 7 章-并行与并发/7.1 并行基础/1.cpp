#include <iostream>
#include <thread>

void threadFunction(int i) {
    std::cout << "hello world. thread " << i << std::endl;
}

int main() {
    int num = 10;
    std::thread threads[num];

    for (int i = 0; i < num; ++i) {
        threads[i] = std::thread(threadFunction, i);
    }

    for (int i = 0; i < num; ++i) {
        threads[i].join();
    }

    return 0;
}