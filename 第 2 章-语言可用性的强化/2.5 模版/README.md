C++ 的模板一直是这门语言的一种特殊的艺术，模板甚至可以独立作为一门新的语言来进行使用。模板的哲学在于将一切能够在编译期处理的问题丢到编译期进行处理，仅在运行时处理那些最核心的动态服务，进而大幅优化运行期的性能。因此模板也被很多人视作 C++ 的黑魔法之一。

# 外部模板
传统 C++ 中，模板只有在使用时才会被编译器实例化。换句话说，只要在每个编译单元（文件）中编译的代码中遇到了被完整定义的模板，都会实例化。这就产生了重复实例化而导致的编译时间的增加。并且，我们没有办法通知编译器不要触发模板的实例化。

为此，C++11 引入了外部模板，扩充了原来的强制编译器在特定位置实例化模板的语法，使我们能够显式的通知编译器何时进行模板的实例化：
```C++
template class std::vector<bool>;          // 强行实例化
extern template class std::vector<double>; // 不在该当前编译文件中实例化模板
```

# 尖括号 ">"
在传统 C++ 的编译器中，>>一律被当做右移运算符来进行处理。但实际上我们很容易就写出了嵌套模板的代码：
```C++
std::vector<std::vector<int>> matrix;
```
这在传统 C++ 编译器下是不能够被编译的，而 C++11 开始，连续的右尖括号将变得合法，并且能够顺利通过编译。甚至于像下面这种写法都能够通过编译：
```C++
template<bool T>
class MagicType {
    bool magic = T;
};

// in main function:
std::vector<MagicType<(1>2)>> magic; // 合法, 但不建议写出这样的代码
```

# 类型别名模板
在了解类型别名模板之前，需要理解『模板』和『类型』之间的不同。仔细体会这句话：模板是用来产生类型的。在传统 C++ 中，typedef 可以为类型定义一个新的名称，但是却没有办法为模板定义一个新的名称。因为，模板不是类型。例如：
```C++
template<typename T, typename U>
class MagicType {
public:
    T dark;
    U magic;
};

// 不合法
template<typename T>
typedef MagicType<std::vector<T>, std::string> FakeDarkMagic;
```

C++11 使用 using 引入了下面这种形式的写法，并且同时支持对传统 typedef 相同的功效：

>通常我们使用 typedef 定义别名的语法是：typedef 原名称 新名称;，但是对函数指针等别名的定义语法却不相同，这通常给直接阅读造成了一定程度的困难。

```C++
typedef int (*process)(void *);
using NewProcess = int(*)(void *);
template<typename T>
using TrueDarkMagic = MagicType<std::vector<T>, std::string>;

int main() {
    TrueDarkMagic<bool> you;
}
```

# 变长参数模板
模板一直是 C++ 所独有的黑魔法（一起念：Dark Magic）之一。 在 C++11 之前，无论是类模板还是函数模板，都只能按其指定的样子， 接受一组固定数量的模板参数；而 C++11 加入了新的表示方法， 允许任意个数、任意类别的模板参数，同时也不需要在定义时将参数的个数固定。
```C++
template<typename... Ts> class Magic;
```
模板类 Magic 的对象，能够接受不受限制个数的 typename 作为模板的形式参数，例如下面的定义：
```C++
class Magic<int,
            std::vector<int>,
            std::map<std::string,
            std::vector<int>>> darkMagic;
```

既然是任意形式，所以个数为 0 的模板参数也是可以的：class Magic<> nothing;。

如果不希望产生的模板参数个数为 0，可以手动的定义至少一个模板参数：

```C++
template<typename Require, typename... Args> class Magic;
```

变长参数模板也能被直接调整到到模板函数上。传统 C 中的 printf 函数， 虽然也能达成不定个数的形参的调用，但其并非类别安全。 而 C++11 除了能定义类别安全的变长参数函数外， 还可以使类似 printf 的函数能自然地处理非自带类别的对象。 除了在模板参数中能使用 ... 表示不定长模板参数外， 函数参数也使用同样的表示法代表不定长参数， 这也就为我们简单编写变长参数函数提供了便捷的手段，例如：

```C++
template<typename... Args> void printf(const std::string &str, Args... args);
```

那么我们定义了变长的模板参数，如何对参数进行解包呢？

首先，我们可以使用 sizeof... 来计算参数的个数，：

```C++
template<typename... Ts>
void magic(Ts... args) {
    std::cout << sizeof...(args) << std::endl;
}
```
我们可以传递任意个参数给 magic 函数：
```C++
magic(); // 输出0
magic(1); // 输出1
magic(1, ""); // 输出2
```


## 1. 递归模板函数

递归是非常容易想到的一种手段，也是最经典的处理方法。这种方法不断递归地向函数传递模板参数，进而达到递归遍历所有模板参数的目的：
```C++
#include <iostream>
template<typename T0>
void printf1(T0 value) {
    std::cout << value << std::endl;
}
template<typename T, typename... Ts>
void printf1(T value, Ts... args) {
    std::cout << value << std::endl;
    printf1(args...);
}
int main() {
    printf1(1, 2, "123", 1.1);
    return 0;
}
```

## 2. 变参模板展开
你应该感受到了这很繁琐，在 C++17 中增加了变参模板展开的支持，于是你可以在一个函数中完成 printf 的编写：
```C++
template<typename T0, typename... T>
void printf2(T0 t0, T... t) {
    std::cout << t0 << std::endl;
    if constexpr (sizeof...(t) > 0) printf2(t...);
}
```
>事实上，有时候我们虽然使用了变参模板，却不一定需要对参数做逐个遍历，我们可以利用 std::bind 及完美转发等特性实现对函数和参数的绑定，从而达到成功调用的目的。

## 3. 初始化列表展开
递归模板函数是一种标准的做法，但缺点显而易见的在于必须定义一个终止递归的函数。

这里介绍一种使用初始化列表展开的黑魔法：
```C++
template<typename T, typename... Ts>
auto printf3(T value, Ts... args) {
    std::cout << value << std::endl;
    (void) std::initializer_list<T>{([&args] {
        std::cout << args << std::endl;
    }(), value)...};
}
```

在这个代码中，额外使用了 C++11 中提供的初始化列表以及 Lambda 表达式的特性（下一节中将提到）。

通过初始化列表，(lambda 表达式, value)... 将会被展开。由于逗号表达式的出现，首先会执行前面的 lambda 表达式，完成参数的输出。 为了避免编译器警告，我们可以将 std::initializer_list 显式的转为 void。

# 折叠表达式
C++ 17 中将变长参数这种特性进一步带给了表达式，考虑下面这个例子：
```C++
#include <iostream>
template<typename ... T>
auto sum(T ... t) {
    return (t + ...);
}
int main() {
    std::cout << sum(1, 2, 3, 4, 5, 6, 7, 8, 9, 10) << std::endl;
}
```

# 非类型模板参数推导
前面我们主要提及的是模板参数的一种形式：类型模板参数。
```C++
template <typename T, typename U>
auto add(T t, U u) {
    return t+u;
}
```

其中模板的参数 T 和 U 为具体的类型。 但还有一种常见模板参数形式可以让不同字面量成为模板参数，即非类型模板参数：
```C++
template <typename T, int BufSize>
class buffer_t {
public:
    T& alloc();
    void free(T& item);
private:
    T data[BufSize];
}

buffer_t<int, 100> buf; // 100 作为模板参数
```

在这种模板参数形式下，我们可以将 100 作为模板的参数进行传递。 在 C++11 引入了类型推导这一特性后，我们会很自然的问，既然此处的模板参数 以具体的字面量进行传递，能否让编译器辅助我们进行类型推导， 通过使用占位符 auto 从而不再需要明确指明类型？ 幸运的是，C++17 引入了这一特性，我们的确可以 auto 关键字，让编译器辅助完成具体类型的推导， 例如：
```C++
template <auto value> void foo() {
    std::cout << value << std::endl;
    return;
}

int main() {
    foo<10>();  // value 被推导为 int 类型
}
```













