# Syntax Note - `data_structure/pool`

## Rvalue References and Move Semantics

### Why this feature exists

Before C++11, C++ only had lvalue references. This meant that when you wanted to pass an object to a function, you had to decide whether to pass it by value (which would involve copying the object) or by reference (which could lead to unintended side effects if the function modified the object).

In the `copy constructor` whcih with `Lvalue reference`, as shown in the example below, it will create a new object by copying the data from another object. This can be inefficient if the object is large, as it involves copying all of its data.

In contrast, the `move constructor` with `Rvalue reference` allows you to transfer ownership of the data from one object to another without copying. This is more efficient, especially for large objects, as it avoids the overhead of copying.

```c++
class MyClass {
    public:
    // copy constructor
    MyClass(const MyClass& other) {
        // copy the data from other
    }

    // move constructor
    MyClass(MyClass&& other) {
        // move the data from other
    }
};
```

#### Universal references

But this is not enough, because we also want to be able to write functions that can accept both lvalues and rvalues without having to write separate overloads for each. This is where `universal references` come in. 

A universal reference is a special type of reference that can bind to both lvalues and rvalues, it will be `type-deduced` based on the value category of the argument passed to the function. Then it will do `reference collapsing` to determine the final type of the reference.

For example, in the function template below, `T&& arg` is a universal reference, **forwarding reference**.
- If `arg` is an `lvalue` with type `E`, then `T` will be deduced as `E&`, and the type of `arg` will be `E& &&`, which collapses to `E&`.
- If `arg` is an `rvalue` with type `E`, then `T` will be deduced as `E`, and the type of `arg` will be `E&&`, which is an rvalue reference.

```c++
template <typename T>
void foo(T&& arg);

int a = 10;

foo(a); // T is deduced as int&, arg is int& &&, which collapses to int&
foo(10); // T is deduced as int, arg is int&&, which is an rvalue reference
```

#### Reference collapsing

How to understand `int && &&` or `int& &&`? This is where reference collapsing comes in. The rules for reference collapsing are as follows:

- `T& &` collapses to `T&`
    - lvalue reference to an lvalue reference collapses to an lvalue reference final type
- `T& &&` collapses to `T&`
    - lvalue reference to an rvalue reference collapses to an lvalue reference final type
- `T&& &` collapses to `T&`
    - rvalue reference to an lvalue reference collapses to an lvalue reference final type
- `T&& &&` collapses to `T&&`
    - rvalue reference to an rvalue reference collapses to an rvalue reference final type

For example:
```c++
template <typename T>
void foo(T&& arg) {
    // ...
}

int a = 10;
int&& r = 30;

// T& &  → T&: lvalue of type int
// Passing an lvalue (a) to foo
// T is deduced as int& (because a is an lvalue of type int)
// arg is of type int& &&, which collapses to int&
foo(a);

// same as above
foo(r);

// Passing an rvalue (10) to foo
// T is deduced as int (because 10 is an rvalue of type int)
// arg is of type int&&, which is an rvalue reference without collapsing.
foo(10);
```

```c++
using LRef = int&;
using RRef = int&&;

int a = 10;

// deduced int& & → collapses to int& (lvalue reference)
LRef&  x = a;

// deduced int&& & → collapses to int& (lvalue reference)
RRef&  y = a;

// deduced int& && → collapses to int& (lvalue reference)
LRef&& p = a;

// deduced int&& && → collapses to int&& (rvalue reference)
RRef&& q = 10;
```

#### Move semantics

In the move constructor, we can use `std::move` to indicate that we want to move the data from one object to another.
And the `std::forward` is used in the function template to perfectly forward the argument to another function, preserving its value category (lvalue or rvalue).

```c++
template <typename T>
void foo(T&& arg) {
    // perfectly forward arg to another function
    bar(std::forward<T>(arg));
}

int a = 10;

foo(std::move(a)); // T is deduced as int, arg is int&&, which is an rvalue reference
```


```c++
class MyClass {
    public:
    // move constructor
    MyClass(MyClass&& other) noexcept {
        // move the data from other
    }
};

MyClass a;

// 1. move constructor is called by explicitly calling std::move
MyClass b = std::move(a); 

// 2. a is an lvalue, so copy constructor is called
MyClass c(a);

// 3. pass a temporary object, so move constructor is called
MyClass d = MyClass();

// 4. function returns a temporary object, so move constructor is called
MyClass makeObj() { return MyClass(); }
MyClass e = makeObj();
```

> The `noexcept` specifier in the move constructor indicates that the move constructor does not throw exceptions. So for the STL containers, if the move constructor is `noexcept`, they will prefer to use the move constructor over the copy constructor when resizing or rehashing.

#### `std::forward`

Check the real case in `data_structure/pool`:

```c++
template <typename... TArgs> Object<TType> acquire(TArgs&&... args)
{
    if (available_.empty()) {
        throw std::runtime_error("no object is available.");
    }

    It it = available_.back();
    available_.pop_back();

    try {
        it->emplace(std::forward<TArgs>(args)...);
    }
    catch (...) {
        *it = std::nullopt;
        available_.push_back(it);
        throw;
    }

    return Object<TType>(it, this);
}

// function called by the user
std::string some_string = "hello";
auto obj = pool.acquire(1, 2.0, std::move(some_string));
```

**The value which has name is an lvalue**, 
input arguments will be inferred as rvalue references
    - `TArgs` will be deduced as `int`, `double`, and `std::string`
    - the type of `args` will be `int&&`, `double&&`, and `std::string&&` respectively.
    - `arg` is the name of the parameter, so it is an lvalue, even if its type is an rvalue reference.
With `std::forward`
    - all the arguments will be perfectly forwarded to `emplace with move semantics
Without `std::forward`
    - all the arguments will be treated as lvalues, which will lead to copy semantics instead

```c++
// `T` during the inferencing will record:
//   - caller passes an rvalue -> `T` indered as `int` -> std::forward<int> -> static_cast<int&&> -> move semantics
template <typename T>
void acquire(T&& arg) {
    emplace(std::forward<T>(arg));
}

// Without std::forward, it will always be an lvalue, even if the caller passes an rvalue, which will lead to copy semantics instead of move semantics.
template <typename T>
void acquire(T&& arg) {
    emplace(arg);
}

acquire(10); 
```

### Trade-off and Trap

1. After moving from an object, the state of the object is unspecified but valid. It is a common mistake to use an object after it has been moved from, which can lead to undefined behavior if the object is not in a valid state.

2. move semantics can lead to performance improvements, but it can also lead to performance degradation if not used correctly. For example, if you move an object that is small and cheap to copy, it may be more efficient to copy it instead of moving it. (e.g. `int`, `double`, etc. trivially copyable types)

3. `noexcept` will lead to `std::terminate` being called if an exception is thrown, so it should only be used when you are sure that the function will not throw exceptions. If you mark a move constructor as `noexcept` but it can actually throw exceptions, it can lead to unexpected behavior and crashes.
