# Syntax Note - Value Representation

> `std::optional<T>` Use type to represent if a value is present or not, but not use raw pointer or convention of "null" value.
> `std::expected<T, E>` Use type to represent if a value is present or not, and if not present, what is the error information, but not use exception or output parameter.
> `std::variant<T, U, ...>` Use type to represent a value that can be one of several types, but not use inheritance or union.

## `std::optional<T>`

### Why this feature exists

Before C++17, representing an optional value was:
1. **magic value**
    - cons: caller needs to know the magic value, and it may be a valid value in some cases
2. **raw pointer**
    - cons: ambiguous, it may be a valid pointer, and it may cause memory leak if caller forgets to free the memory
3. **output parameter + return bool**
    - redundant, caller needs to prepare an output parameter, and it may cause confusion if caller forgets to check the return value

```c++
// 1. magic value
int find(const std::vector<int>& v, int target); // return -1 if not found

// 2. raw pointer
const Config* getConfig(); // returns nullptr if not configured

// 3. output parameter + return bool
bool tryGetValue(int key, std::string& out);
```

`std::optional<T>` provides a clear and type-safe way to represent an optional value, let value semantics be part of the type system.

### Mental model

1. It is NOT a pointer
    - It won't have null dereference issue
    - It is a value, it can be copied, moved, assigned, etc.
2. It is NOT nullable
    - `nullptr` is not a valid value of `std::optional<T>`
    - `std::nullopt` is the only way to represent "no value"
3. Empty optional is a valid state
    - It is not an error to have an empty optional, it just means "no value"

### Trade-off

#### When to use `std::optional<T>`
1. the function may not be able to return a value
2. the configuration is optional
3. Lazy initialization, where the value is expensive to compute and may not be needed

#### When NOT to use

1. Use `std::optional<T>` when the absence of a value is an error condition, in which case `std::expected<T, E>` may be more appropriate.
2. Most of time the value is null, should consider using pointer or reference instead of `std::optional<T>`, as it may be more efficient and easier to use.

## `std::expected<T, E>`

### Why this feature exists

But `std::optional<T>` only represents the presence or absence of a value, it does not provide any information about why the value is absent. `std::expected<T, E>` provides a way to represent both the value and the error information in a single type.

How it was done before C++23:
1. exception
    - cons: it may be expensive to throw and catch exceptions, and it may not be suitable for all use cases
2. output parameter + return bool
    - cons: redundant, caller needs to prepare an output parameter, and it may cause confusion

With `std::expected<T, E>`, the return can be either a value of type `T` or an error of type `E`, and the caller can easily check which one it is and handle it accordingly.

```c++
// 1. exception
int divide(int a, int b) {
    if (b == 0) {
        throw std::invalid_argument("division by zero");
    }
    return a / b;
}

// 2. output parameter + return bool
bool tryDivide(int a, int b, int& out) {
    if (b == 0) {
        return false;
    }
    out = a / b;
    return true;
}

// with std::expected<T, E>
std::expected<int, std::string> divide(int a, int b) {
    if (b == 0) {
        return std::unexpected("division by zero");
    }
    return a / b;
}

auto result = divide(10, 0);
if (result) {
    std::cout << "Result: " << *result << std::endl;
} else {
    std::cout << "Error: " << result.error() << std::endl;
}
```

#### Monadic operations

Like `std::optional<T>`, `std::expected<T, E>` also provides monadic operations like `map`, `and_then`, etc. to allow chaining operations on the value or error. Which is much more convenient than nested if-else statements.

## `std::variant<T, U, ...>`

### Why this feature exists

Before C++17, in C/C++ we can use `union` or inheritance to represent a value that can be one of several types, but both of them have their own drawbacks:

1. `union`
    - pros: it gives contiguous memory layout, and it can be more efficient in some cases
    - cons: it is not type-safe, and it can only hold one of the types at a time, and it does not have any way to know which type it currently holds
2. inheritance
    - pros: it is type-safe, and it can hold multiple types at the same time
    - cons: it may have performance overhead due to virtual function calls, and it may not be suitable for all use cases

With `std::variant<T, U, ...>`, we can represent a value that can be one of several types in a type-safe way, and it provides a way to know which type it currently holds.

```c++
// 1. union
union Value {
    int i;
    double d;
};

auto value = Value{.i = 42}; // holds an int
value.d = 3.14;

// 2. inheritance
struct Base {
    virtual ~Base() = default;
};

struct IntValue : Base {
    int value;
};

struct DoubleValue : Base {
    double value;
};

auto value = std::make_unique<IntValue>(42); // holds an int
value = std::make_unique<DoubleValue>(3.14); // holds a double

// with std::variant<T, U, ...>
std::variant<int, double> value;
value = 42; // holds an int
value = 3.14; // holds a double
```

#### `std::visit`

With `std::visit`, we can force the caller to handle all possible types in a type-safe way, which is much more convenient than dynamic_cast or if-else statements.

### Mental model

Different from `std::optional<T>` and `std::expected<T, E>`, `std::variant<T, U, ...>` is not about the presence or absence of a value, but about the type of the value. It can hold a value of one of the specified types, and it provides a way to know which type it currently holds.

### Trade-off

#### When to use `std::variant<T, U, ...>`

It's perfect for state machines, where the state can be one of several types, and you want to enforce handling all possible states in a type-safe way.

## Summary

- std::optional<T>           present or absent         — absence without a reason
- std::variant<T, U, ...>    T or U or ...             — multiple types as equals
- std::expected<T, E>        success(T) or failure(E)  — a directed either

Decision guide:

- Absence means "not found" or "not configured" → optional
- A value can be one of several unrelated types → variant
- An operation can fail and the caller needs to know why → expected
