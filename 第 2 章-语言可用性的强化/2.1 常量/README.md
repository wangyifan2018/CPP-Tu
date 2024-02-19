# nullptr
nullptr 出现的目的是为了替代 NULL。在某种意义上来说，传统 C++ 会把 NULL、0 视为同一种东西，这取决于编译器如何定义 NULL，有些编译器会将 NULL 定义为 ((void*)0)，有些则会直接将其定义为 0。

C++ 不允许直接将 void * 隐式转换到其他类型。但如果编译器尝试把 NULL 定义为 ((void*)0)，那么在下面这句代码中：

```C++
char *ch = NULL;
```
没有了 void * 隐式转换的 C++ 只好将 NULL 定义为 0。而这依然会产生新的问题，将 NULL 定义成 0 将导致 C++ 中重载特性发生混乱。考虑下面这两个 foo 函数：
```C++
void foo(char*);
void foo(int);
```
那么 foo(NULL); 这个语句将会去调用 foo(int)，从而导致代码违反直觉。

为了解决这个问题，C++11 引入了 nullptr 关键字，专门用来区分空指针、0。**而 nullptr 的类型为 nullptr_t，能够隐式的转换为任何指针或成员指针的类型，也能和他们进行相等或者不等的比较。**

你可以尝试使用 clang++ 编译下面的代码：
```C++
#include <iostream>
#include <type_traits>

void foo(char *);
void foo(int);

int main() {
    if (std::is_same<decltype(NULL), decltype(0)>::value)
        std::cout << "NULL == 0" << std::endl;
    if (std::is_same<decltype(NULL), decltype((void*)0)>::value)
        std::cout << "NULL == (void *)0" << std::endl;
    if (std::is_same<decltype(NULL), std::nullptr_t>::value)
        std::cout << "NULL == nullptr" << std::endl;

    foo(0);          // 调用 foo(int)
    // foo(NULL);    // 该行不能通过编译
    foo(nullptr);    // 调用 foo(char*)
    return 0;
}

void foo(char *) {
    std::cout << "foo(char*) is called" << std::endl;
}
void foo(int i) {
    std::cout << "foo(int) is called" << std::endl;
}
```
将输出：
```sh
foo(int) is called
foo(char*) is called
```
从输出中我们可以看出，NULL 不同于 0 与 nullptr。所以，请养成直接使用 nullptr的习惯。

此外，在上面的代码中，我们使用了 decltype 和 std::is_same 这两个属于现代 C++ 的语法，简单来说，decltype 用于类型推导，而 std::is_same 用于比较两个类型是否相同，我们会在后面 decltype 一节中详细讨论。

# constexpr
C++ 本身已经具备了常量表达式的概念，比如 1+2, 3*4 这种表达式总是会产生相同的结果并且没有任何副作用。如果编译器能够在编译时就把这些表达式直接优化并植入到程序运行时，将能增加程序的性能。一个非常明显的例子就是在数组的定义阶段：
```C++
#include <iostream>
#define LEN 10

int len_foo() {
    int i = 2;
    return i;
}
constexpr int len_foo_constexpr() {
    return 5;
}

constexpr int fibonacci(const int n) {
    return n == 1 || n == 2 ? 1 : fibonacci(n-1)+fibonacci(n-2);
}

int main() {
    char arr_1[10];                      // 合法
    char arr_2[LEN];                     // 合法

    int len = 10;
    // char arr_3[len];                  // 非法

    const int len_2 = len + 1;
    constexpr int len_2_constexpr = 1 + 2 + 3;
    // char arr_4[len_2];                // 非法
    char arr_4[len_2_constexpr];         // 合法

    // char arr_5[len_foo()+5];          // 非法
    char arr_6[len_foo_constexpr() + 1]; // 合法

    std::cout << fibonacci(10) << std::endl;
    // 1, 1, 2, 3, 5, 8, 13, 21, 34, 55
    std::cout << fibonacci(10) << std::endl;
    return 0;
}
```
上面的例子中，char arr_4[len_2] 可能比较令人困惑，因为 len_2 已经被定义为了常量。为什么 char arr_4[len_2] 仍然是非法的呢？**这是因为 C++ 标准中数组的长度必须是一个常量表达式**，而对于 len_2 而言，这是一个 const 常数，而不是一个常量表达式，因此（即便这种行为在大部分编译器中都支持，但是）它是一个非法的行为，我们需要使用接下来即将介绍的 C++11 引入的 constexpr 特性来解决这个问题；而对于 arr_5 来说，C++98 之前的编译器无法得知 len_foo() 在运行期实际上是返回一个常数，这也就导致了非法的产生。

>注意，现在大部分编译器其实都带有自身编译优化，很多非法行为在编译器优化的加持下会变得合法，若需重现编译报错的现象需要使用老版本的编译器。

此外，constexpr 修饰的函数可以使用递归：
```C++
constexpr int fibonacci(const int n) {
    return n == 1 || n == 2 ? 1 : fibonacci(n-1)+fibonacci(n-2);
}
```

从 C++14 开始，constexpr 函数可以在内部使用局部变量、循环和分支等简单语句，例如下面的代码在 C++11 的标准下是不能够通过编译的：
```C++
constexpr int fibonacci(const int n) {
    if(n == 1) return 1;
    if(n == 2) return 1;
    return fibonacci(n-1) + fibonacci(n-2);
}
// warning: use of this statement in a constexpr function is a C++14 extension [-Wc++14-extensions]
```
为此，我们可以写出下面这类简化的版本来使得函数从 C++11 开始即可用：
```C++
constexpr int fibonacci(const int n) {
    return n == 1 || n == 2 ? 1 : fibonacci(n-1) + fibonacci(n-2);
}
```

