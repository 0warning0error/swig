# SWIG Rust 绑定设计文档

## 概述

本文档记录为 SWIG 添加 Rust 语言绑定支持的设计决策和实现方案。

---

## 1. 核心设计原则

### 1.1 两层架构

```
┌─────────────────────────────────────────────┐
│      Rust 绑定层（用户直接使用）              │
│   - struct + trait + impl                   │
│   - 安全包装 API                             │
│   - 所有权语义映射                           │
└──────────────────────┬──────────────────────┘
                       │
┌──────────────────────▼──────────────────────┐
│      FFI 层（自动生成的 unsafe 代码）         │
│   - extern "C" 函数声明                      │
│   - 原始指针传递                             │
│   - c_int, c_char 等类型映射                 │
└──────────────────────┬──────────────────────┘
                       │
┌──────────────────────▼──────────────────────┐
│      C/C++ 原始库                            │
│   - extern "C" 包装函数                      │
└─────────────────────────────────────────────┘
```

### 1.2 C 接口对称暴露

C++ 端和 Rust 端都使用 `extern "C"`，符号名对齐：

```
C++ 端:                          Rust 端:
extern "C" {                     mod ffi {
  void MyClass_method(...);        extern "C" {
}                                   pub fn MyClass_method(...);
                                  }
                                }
```

---

## 2. 类处理设计

### 2.1 C++ 类拆分模式

C++ 类拆分为 Rust 的 struct + trait + impl：

```cpp
// C++ 类
class Animal {
public:
    virtual void speak();
    virtual int getAge() const;           // const 方法
    void setName(const std::string& name); // 非 const 方法
};
```

生成 Rust 代码：

```rust
// 结构体（持有 C++ 对象指针）
pub struct Animal {
    ptr: *mut c_void,
}

// Trait（定义行为接口）
pub trait AnimalTrait {
    fn speak(&mut self);           // 非 const → &mut self
    fn get_age(&self) -> i32;      // const → &self
    fn set_name(&mut self, name: &str);
}

// 默认 impl（调用 C++ 基类实现）
impl AnimalTrait for Animal {
    fn speak(&mut self) {
        unsafe { ffi::Animal_speak(self.ptr) }
    }

    fn get_age(&self) -> i32 {
        unsafe { ffi::Animal_getAge(self.ptr) }
    }

    fn set_name(&mut self, name: &str) {
        unsafe { ffi::Animal_setName(self.ptr, name.as_ptr()) }
    }
}
```

### 2.2 所有权映射规则

| C++ 方法类型 | Rust self 类型 |
|-------------|----------------|
| 非 const 成员函数 | `&mut self` |
| const 成员函数 | `&self` |
| 静态成员函数 | 关联函数 `fn method()` |
| 构造函数 | `fn new() -> Self` |
| 析构函数 | `impl Drop` |

### 2.3 构造函数命名规则

当将 C++ 构造函数映射到 Rust 时，需要考虑命名冲突问题。

**命名优先级**：

1. **首选**：`new` - Rust 惯用的构造函数名
2. **备选**：`create`、`make` - 常见的替代名称
3. **回退**：使用类名作为函数名（如 `Simple` 类的构造函数叫 `simple`）

**冲突检测流程**：

```
检查类中是否存在成员函数名为 "new"
  ├─ 不存在 → 使用 new()
  └─ 存在 → 检查是否存在成员函数名为 "create"
              ├─ 不存在 → 使用 create()
              └─ 存在 → 检查是否存在成员函数名为 "make"
                          ├─ 不存在 → 使用 make()
                          └─ 存在 → 使用类名（如 Animal()）
```

**示例**：

```cpp
// 情况1：无冲突
class Widget {
public:
    Widget();  // → Widget::new()
};

// 情况2：成员函数名为 new
class Factory {
public:
    Factory();        // → Factory::create()
    Widget* new(int); // 成员函数，保持原名
};

// 情况3：new、create、make 都被占用
class Builder {
public:
    Builder();              // → Builder::builder()
    void new(int);          // 成员函数
    void create(double);    // 成员函数
    void make(string);      // 成员函数
};
```

**重载构造函数**：使用 Trait 泛化方案（见第10章）

**默认参数**：使用 Builder 模式（见第11章）

---

## 3. 类型映射设计

### 3.1 Trivially Copyable 类型

**设计决策：用户手动指定，SWIG 不自动判断**

原因：
- 纯代码层面判断 trivially copyable 困难
- 用户有完全控制权
- 类似其他语言的 typemap 机制

```swig
// 用户在 SWIG 接口文件中指定
%typemap(rusttype) Point "Point"

// 用户自己在 Rust 端定义（不是 SWIG 生成）
#[repr(C)]
#[derive(Copy, Clone)]
pub struct Point {
    pub x: f64,
    pub y: f64,
}
```

### 3.2 非 Trivially Copyable 类型（默认行为）

如果用户没有提供 typemap，SWIG 生成不透明类型：

```rust
// C++ class MyClass → Rust
pub struct MyClass {
    ptr: *mut c_void,  // 不透明句柄
}
// 不实现 Copy/Clone
```

### 3.3 Typemap 类型定义

```swig
// Rust 类型映射
%typemap(rusttype)    int "i32"           // Rust 用户可见类型
%typemap(rsffitype)   int "c_int"         // FFI 层类型
%typemap(rsin)        int "$1 = $input;"  // Rust → C 转换
%typemap(rsout)       int "$result = $1;" // C → Rust 转换

// 指针类型
%typemap(rusttype)    T *    "*mut T"
%typemap(rusttype)    const T * "*const T"

// 字符串
%typemap(rusttype)    const char * "&CStr"
%typemap(rusttype)    char * "*mut c_char"
```

---

## 4. 生命周期处理

### 4.1 设计决策：FFI 层不处理生命周期

**原因：**
1. C++ 没有生命周期概念，无法自动推断
2. 所有主流工具（bindgen、cxx）都采用此策略
3. 这是信息丢失问题，无法解决

**实现方式：**

```rust
// FFI 层：全部原始指针，无需生命周期
mod ffi {
    extern "C" {
        fn MyClass_getName(self: *const c_void) -> *const c_char;
    }
}

// 安全包装层：返回 owned 类型
impl MyClass {
    fn get_name(&self) -> String {  // String 而非 &str
        unsafe {
            let ptr = ffi::MyClass_getName(self.ptr);
            CStr::from_ptr(ptr).to_string_lossy().into_owned()
        }
    }
}
```

**权衡：**
- 优点：简单，无需生命周期标注
- 缺点：会有额外的内存分配/拷贝
- 用户如需零拷贝，可自行通过 typemap 指定

---

## 5. Director 模式（虚函数支持）

### 5.1 桥接设计

```cpp
// 生成的 C++ 桥接类
class AnimalDirector : public Animal {
public:
    void* rust_trait_obj;  // 存储 Rust trait 对象

    void speak() override {
        if (rust_trait_obj && has_rust_override("speak")) {
            rust_speak(rust_trait_obj);  // 调用 Rust 端实现
        } else {
            Animal::speak();  // 默认：调用基类
        }
    }
};
```

```rust
// Rust 端 Director 支持
impl Animal {
    pub fn new_with_trait<T: AnimalTrait>(trait_impl: T) -> Self {
        let trait_box = Box::new(trait_impl);
        let ptr = unsafe {
            ffi::AnimalDirector_new(Box::into_raw(trait_box) as *mut c_void)
        };
        Self { ptr }
    }
}

// 用户自定义实现
struct MyDog;
impl AnimalTrait for MyDog {
    fn speak(&mut self) {
        println!("Woof!");
    }
}
```

### 5.2 纯虚函数处理

C++ 纯虚函数 = Rust trait 必须实现的方法（无默认实现）

### 5.3 回调安全性设计

#### 5.3.1 问题背景

Director 模式下，Rust 实现的回调方法可能在 C++ 调用期间被重入，导致违反 Rust 借用规则：

```rust
struct MyHandler;
impl HandlerTrait for MyHandler {
    fn on_event(&mut self, data: &mut Data) {
        // 持有 &mut self
        
        // 调用 C++ 方法，C++ 可能反过来回调另一个方法
        data.process();  // 如果 process() 回调了另一个需要 &mut self 的方法...
                         // 就会有两个同时存在的可变借用 → UB!
    }
    
    fn on_callback(&mut self) {
        // 如果在 on_event 执行期间被回调，违反借用规则
    }
}
```

**关键问题：Rust 编译器无法跨 FFI 边界进行借用检查。**

借用检查器只看当前函数内的 Rust 代码，无法追踪：
- C++ 内部会做什么
- C++ 是否会回调 Rust
- 回调时是否会产生借用冲突

#### 5.3.2 设计决策

| 设计点 | 决策 |
|--------|------|
| Director 方法签名 | 默认生成 `&self`（不是 `&mut self`） |
| 内部状态修改 | 用户提供 `Cell`/`RefCell`，SWIG 不干预 |
| 显式 `&mut self` | 提供 typemap 选项，用户自行承担风险 |
| 文档 | 生成代码中包含安全警告注释 |

#### 5.3.3 默认安全策略

Director trait 方法默认生成 `&self` 签名：

```rust
// SWIG 生成的 trait
pub trait HandlerTrait {
    fn on_event(&self, data: &mut Data);  // 默认 &self
    fn on_callback(&self);                 // 默认 &self
}
```

用户通过 `Cell`/`RefCell` 实现内部可变性：

```rust
struct MyHandler {
    count: Cell<i32>,           // Copy 类型用 Cell
    state: RefCell<Vec<i32>>,   // 非 Copy 类型用 RefCell
}

impl HandlerTrait for MyHandler {
    fn on_event(&self, data: &mut Data) {
        self.count.set(self.count.get() + 1);  // OK
        
        let mut state = self.state.borrow_mut();
        state.push(42);  // 如果重入，borrow_mut() 会 panic（安全失败）
    }
}
```

#### 5.3.4 显式可变借用

用户可通过 typemap 为特定方法指定 `&mut self`：

```swig
// 用户显式指定需要 &mut self
%typemap(rust_mut_self) Handler::reset "1"
%typemap(rust_mut_self) Handler::configure "1"
```

生成：

```rust
pub trait HandlerTrait {
    fn on_event(&self, data: &mut Data);    // 默认 &self
    fn reset(&mut self);                     // 用户指定 &mut self
    fn configure(&mut self, config: Config); // 用户指定 &mut self
}
```

**警告：使用 `&mut self` 的方法，用户需自行确保不会在执行期间被 C++ 重入。**

#### 5.3.5 安全保证总结

| 场景 | 结果 |
|------|------|
| `&self` + `Cell` | 编译期安全，无重入问题 |
| `&self` + `RefCell` | 运行时安全，重入时 panic |
| `&mut self`（用户显式） | 用户自行负责，可能 UB |

#### 5.3.6 生成代码中的警告

SWIG 在生成的 Director 代码中添加安全警告：

```rust
/// # Safety
///
/// This trait is used for C++ callbacks (Director pattern).
///
/// **Re-entrancy warning**: C++ may call back into Rust during method execution.
/// Methods use `&self` by default to allow safe re-entrancy.
///
/// - Use `Cell<T>` for simple mutable state (Copy types)
/// - Use `RefCell<T>` for complex state (panics on re-entrancy)
/// - Use `%typemap(rust_mut_self) Method "1"` for `&mut self` (unsafe if re-entered)
pub trait HandlerTrait {
    // ...
}
```

---

## 6. 命令行选项

```
-rust              启用 Rust 模块
-crate-name <name> crate 名称
-module <name>     模块名
-safe-wrapper      生成安全包装层（默认）
-ffi-only          只生成 FFI 声明
-no-directors      禁用 director 支持
```

---

## 7. 错误处理映射

| C++ 方式 | Rust 方式 |
|---------|-----------|
| 异常 | 默认 panic，可选 Result<T, E> |
| 错误码 | Result<T, ErrorCode> |
| 空指针 | Option<T> |

---

## 8. 参考实现

参考 SWIG 现有模块：
- `Source/Modules/go.cxx` - Go 模块，struct + interface 模式
- `Source/Modules/csharp.cxx` - C# 模块，多文件生成
- `Source/Modules/java.cxx` - Java 模块，JNI 机制

---

## 9. 函数重载处理

### 10.1 问题背景

C++ 支持函数重载，Rust 不支持。现有工具的方案：

| 工具 | 方案 | 缺点 |
|------|------|------|
| bindgen | 数字后缀 `func1`, `func2` | 语义不清晰 |
| autocxx | 同上 | 同上 |

### 10.2 Trait 泛化方案（SWIG 采用）

**核心思想**：为重载函数定义 trait，让不同参数类型实现该 trait。

```cpp
// C++ 重载
class Foo {
public:
    void bar(int x);
    void bar(const char* s);
    void bar(int x, int y);
};
```

生成 Rust 代码：

```rust
// 1. 定义 trait（关联类型用于支持不同返回类型）
pub trait Foo_bar {
    type Output;
    fn call(self, foo: &mut Foo) -> Self::Output;
}

// 2. 为每个重载实现 trait
impl Foo_bar for i32 {
    type Output = ();
    fn call(self, foo: &mut Foo) {
        unsafe { ffi::Foo_bar_int(foo.ptr, self) }
    }
}

impl Foo_bar for &str {
    type Output = ();
    fn call(self, foo: &mut Foo) {
        unsafe { ffi::Foo_bar_str(foo.ptr, self.as_ptr()) }
    }
}

impl Foo_bar for (i32, i32) {
    type Output = ();
    fn call(self, foo: &mut Foo) {
        unsafe { ffi::Foo_bar_int_int(foo.ptr, self.0, self.1) }
    }
}

// 3. 统一入口方法
impl Foo {
    pub fn bar<P: Foo_bar>(&mut self, p: P) -> P::Output {
        p.call(self)
    }
}
```

**调用效果**：

```rust
foo.bar(42);           // bar(int)
foo.bar("hello");      // bar(const char*)
foo.bar((1, 2));       // bar(int, int) - 多参数用元组
```

### 10.3 方案优势

| 特性 | 说明 |
|------|------|
| 类型安全 | 编译时检查参数类型 |
| 无丑陋后缀 | 不需要 `bar1`, `bar2` |
| IDE 友好 | 自动补全可推断类型 |
| 支持返回类型重载 | 通过关联类型实现 |

### 10.4 多参数重载处理

当重载函数有多个参数时，使用元组：

```cpp
void draw(int x, int y);
void draw(int x, int y, int z);
void draw(Point p);
```

```rust
impl Foo_draw for (i32, i32) { ... }
impl Foo_draw for (i32, i32, i32) { ... }
impl Foo_draw for Point { ... }

// 调用
foo.draw((10, 20));
foo.draw((10, 20, 30));
foo.draw(Point { x: 10, y: 20 });
```

---

## 10. 默认参数处理

### 11.1 问题背景

C++ 支持默认参数，Rust 不支持。现有工具都不支持此特性。

### 11.2 Builder 模式方案（SWIG 采用）

```cpp
// C++
void configure(int port, bool ssl = false, int timeout = 30);
```

生成 Rust 代码：

```rust
// 1. 参数结构体
pub struct ConfigureArgs {
    port: i32,
    ssl: bool,
    timeout: i32,
}

// 2. Builder 结构体
pub struct ConfigureArgsBuilder {
    port: Option<i32>,
    ssl: Option<bool>,
    timeout: Option<i32>,
}

impl ConfigureArgsBuilder {
    pub fn new() -> Self {
        Self {
            port: None,
            ssl: None,
            timeout: None,
        }
    }

    pub fn port(mut self, port: i32) -> Self {
        self.port = Some(port);
        self
    }

    pub fn ssl(mut self, ssl: bool) -> Self {
        self.ssl = Some(ssl);
        self
    }

    pub fn timeout(mut self, timeout: i32) -> Self {
        self.timeout = Some(timeout);
        self
    }

    pub fn build(self) -> ConfigureArgs {
        ConfigureArgs {
            port: self.port.expect("port is required"),
            ssl: self.ssl.unwrap_or(false),      // 默认值
            timeout: self.timeout.unwrap_or(30), // 默认值
        }
    }
}

// 3. 使用 Builder 参数的方法
impl Foo {
    pub fn configure(&mut self, args: ConfigureArgs) {
        unsafe { ffi::Foo_configure(self.ptr, args.port, args.ssl, args.timeout) }
    }
}
```

**调用效果**：

```rust
// 使用默认值
foo.configure(ConfigureArgsBuilder::new().port(8080).build());

// 指定部分可选参数
foo.configure(ConfigureArgsBuilder::new()
    .port(8080)
    .ssl(true)
    .build());

// 指定所有参数
foo.configure(ConfigureArgsBuilder::new()
    .port(8080)
    .ssl(true)
    .timeout(60)
    .build());
```

### 11.3 简化调用语法（可选）

对于简单情况，提供便捷构造函数：

```rust
impl ConfigureArgs {
    // 必填参数的便捷构造
    pub fn new(port: i32) -> Self {
        Self { port, ssl: false, timeout: 30 }
    }
}

// 调用
foo.configure(ConfigureArgs::new(8080));
```

---

## 11. Rust 版本兼容性

### 12.1 设计原则

**所有生成的 Rust 代码必须兼容 Rust 1.0**

这是最强的兼容性保证，确保：
- 用户无需升级 Rust 版本
- 无外部 crate 依赖
- 最大程度的可移植性

### 12.2 使用的特性及版本支持

| 特性 | 最低版本 | 用途 |
|------|---------|------|
| 关联类型 `type Output` | 1.0 | Trait 泛化重载 |
| 泛型 trait bound `P: Trait` | 1.0 | 统一入口函数 |
| `?Sized` bound | 1.0 | 灵活泛型参数 |
| `Option<T>` | 1.0 | Builder 默认值处理 |
| `unwrap_or()` | 1.0 | 默认值回退 |
| `struct` + `impl` | 1.0 | 类型定义 |

### 12.3 禁止使用的特性

以下特性在 1.0 之后添加，生成的代码不得使用：

| 特性 | 引入版本 | 替代方案 |
|------|---------|---------|
| `impl Trait` 返回类型 | 1.26 | 具体类型 |
| `dyn Trait` | 1.27 | `&Trait` (1.0 语法) |
| `?` 操作符 | 1.22 | `try!` 宏或 match |
| `const fn` | 1.46 | 普通 fn |
| 切片模式 | 1.42 | match 分支 |

### 12.4 外部依赖策略

生成的代码 **不依赖** 任何外部 crate：

- 不使用 `derive_builder`：手写 Builder 代码
- 不使用 `thiserror`：手写 Error impl
- 不使用 `serde`：用户自行添加

用户可在自己的 `Cargo.toml` 中添加所需依赖。

---

## 12. 输出文件结构

### 13.1 单文件模式

```
xxx_wrap.cxx    → C++ extern "C" 包装函数
xxx_wrap.h      → C 头文件（director 需要）
xxx.rs          → Rust FFI 声明 + 高层包装
Cargo.toml      → 可选生成
```

### 13.2 多文件模式（启用命名空间映射时）

```
output/
├── lib.rs              # 根模块
├── namespace1.rs       # 或 namespace1/mod.rs
├── namespace1/
│   └── nested.rs
└── ...
```

---

## 13. 命名空间映射设计

### 14.1 问题背景

C++ 命名空间和 Rust mod 有根本性差异：

| 特性 | C++ 命名空间 | Rust mod |
|------|-------------|----------|
| 定义方式 | 可分散定义（多处、多文件） | 必须集中定义 |
| 扩展性 | 可随时添加新成员 | 模块树固定 |
| 文件对应 | 与文件无关 | 一个文件 = 一个模块（或子模块声明） |
| 匿名 | 支持匿名命名空间 | 无等价概念 |
| 别名 | 支持 `namespace A = B::C` | 使用 `use` 重导出 |

**核心矛盾**：C++ 命名空间可以分散定义，但 Rust mod 必须集中在一个地方。

```cpp
// C++: 合法 - 命名空间可以分散定义
// file1.cpp
namespace math {
    struct Vector { ... };
}

// file2.cpp
namespace math {
    struct Matrix { ... };  // 扩展 math 命名空间
}

// file3.cpp
namespace math {
    namespace linear {
        struct Transform { ... };
    }
}
```

```rust
// Rust: 一个文件不能多次定义同一个 mod
mod math {
    struct Vector { ... }
}
mod math {  // 错误！重复定义
    struct Matrix { ... }
}
```

### 14.2 设计方案

#### 方案概述：合并生成 + 嵌套模块

**核心策略**：SWIG 在代码生成阶段合并所有同名命名空间的定义，生成单一的 Rust 模块。

```
C++ 分散定义                  Rust 合并生成
─────────────────            ─────────────────
file1: ns::ClassA      ─┐
file2: ns::ClassB      ─┼──>  ns.rs (或 ns/mod.rs)
file3: ns::ClassC      ─┘        包含 ClassA, ClassB, ClassC
```

#### 14.2.1 单一输入文件场景

```cpp
// input.i
namespace graphics {
    struct Point { ... };
    struct Color { ... };
    
    namespace shapes {
        struct Circle { ... };
        struct Rectangle { ... };
    }
}
```

生成文件结构（推荐 `mod.rs` 风格）：

```
output/
├── lib.rs              # 根模块
│   pub mod graphics;
│
└── graphics/
    ├── mod.rs          # graphics 命名空间
    │   pub struct Point { ... }
    │   pub struct Color { ... }
    │   pub mod shapes;
    │
    └── shapes/
        └── mod.rs      # graphics::shapes 命名空间
            pub struct Circle { ... }
            pub struct Rectangle { ... }
```

或使用扁平文件风格（简单项目）：

```
output/
├── lib.rs              # 根模块
│   pub mod graphics;
│
├── graphics.rs         # graphics 命名空间
│   pub struct Point { ... }
│   pub struct Color { ... }
│   pub mod shapes;
│
└── graphics/
    └── shapes.rs       # graphics::shapes 命名空间
        pub struct Circle { ... }
        pub struct Rectangle { ... }
```

#### 14.2.2 多输入文件场景

**场景 A：同一命名空间在不同文件中定义**

```cpp
// math_vector.i
namespace math {
    struct Vector { ... };
}

// math_matrix.i
namespace math {
    struct Matrix { ... };
}
```

**策略 1：合并输出（推荐）**

所有同名命名空间的类型合并到一个文件：

```rust
// math.rs - 合并 math_vector.i 和 math_matrix.i 的内容
pub struct Vector { ... }
pub struct Matrix { ... }
```

需要 SWIG 在生成时收集所有命名空间定义，然后合并输出。

**策略 2：分文件输出 + 重导出**

```rust
// math_vector.rs
pub struct Vector { ... }

// math_matrix.rs
pub struct Matrix { ... }

// math.rs - 重导出模块
mod math_vector;
mod math_matrix;

pub use math_vector::*;
pub use math_matrix::*;
```

**场景 B：不同命名空间在不同文件中**

```cpp
// graphics.i
namespace graphics { ... }

// audio.i
namespace audio { ... }
```

直接生成独立模块：

```
output/
├── lib.rs
│   pub mod graphics;
│   pub mod audio;
├── graphics.rs
└── audio.rs
```

#### 14.2.3 匿名命名空间处理

C++ 匿名命名空间提供文件级私有作用域：

```cpp
namespace {
    int internal_state;  // 仅本文件可见
}
```

**映射策略**：生成私有模块，不对外导出

```rust
// 生成私有模块
mod __anon__ {
    pub static mut INTERNAL_STATE: i32 = 0;
}

// 使用时通过私有模块访问
// 注意：Rust 没有 C++ 意义上的"文件私有"，使用 mod 私有性
```

#### 14.2.4 命名空间别名处理

```cpp
namespace A = B::C;
A::SomeType x;  // 实际是 B::C::SomeType
```

**映射策略**：在生成代码时展开别名

```rust
// 不生成别名模块，直接使用完整路径
use b::c::SomeType;

// 或生成重导出（可选）
pub mod a {
    pub use super::b::c::*;
}
```

### 14.3 实现设计

#### 14.3.1 数据结构

```cpp
// 命名空间树节点
struct NamespaceNode {
    String *name;              // 命名空间名称
    List *types;               // 该命名空间中的类型定义
    List *functions;           // 该命名空间中的函数
    Hash *children;            // 子命名空间
    String *output_file;       // 输出文件路径
};

// 全局命名空间收集器
NamespaceNode *global_namespace_tree;
```

#### 14.3.2 处理流程

```
Phase 1: 收集（Parse 阶段）
───────────────────────────
for each declaration in SWIG AST:
    namespace = get_namespace(declaration)
    add_to_namespace_tree(namespace, declaration)

Phase 2: 生成（Emit 阶段）
───────────────────────────
for each namespace in tree (depth-first):
    1. 确定输出文件路径
    2. 生成模块声明（pub mod name;）
    3. 生成类型/函数定义
    4. 递归处理子命名空间
```

#### 14.3.3 命令行选项

```
-namespace                启用命名空间映射（默认）
-namespace-flat           扁平化模式（生成前缀名，无模块）
-namespace-output <dir>   指定模块输出目录
-namespace-style <style>  文件风格：mod（默认）或 flat
```

### 14.4 边界情况处理

#### 14.4.1 全局命名空间

C++ 全局作用域映射到 Rust 根模块（`lib.rs`）：

```cpp
// 全局函数和类型
void global_func();
struct GlobalStruct { ... };
```

```rust
// lib.rs
pub fn global_func() { ... }
pub struct GlobalStruct { ... }
```

#### 14.4.2 命名冲突

不同命名空间的同名类型在 Rust 中无冲突：

```cpp
namespace A { struct Point { ... }; }
namespace B { struct Point { ... }; }
```

```rust
// a.rs
pub struct Point { ... }  // a::Point

// b.rs
pub struct Point { ... }  // b::Point

// 用户使用时
use a::Point as APoint;
use b::Point as BPoint;
```

#### 14.4.3 跨模块引用

SWIG 多模块项目中，一个模块可能使用另一个模块的类型：

```cpp
// module1.i
namespace core { struct Object { ... }; }

// module2.i
%import "module1.i"
namespace app {
    core::Object* create();  // 使用 module1 的类型
}
```

**处理策略**：生成 `use` 声明引用外部模块

```rust
// module2/app.rs
use crate::module1::core::Object;  // 引用外部模块

pub fn create() -> *mut Object { ... }
```

### 14.5 与其他工具对比

| 工具 | 命名空间处理 |
|------|-------------|
| bindgen | 生成扁平代码，使用前缀 |
| cxx | 用户手动定义模块结构 |
| autocxx | 类似 bindgen |
| **SWIG-Rust** | **自动合并生成嵌套模块** |

### 14.6 设计决策总结

| 设计点 | 决策 |
|--------|------|
| 命名空间 → mod | 是，生成嵌套模块 |
| 分散定义处理 | 合并到单一模块文件 |
| 输出风格 | 默认 `mod.rs` 风格 |
| 匿名命名空间 | 生成私有模块 |
| 命名空间别名 | 展开为完整路径 |
| 全局作用域 | 放入根模块 `lib.rs` |

### 14.7 当前实现限制 (2026-04-14)

#### 14.7.1 核心限制：Rust 不允许重复定义同一个 `mod`

**问题描述**：

C++ 允许同一个命名空间在多处定义：
```cpp
// 合法的 C++ 代码
namespace math {
    struct Vector { ... };
}

namespace math {  // 扩展 math 命名空间
    struct Matrix { ... };
}
```

但 Rust 不允许：
```rust
// 错误的 Rust 代码
pub mod math {
    pub struct Vector { ... }
}
pub mod math {  // 错误！重复定义
    pub struct Matrix { ... }
}
```

#### 14.7.2 当前解决方案

**方案 1：使用 `-namespace` 命令行选项**

将所有内容包装到一个统一的外层模块中：

```bash
swig -rust -c++ -namespace mylib -module mymodule input.i
```

生成：
```rust
pub mod mylib {
    pub struct Vector { ... }
    pub struct Matrix { ... }
    // 所有类都在这里
}
```

**方案 2：使用 `%rename` 添加前缀**

手动避免命名冲突：

```swig
%rename(Math_Vector) math::Vector;
%rename(Math_Matrix) math::Matrix;

namespace math {
    struct Vector { ... };
    struct Matrix { ... };
}
```

**方案 3：分开处理不同命名空间**

每个命名空间使用独立的 SWIG 运行：

```bash
swig -rust -c++ -module math_vector math_vector.i
swig -rust -c++ -module math_matrix math_matrix.i
```

然后在 Rust 端手动组合：

```rust
// lib.rs
pub mod math_vector;
pub mod math_matrix;

pub use math_vector::*;
pub use math_matrix::*;
```

#### 14.7.3 默认配置

为避免生成无效 Rust 代码，`%feature("nspace")` **默认未启用**。

如果用户手动启用 `%feature("nspace")`，每个类会尝试生成命名空间包装，可能导致重复 `mod` 定义错误。

#### 14.7.4 未来改进方向

完整的命名空间支持需要架构级改进：

1. **收集阶段**：遍历 AST 收集所有命名空间及其内容
2. **组织阶段**：构建命名空间树，合并同名命名空间
3. **生成阶段**：每个命名空间生成一个 `pub mod` 块

```cpp
// 需要添加的数据结构
Hash *namespace_content;  // 命名空间 -> 内容缓冲区
Hash *opened_namespaces;  // 跟踪已打开的命名空间
```

这需要对 `emitRustFile()` 进行较大重构。

---

## 14. 与其他 Rust 绑定工具对比

| 工具 | 生命周期处理 | Director 支持 | 函数重载 | 默认参数 | Rust 版本 |
|------|-------------|--------------|---------|---------|----------|
| bindgen | 原始指针 | 无 | 数字后缀 | 不支持 | 1.60+ |
| cxx | 手动标注 | 无 | 不支持 | 不支持 | 1.48+ |
| autocxx | 原始指针 | 无 | 数字后缀 | 计划中 | 1.51+ |
| **SWIG-Rust** | 原始指针 | 有 | **Trait 泛化** | **Builder** | **1.0+** |

SWIG 的优势：
- 复用 SWIG 强大的类型系统和 typemap
- 支持 Director 模式
- 与现有 SWIG 项目无缝集成
- 最强的 Rust 版本兼容性（1.0+）
- 更优雅的重载处理方案
