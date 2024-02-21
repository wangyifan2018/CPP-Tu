右值引用是 C++11 引入的与 Lambda 表达式齐名的重要特性之一。它的引入解决了 C++ 中大量的历史遗留问题， 消除了诸如 std::vector、std::string 之类的额外开销， 也才使得函数对象容器 std::function 成为了可能。

# 左值、右值的纯右值、将亡值、右值
要弄明白右值引用到底是怎么一回事，必须要对左值和右值做一个明确的理解。

**左值** (lvalue, left value)，顾名思义就是赋值符号左边的值。准确来说， 左值是表达式（不一定是赋值表达式）后依然存在的持久对象。

**右值** (rvalue, right value)，右边的值，是指表达式结束后就不再存在的临时对象。

而 C++11 中为了引入强大的右值引用，将右值的概念进行了进一步的划分，分为：纯右值、将亡值。

**纯右值** (prvalue, pure rvalue)，纯粹的右值，要么是纯粹的字面量，例如 10, true； 要么是求值结果相当于字面量或匿名临时对象，例如 1+2。非引用返回的临时变量、运算表达式产生的临时变量、 原始字面量、Lambda 表达式都属于纯右值。

需要注意的是，字面量除了字符串字面量以外，均为纯右值。而字符串字面量是一个左值，类型为 const char 数组。例如：
```C++
#include <type_traits>

int main() {
    // 正确，"01234" 类型为 const char [6]，因此是左值
    const char (&left)[6] = "01234";

    // 断言正确，确实是 const char [6] 类型，注意 decltype(expr) 在 expr 是左值
    // 且非无括号包裹的 id 表达式与类成员表达式时，会返回左值引用
    static_assert(std::is_same<decltype("01234"), const char(&)[6]>::value, "");

    // 错误，"01234" 是左值，不可被右值引用
    // const char (&&right)[6] = "01234";
}
```

但是注意，数组可以被隐式转换成相对应的指针类型，而转换表达式的结果（如果不是左值引用）则一定是个右值（右值引用为将亡值，否则为纯右值）。例如：
```C++
const char*   p   = "01234";  // 正确，"01234" 被隐式转换为 const char*
const char*&& pr  = "01234";  // 正确，"01234" 被隐式转换为 const char*，该转换的结果是纯右值
// const char*& pl = "01234"; // 错误，此处不存在 const char* 类型的左值
```

但是注意，数组可以被隐式转换成相对应的指针类型，而转换表达式的结果（如果不是左值引用）则一定是个右值（右值引用为将亡值，否则为纯右值）。例如：
```C++
const char*   p   = "01234";  // 正确，"01234" 被隐式转换为 const char*
const char*&& pr  = "01234";  // 正确，"01234" 被隐式转换为 const char*，该转换的结果是纯右值
// const char*& pl = "01234"; // 错误，此处不存在 const char* 类型的左值
```

**将亡值** (xvalue, expiring value)，是 C++11 为了引入右值引用而提出的概念（因此在传统 C++ 中， 纯右值和右值是同一个概念），也就是即将被销毁、却能够被移动的值。

将亡值可能稍有些难以理解，我们来看这样的代码：
```C++
std::vector<int> foo() {
    std::vector<int> temp = {1, 2, 3, 4};
    return temp;
}

std::vector<int> v = foo();
```
在这样的代码中，就传统的理解而言，函数 foo 的返回值 temp 在内部创建然后被赋值给 v， 然而 v 获得这个对象时，会将整个 temp 拷贝一份，然后把 temp 销毁，如果这个 temp 非常大， 这将造成大量额外的开销（这也就是传统 C++ 一直被诟病的问题）。在最后一行中，v 是左值、 foo() 返回的值就是右值（也是纯右值）。但是，v 可以被别的变量捕获到， 而 foo() 产生的那个返回值作为一个临时值，一旦被 v 复制后，将立即被销毁，无法获取、也不能修改。 而将亡值就定义了这样一种行为：临时的值能够被识别、同时又能够被移动。

在 C++11 之后，编译器为我们做了一些工作，此处的左值 temp 会被进行此隐式右值转换， 等价于 static_cast<std::vector<int> &&>(temp)，进而此处的 v 会将 foo 局部返回的值进行移动。 也就是后面我们将会提到的移动语义。

# 右值引用和左值引用
要拿到一个将亡值，就需要用到右值引用：T &&，其中 T 是类型。 右值引用的声明让这个临时值的生命周期得以延长、只要变量还活着，那么将亡值将继续存活。

C++11 提供了 std::move 这个方法将左值参数无条件的转换为右值， 有了它我们就能够方便的获得一个右值临时对象，例如：
```C++
#include <iostream>
#include <string>

void reference(std::string& str) {
    std::cout << "左值" << std::endl;
}
void reference(std::string&& str) {
    std::cout << "右值" << std::endl;
}

int main()
{
    std::string lv1 = "string,"; // lv1 是一个左值
    // std::string&& r1 = lv1; // 非法, 右值引用不能引用左值
    std::string&& rv1 = std::move(lv1); // 合法, std::move可以将左值转移为右值
    std::cout << rv1 << std::endl; // string,

    const std::string& lv2 = lv1 + lv1; // 合法, 常量左值引用能够延长临时变量的生命周期
    // lv2 += "Test"; // 非法, 常量引用无法被修改
    std::cout << lv2 << std::endl; // string,string,

    std::string&& rv2 = lv1 + lv2; // 合法, 右值引用延长临时对象生命周期
    rv2 += "Test"; // 合法, 非常量引用能够修改临时变量
    std::cout << rv2 << std::endl; // string,string,string,Test

    reference(rv2); // 输出左值

    return 0;
}
```

注意，这里有一个很有趣的历史遗留问题，我们先看下面的代码：
```C++
#include <iostream>

int main() {
    // int &a = std::move(1);    // 不合法，非常量左引用无法引用右值
    const int &b = std::move(1); // 合法, 常量左引用允许引用右值

    std::cout << a << b << std::endl;
}
```

第一个问题，为什么不允许非常量引用绑定到非左值？这是因为这种做法存在逻辑错误：
```C++
void increase(int & v) {
    v++;
}
void foo() {
    double s = 1;
    increase(s);
}
```

由于 int& 不能引用 double 类型的参数，因此必须产生一个临时值来保存 s 的值， 从而当 increase() 修改这个临时值时，调用完成后 s 本身并没有被修改。

第二个问题，为什么常量引用允许绑定到非左值？原因很简单，因为 Fortran 需要。


# 移动语义
传统 C++ 通过拷贝构造函数和赋值操作符为类对象设计了拷贝/复制的概念，但为了实现对资源的移动操作， 调用者必须使用先复制、再析构的方式，否则就需要自己实现移动对象的接口。 试想，搬家的时候是把家里的东西直接搬到新家去，而不是将所有东西复制一份（重买）再放到新家、 再把原来的东西全部扔掉（销毁），这是非常反人类的一件事情。

传统的 C++ 没有区分『移动』和『拷贝』的概念，造成了大量的数据拷贝，浪费时间和空间。 右值引用的出现恰好就解决了这两个概念的混淆问题，例如：
```C++
#include <iostream>
class A {
public:
    int *pointer;
    A():pointer(new int(1)) {
        std::cout << "构造" << pointer << std::endl;
    }
    A(A& a):pointer(new int(*a.pointer)) {
        std::cout << "拷贝" << pointer << std::endl;
    } // 无意义的对象拷贝
    A(A&& a):pointer(a.pointer) {
        a.pointer = nullptr;
        std::cout << "移动" << pointer << std::endl;
    }
    ~A(){
        std::cout << "析构" << pointer << std::endl;
        delete pointer;
    }
};
// 防止编译器优化
A return_rvalue(bool test) {
    A a,b;
    if(test) return a; // 等价于 static_cast<A&&>(a);
    else return b;     // 等价于 static_cast<A&&>(b);
}
int main() {
    A obj = return_rvalue(false);
    std::cout << "obj:" << std::endl;
    std::cout << obj.pointer << std::endl;
    std::cout << *obj.pointer << std::endl;
    return 0;
}
```

在上面的代码中：

1. 首先会在 return_rvalue 内部构造两个 A 对象，于是获得两个构造函数的输出；
2. 函数返回后，产生一个将亡值，被 A 的移动构造（A(A&&)）引用，从而延长生命周期，并将这个右值中的指针拿到，保存到了 obj 中，而将亡值的指针被设置为 nullptr，防止了这块内存区域被销毁。

从而避免了无意义的拷贝构造，加强了性能。再来看看涉及标准库的例子：
```C++
#include <iostream> // std::cout
#include <utility> // std::move
#include <vector> // std::vector
#include <string> // std::string

int main() {

    std::string str = "Hello world.";
    std::vector<std::string> v;

    // 将使用 push_back(const T&), 即产生拷贝行为
    v.push_back(str);
    // 将输出 "str: Hello world."
    std::cout << "str: " << str << std::endl;

    // 将使用 push_back(const T&&), 不会出现拷贝行为
    // 而整个字符串会被移动到 vector 中，所以有时候 std::move 会用来减少拷贝出现的开销
    // 这步操作后, str 中的值会变为空
    v.push_back(std::move(str));
    // 将输出 "str: "
    std::cout << "str: " << str << std::endl;

    return 0;
}
```

>在这段代码中，str 字符串对象在执行 std::move(str) 后仍然存在，但它的值变为空字符串。这是因为 std::move 将 str 转换为一个右值引用，这允许 std::vector 的 push_back 方法使用移动构造函数，而不是拷贝构造函数。
移动构造函数会从源（str）那里"窃取"资源（在这种情况下是字符串的内容），将资源转移到新的位置（在这里是向量 v 的新元素）。一旦移动完成，原始对象（str）不再拥有被移动的资源，并且通常会被置于一个有效但不确定的状态。对于标准库中的字符串来说，这通常意味着它变成了一个空字符串。
这个操作后，str 仍然是一个有效的 std::string 对象，但它不再包含之前的数据。你可以对它进行重新赋值，清除它，或者对它执行任何其他合法的 std::string 操作。

# 完美转发
前面我们提到了，一个声明的右值引用其实是一个左值。这就为我们进行参数转发（传递）造成了问题：
```C++
void reference(int& v) {
    std::cout << "左值" << std::endl;
}
void reference(int&& v) {
    std::cout << "右值" << std::endl;
}
template <typename T>
void pass(T&& v) {
    std::cout << "普通传参:";
    reference(v); // 始终调用 reference(int&)
}
int main() {
    std::cout << "传递右值:" << std::endl;
    pass(1); // 1是右值, 但输出是左值

    std::cout << "传递左值:" << std::endl;
    int l = 1;
    pass(l); // l 是左值, 输出左值

    return 0;
}
```

对于 pass(1) 来说，虽然传递的是右值，但由于 v 是一个引用，所以同时也是左值。 因此 reference(v) 会调用 reference(int&)，输出『左值』。 而对于pass(l)而言，l是一个左值，为什么会成功传递给 pass(T&&) 呢？

这是基于**引用坍缩**规则的：在传统 C++ 中，我们不能够对一个引用类型继续进行引用， 但 C++ 由于右值引用的出现而放宽了这一做法，从而产生了引用坍缩规则，允许我们对引用进行引用， 既能左引用，又能右引用。但是却遵循如下规则：


|  函数形参类型   | 实参参数类型  |  推导后函数形参类型 ｜
|  ----  | ----  | ------
| T&	  | 左引用 | T& ｜
| T&	  | 右引用 | T& ｜
| T&&   | 左引用 | T& ｜
| T&&		  | 右引用 | T&& ｜

因此，模板函数中使用 T&& 不一定能进行右值引用，当传入左值时，此函数的引用将被推导为左值。 更准确的讲，无论模板参数是什么类型的引用，当且仅当实参类型为右引用时，模板参数才能被推导为右引用类型。 这才使得 v 作为左值的成功传递。

完美转发就是基于上述规律产生的。所谓完美转发，就是为了让我们在传递参数的时候， 保持原来的参数类型（左引用保持左引用，右引用保持右引用）。 为了解决这个问题，我们应该使用 std::forward 来进行参数的转发（传递）：

```C++
#include <iostream>
#include <utility>
void reference(int& v) {
    std::cout << "左值引用" << std::endl;
}
void reference(int&& v) {
    std::cout << "右值引用" << std::endl;
}
template <typename T>
void pass(T&& v) {
    std::cout << "              普通传参: ";
    reference(v);
    std::cout << "       std::move 传参: ";
    reference(std::move(v));
    std::cout << "    std::forward 传参: ";
    reference(std::forward<T>(v));
    std::cout << "static_cast<T&&> 传参: ";
    reference(static_cast<T&&>(v));
}
int main() {
    std::cout << "传递右值:" << std::endl;
    pass(1);

    std::cout << "传递左值:" << std::endl;
    int v = 1;
    pass(v);

    return 0;
}
```

输出结果为：
```
传递右值:
              普通传参: 左值引用
       std::move 传参: 右值引用
    std::forward 传参: 右值引用
static_cast<T&&> 传参: 右值引用
传递左值:
              普通传参: 左值引用
       std::move 传参: 右值引用
    std::forward 传参: 左值引用
static_cast<T&&> 传参: 左值引用
```

无论传递参数为左值还是右值，普通传参都会将参数作为左值进行转发； 由于类似的原因，std::move 总会接受到一个左值，从而转发调用了reference(int&&) 输出右值引用。

唯独 std::forward 即没有造成任何多余的拷贝，同时完美转发(传递)了函数的实参给了内部调用的其他函数。

std::forward 和 std::move 一样，没有做任何事情，std::move 单纯的将左值转化为右值， std::forward 也只是单纯的将参数做了一个类型的转换，从现象上来看， std::forward<T>(v) 和 static_cast<T&&>(v) 是完全一样的。

读者可能会好奇，为何一条语句能够针对两种类型的返回对应的值， 我们再简单看一看 std::forward 的具体实现机制，std::forward 包含两个重载：
```C++
template<typename _Tp>
constexpr _Tp&& forward(typename std::remove_reference<_Tp>::type& __t) noexcept
{ return static_cast<_Tp&&>(__t); }

template<typename _Tp>
constexpr _Tp&& forward(typename std::remove_reference<_Tp>::type&& __t) noexcept
{
    static_assert(!std::is_lvalue_reference<_Tp>::value, "template argument"
        " substituting _Tp is an lvalue reference type");
    return static_cast<_Tp&&>(__t);
}
```

在这份实现中，std::remove_reference 的功能是消除类型中的引用， std::is_lvalue_reference 则用于检查类型推导是否正确，在 std::forward 的第二个实现中 检查了接收到的值确实是一个左值，进而体现了坍缩规则。

当 std::forward 接受左值时，_Tp 被推导为左值，所以返回值为左值；而当其接受右值时， _Tp 被推导为 右值引用，则基于坍缩规则，返回值便成为了 && + && 的右值。 可见 std::forward 的原理在于巧妙的利用了模板类型推导中产生的差异。

这时我们能回答这样一个问题：为什么在使用循环语句的过程中，auto&& 是最安全的方式？ 因为当 auto 被推导为不同的左右引用时，与 && 的坍缩组合是完美转发。

