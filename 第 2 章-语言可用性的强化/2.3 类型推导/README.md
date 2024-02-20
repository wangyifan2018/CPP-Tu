在传统 C 和 C++ 中，参数的类型都必须明确定义，这其实对我们快速进行编码没有任何帮助，尤其是当我们面对一大堆复杂的模板类型时，必须明确的指出变量的类型才能进行后续的编码，这不仅拖慢我们的开发效率，也让代码变得又臭又长。

C++11 引入了 auto 和 decltype 这两个关键字实现了类型推导，让编译器来操心变量的类型。这使得 C++ 也具有了和其他现代编程语言一样，某种意义上提供了无需操心变量类型的使用习惯。

# auto
auto 在很早以前就已经进入了 C++，但是他始终作为一个存储类型的指示符存在，与 register 并存。在传统 C++ 中，如果一个变量没有声明为 register 变量，将自动被视为一个 auto 变量。而随着 register 被弃用（在 C++17 中作为保留关键字，以后使用，目前不具备实际意义），对 auto 的语义变更也就非常自然了。

使用 auto 进行类型推导的一个最为常见而且显著的例子就是迭代器。你应该在前面的小节里看到了传统 C++ 中冗长的迭代写法：
```C++
// 在 C++11 之前
// 由于 cbegin() 将返回 vector<int>::const_iterator
// 所以 it 也应该是 vector<int>::const_iterator 类型
for(vector<int>::const_iterator it = vec.cbegin(); it != vec.cend(); ++it)
```
而有了 auto 之后可以：
```C++

#include <initializer_list>
#include <vector>
#include <iostream>

class MagicFoo {
public:
    std::vector<int> vec;
    MagicFoo(std::initializer_list<int> list) {
        // 从 C++11 起, 使用 auto 关键字进行类型推导
        for (auto it = list.begin(); it != list.end(); ++it) {
            vec.push_back(*it);
        }
    }
};
int main() {
    MagicFoo magicFoo = {1, 2, 3, 4, 5};
    std::cout << "magicFoo: ";
    for (auto it = magicFoo.vec.begin(); it != magicFoo.vec.end(); ++it) {
        std::cout << *it << ", ";
    }
    std::cout << std::endl;
    return 0;
}
```
一些其他的常见用法：
```C++
auto i = 5;              // i 被推导为 int
auto arr = new auto(10); // arr 被推导为 int *
```
从 C++ 20 起，auto 甚至能用于函数传参，考虑下面的例子：
```C++
int add(auto x, auto y) {
    return x+y;
}

auto i = 5; // 被推导为 int
auto j = 6; // 被推导为 int
std::cout << add(i, j) << std::endl;
```
>注意：auto 还不能用于推导数组类型：
```C++
auto auto_arr2[10] = {arr}; // 错误, 无法推导数组元素类型

2.6.auto.cpp:30:19: error: 'auto_arr2' declared as array of 'auto'
    auto auto_arr2[10] = {arr};
```

# decltype
decltype 关键字是为了解决 auto 关键字只能对变量进行类型推导的缺陷而出现的。它的用法和 typeof 很相似：
```C++
decltype(表达式)
```
有时候，我们可能需要计算某个表达式的类型，例如：
```C++
auto x = 1;
auto y = 2;
decltype(x+y) z;
```
你已经在前面的例子中看到 decltype 用于推断类型的用法，下面这个例子就是判断上面的变量 x, y, z 是否是同一类型：
```C++
if (std::is_same<decltype(x), int>::value)
    std::cout << "type x == int" << std::endl;
if (std::is_same<decltype(x), float>::value)
    std::cout << "type x == float" << std::endl;
if (std::is_same<decltype(x), decltype(z)>::value)
    std::cout << "type z == type x" << std::endl;
```
其中，std::is_same<T, U> 用于判断 T 和 U 这两个类型是否相等。输出结果为：
```C++
type x == int
type z == type x
```

# 尾返回类型推导
你可能会思考，在介绍 auto 时，我们已经提过 auto 不能用于函数形参进行类型推导，那么 auto 能不能用于推导函数的返回类型呢？还是考虑一个加法函数的例子，在传统 C++ 中我们必须这么写：
```C++
template<typename R, typename T, typename U>
R add(T x, U y) {
    return x+y;
}
```
>注意：typename 和 class 在模板参数列表中没有区别，在 typename 这个关键字出现之前，都是使用 class 来定义模板参数的。但在模板中定义有嵌套依赖类型的变量时，需要用 typename 消除歧义

这样的代码其实变得很丑陋，因为程序员在使用这个模板函数的时候，必须明确指出返回类型。但事实上我们并不知道 add() 这个函数会做什么样的操作，以及获得一个什么样的返回类型。

在 C++11 中这个问题得到解决。虽然你可能马上会反应出来使用 decltype 推导 x+y 的类型，写出这样的代码：
```C++
decltype(x+y) add(T x, U y)
```
但事实上这样的写法并不能通过编译。这是因为在编译器读到 decltype(x+y) 时，x 和 y 尚未被定义。为了解决这个问题，C++11 还引入了一个叫做尾返回类型（trailing return type），利用 auto 关键字将返回类型后置：
```C++
template<typename T, typename U>
auto add2(T x, U y) -> decltype(x+y){
    return x + y;
}
```
令人欣慰的是从 C++14 开始是可以直接让普通函数具备返回值推导，因此下面的写法变得合法：
```C++
template<typename T, typename U>
auto add3(T x, U y){
    return x + y;
}
```
可以检查一下类型推导是否正确：
```C++
// after c++11
auto w = add2<int, double>(1, 2.0);
if (std::is_same<decltype(w), double>::value) {
    std::cout << "w is double: ";
}
std::cout << w << std::endl;

// after c++14
auto q = add3<double, int>(1.0, 2);
std::cout << "q: " << q << std::endl;
```

# decltype(auto)
decltype(auto) 是 C++14 开始提供的一个略微复杂的用法。
>要理解它你需要知道 C++ 中参数转发的概念，我们会在语言运行时强化一章中详细介绍，你可以到时再回来看这一小节的内容。

简单来说，decltype(auto) 主要用于对转发函数或封装的返回类型进行推导，它使我们无需显式的指定 decltype 的参数表达式。考虑看下面的例子，当我们需要对下面两个函数进行封装时：
```C++
std::string  lookup1();
std::string& lookup2();
```
在 C++11 中，封装实现是如下形式：
```C++
std::string look_up_a_string_1() {
    return lookup1();
}
std::string& look_up_a_string_2() {
    return lookup2();
}
```
而有了 decltype(auto)，我们可以让编译器完成这一件烦人的参数转发：
```C++
decltype(auto) look_up_a_string_1() {
    return lookup1();
}
decltype(auto) look_up_a_string_2() {
    return lookup2();
}
```












