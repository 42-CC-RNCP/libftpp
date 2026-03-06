# Syntax Note - Variadic Templates

## `template<typename... Args>`

### Why this feature exists

- Support variadic number and type of template parameters (C++11)
- Support recursive data structure as template parameters (C++11)
- Type-safe way to represent a list of types which can replace `va_list` in C-style variadic functions

```c++
// 1. variadic number and type of template parameters
template<typename... Args>
void foo(Args... args) {
    // ...
}

foo(1, 2.0, "hello"); // Args is deduced to be <int, double, const char*>

// 2. recursive data structure as template parameters

// use for terminal case
void print() {}

template<typename T, typename... Rest>
void print(T first, Rest... rest) {
    std::cout << first << std::endl;
    if constexpr (sizeof...(rest) > 0) {
        // recursive call with the rest of the arguments
        // the last call will be print() which is the terminal case
        print(rest...);
    }
}

// 3. fold expression (C++17)
template<typename... Args>
void print(Args... args) {
    (std::cout << ... << args) << std::endl; // fold expression to print all arguments
}

template<typename... Args>
auto sum(Args... args) {
    return (args + ...); // fold expression to sum all arguments
}
```
