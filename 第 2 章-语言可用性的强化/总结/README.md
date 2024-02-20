# 总结
本节介绍了现代 C++ 中对语言可用性的增强，其中笔者认为最为重要的几个特性是几乎所有人都需要了解并熟练使用的：

auto 类型推导
范围 for 迭代
初始化列表
变参模板


# 习题
1. 使用结构化绑定，仅用一行函数内代码实现如下函数：
```C++
template <typename Key, typename Value, typename F>
void update(std::map<Key, Value>& m, F foo) {
    // TODO:
}
int main() {
    std::map<std::string, long long int> m {
        {"a", 1},
        {"b", 2},
        {"c", 3}
    };
    update(m, [](std::string key) {
        return std::hash<std::string>{}(key);
    });
    for (auto&& [key, value] : m)
        std::cout << key << ":" << value << std::endl;
}
```


2. 尝试用折叠表达式实现用于计算均值的函数，传入允许任意参数。