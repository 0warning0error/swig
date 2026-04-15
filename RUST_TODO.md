# SWIG Rust 绑定实现计划

> 注意：这是 Rust 绑定功能的开发计划，与项目根目录的 TODO 文件无关。

---

## Phase 1: 基础框架搭建 ✅

### 1.1 创建 Rust 语言模块
- [x] 创建 `Source/Modules/rust.cxx`
  - [x] 定义 `RUST` 类继承 `Language`
  - [x] 实现构造函数/析构函数
  - [x] 实现 `main()` 处理命令行参数
  - [x] 实现 `top()` 初始化输出文件

### 1.2 注册语言模块
- [x] 修改 `Source/Modules/swigmain.cxx`
  - [x] 声明 `swig_rust()` 工厂函数
  - [x] 添加到 `modules[]` 注册表

### 1.3 创建 Lib/rust 目录结构
- [x] 创建 `Lib/rust/rust.swg` - 主配置文件
- [x] 创建 `Lib/rust/rusttype.swg` - 基本类型映射
- [x] 创建 `Lib/rust/rustrun.swg` - 运行时支持

### 1.4 更新构建系统
- [x] 确认 CMakeLists.txt 自动包含（glob）
- [x] 更新 Source/Makefile.am

---

## Phase 2: 类型映射系统 ✅

### 2.1 基本类型映射
- [x] 整数类型 (int, long, short, char)
- [x] 浮点类型 (float, double)
- [x] 布尔类型 (bool)
- [x] size_t, ptrdiff_t

### 2.2 指针类型映射
- [x] 原始指针 `T*` → `*mut T`
- [x] const 指针 `const T*` → `*const T`
- [x] void 指针 `void*` → `*mut c_void`

### 2.3 字符串类型映射
- [x] `const char*` → `&CStr` / `String`
- [x] `char*` → `*mut c_char`
- [x] `std::string` → 专门的SwigString类型映射C++的字符串，保留双向转换成Rust的String类型（实现From和Into Trait）。
### 2.4 Typemap 实现
- [x] 实现 `rusttype` typemap
- [x] 实现 `rsffitype` typemap
- [x] 实现 `rsin` typemap
- [x] 实现 `rsout` typemap

---

## Phase 3: 函数包装生成 ✅

### 3.1 全局函数
- [x] 解析函数签名
- [x] 生成 C++ extern "C" 包装
- [x] 生成 Rust FFI 声明
- [x] 生成 Rust 安全包装

### 3.2 函数参数处理
- [x] 输入参数转换
- [x] 输出参数处理
- [x] 返回值处理

### 3.3 重载函数
- [x] Rust 端重命名处理（类型后缀方案）
- [x] 生成不同签名的包装函数
- [x] 添加 `emitOverloadSuffix()` 辅助方法

---

## Phase 4: 类处理 ✅

### 4.1 类定义处理
- [x] 实现 `classHandler()`
- [x] 生成 Rust struct（持有指针）
- [x] 生成 Rust trait（方法声明）
- [x] 生成 impl Trait for struct

### 4.2 构造函数处理
- [x] 实现 `constructorHandler()`
- [x] 生成 `fn new() -> Self`
- [x] 处理重载构造函数

### 4.3 析构函数处理
- [x] 实现 `destructorHandler()`
- [x] 生成 `impl Drop`
- [x] 确保资源正确释放

### 4.4 成员函数处理
- [x] const 方法 → `&self`
- [x] 非 const 方法 → `&mut self`
- [x] 静态方法 → 关联函数

### 4.5 成员变量处理
- [x] 实现 `variableHandler()`
- [x] 生成 getter/setter

---

## Phase 5: 继承处理 ✅

### 5.1 单继承
- [x] 解析基类信息
- [x] 生成 trait 继承关系
- [x] 实现向上转型

### 5.2 多继承
- [x] 第一个基类使用原始指针
- [x] 其他基类使用转换函数
- [x] 生成 `SwigGetBaseX()` 方法

---

## Phase 6: Director 支持 ✅

### 6.1 Director 框架
- [x] 实现 `classDirectorInit()`
- [x] 实现 `classDirectorEnd()`
- [x] 生成 Director struct

### 6.2 Director 构造函数
- [x] 实现 `classDirectorConstructor()`
- [x] 生成 `new_with_trait()` 函数
- [x] 处理 trait 对象存储

### 6.3 Director 方法
- [x] 实现 `classDirectorMethod()`
- [x] 生成 C++ 桥接类
- [x] 实现虚函数转发

### 6.4 C++ Director 类生成
- [x] 生成 `SwigDirector_XXX` 类
- [x] 实现虚函数重写
- [x] 调用 Rust 端或基类

---

## Phase 7: 枚举和常量 ✅

### 7.1 枚举处理
- [x] 实现 `enumDeclaration()`
- [x] 生成 Rust enum（`#[repr(C)]`）
- [x] 处理枚举值

### 7.2 常量处理
- [x] 实现 `constantWrapper()`
- [x] 生成 Rust const 声明

---

## Phase 8: 高级特性

### 8.1 模板支持 ✅
- [x] 模板类实例化 (SWIG 核心支持 `%template`)
- [x] 模板函数

### 8.2 智能指针 ✅
- [x] `std::shared_ptr` 映射 (`Lib/rust/std_shared_ptr.i`)
- [x] `std::unique_ptr` 映射 (`Lib/rust/std_unique_ptr.i`)

### 8.3 异常处理 ✅
- [x] C++ 异常捕获 (`Lib/rust/exception.i`)
- [x] 映射到 Rust 错误码和 panic

### 8.4 命名空间 ✅
- [x] 映射到 Rust mod
- [x] `-namespace` 命令行选项
- [x] 处理命名冲突

### 8.5 函数重载优化 ✅ (2026-04-11)
- [x] Trait 泛化方案（见 RUST_DESIGN.md 第10章）
  - [x] 添加 `-trait-overload` 命令行选项
  - [x] 实现 `buildOverloadInfo()` 构建重载信息
  - [x] 实现 `emitOverloadTraits()` 生成重载 trait 定义
  - [x] 实现 `emitOverloadTraitImpls()` 为每个重载生成 trait 实现
  - [x] 实现 `emitOverloadEntryPoints()` 生成统一入口方法
  - [x] 测试通过：单参数和多参数重载
- [ ] 默认参数 Builder 模式（见 RUST_DESIGN.md 第11章）

---

## Phase 9: 测试和文档

### 9.1 单元测试
- [x] 基本类型测试
- [x] 类包装测试
- [x] 继承测试
- [x] Director 测试
- [x] 枚举类型测试（2026-04-12 修复）

### 9.2 集成测试 ✅ (2026-04-12)
- [x] 创建 `Examples/rust/` 目录
- [x] 编写示例代码
- [x] 添加到 test-suite（`Examples/test-suite/rust/`）
- [x] 测试运行脚本 `run_tests.py`
- [x] Rust 特定测试 (11个)
- [x] 通用测试 (5个)
- [x] **总计 16 个测试全部通过**

### 9.3 文档
- [ ] 编写 Doc/Manual/Rust.html
- [ ] README 更新

---

## Phase 10: Bug 修复 (2026-04-11)

### 10.1 枚举类型处理修复 ✅
- [x] 跳过匿名枚举或生成常量
- [x] 跳过空枚举
- [x] 修复返回类型中的 `enum` 关键字
- [x] 添加 `cleanTypeName()` 函数

### 10.2 重载函数生成修复 ✅
- [x] 重载构造函数添加类型后缀
- [x] 重载全局函数添加类型后缀
- [x] 改进 `emitOverloadSuffix()`

### 10.3 类型映射修复 ✅
- [x] 枚举类型映射使用 `SWIGENUM` 标记
- [x] 在 `functionWrapper()` 中正确处理枚举返回类型
- [x] 添加 `processRustType()` 函数

---

## Phase 11: Director 完整实现 (2026-04-12) ✅

### 11.1 Rust 回调函数实现 ✅
- [x] 在 `classDirectorMethod()` 中生成回调函数
- [x] 处理 void 返回类型
- [x] 处理有返回值类型（Option<T>）
- [x] 处理参数类型转换
- [x] 添加 `director_rust_callbacks` 缓冲区

### 11.2 Director Drop 函数 ✅
- [x] 生成 `SwigDirector_XXX_drop_director()` Rust 函数
- [x] 修改 C++ 析构函数调用 drop 函数
- [x] 生成 FFI 声明

### 11.3 胖指针问题修复 ✅
- [x] 识别 trait 对象是胖指针（16字节）
- [x] 使用 `Box<Box<dyn Trait>>` 方案传递给 C++
- [x] 正确恢复 trait 对象引用
- [x] 正确释放内存

### 11.4 两种 Director 方案 ✅
- [x] 添加 `-director-thin` 命令行选项（默认）
- [x] 添加 `-director-boxed` 命令行选项
- [x] Thin vtable 方案：手动虚表，单次解引用
- [x] Boxed 方案：`Box<Box<dyn Trait>>`，双重解引用

---

## Phase 12: 成员变量问题修复 (2026-04-12) ✅

### 12.1 问题描述

对于以下 C++ 结构体：
```cpp
struct PODType {
    int a;
    char b;
};
```

**问题**：Rust 生成的成员变量 getter/setter 没有正确处理 `self` 参数。

### 12.2 问题根源

`variableHandler()` 没有调用基类的 `Language::variableHandler()`，导致 SWIG 核心的成员变量处理流程未被触发。

### 12.3 修复方案 ✅

修改 `rust.cxx` 中的 `variableHandler()` 和相关方法，正确调用基类处理。

### 12.4 修复结果 ✅

生成的 Rust 代码现在正确包含：
- FFI 函数带有 `self` 指针参数
- 安全包装方法：`pub fn field(&self) -> T` 和 `pub fn set_field(&self, v: T)`
- 正确的成员访问逻辑

---

## Phase 13: Director VTable 问题修复 (2026-04-12) ✅

### 13.1 问题概述

用户测试 Director 功能时发现代码生成问题：

1. **E0412: 缺少类型导入** - 已修复 ✅
2. **E0401: 静态 VTABLE 不能使用泛型参数** - 不适用（当前使用 `Box<dyn Trait>`）
3. **E0053: Trait 方法签名不匹配** - 已修复 ✅

### 13.2 本次修复内容 ✅

1. **rust.cxx 语法错误** ✅
2. **emitRustTrait 参数处理** ✅
3. **const 方法检测** ✅
4. **emitRustConstructor 参数传递** ✅

---

## Phase 14: 枚举类型处理问题 ✅ (2026-04-12 已修复)

### 14.1 问题描述

对于以下 C++ 代码：
```cpp
enum Color { RED, GREEN, BLUE };

class EnumClass {
public:
    enum Status { OK, ERROR, PENDING };
    Status get_status() const;
};
```

**问题**：
1. 全局枚举 `Color`, `Size` 没有生成 Rust enum 定义
2. 类内枚举 `EnumClass::Status` 使用了 `EnumClass::Status` 路径形式（Rust 不支持）

### 14.2 修复内容 ✅

- [x] 发现根本原因：SWIG 枚举值子节点的 `nodeType` 是 `"enumitem"` 而不是 `"enumvalue"`
- [x] 修改 `enumDeclaration()` 使用正确的节点类型
- [x] 不调用 `Language::enumDeclaration(n)`，避免触发错误的 `constantWrapper`
- [x] 为类内枚举生成独立名称（如 `EnumClass_Status`）
- [x] 修改 `cleanTypeName()` 处理 C++ 作用域分隔符 `::`
- [x] 所有 16 个测试用例通过

---

## Phase 15: 关联常量 VTable 方案 ✅ (2026-04-13 完成)

### 15.1 设计方案

利用 Rust 1.20+ 的关联常量特性：

```rust
trait ATraitVTableProvider : ATrait + Sized {
    const VTABLE: ATraitVTable = ATraitVTable {
        func: vfunc_int_thunk::<Self>
    };
}

impl<T: ATrait + Sized> ATraitVTableProvider for T {}

fn get_vtable<T: ATrait>() -> &'static ATraitVTable {
    &<T as ATraitVTableProvider>::VTABLE
}
```

**优点**:
- 零开销抽象
- 不需要运行时装箱
- Rust 1.20+ 支持

### 15.2 已实现项

- [x] 添加 `-director-vtable` 命令行选项
- [x] 生成 VTable 结构体定义（Rust 和 C++ 两端）
- [x] 生成 thunk 函数（泛型函数，调用 trait 方法）
- [x] 生成 VTableProvider trait 和 blanket impl
- [x] 生成 `new_with_vtable` 构造函数
- [x] 修改 C++ Director 类存储 VTable 指针
- [x] C++ 端通过 VTable 字段名调用回调函数
- [x] 支持纯虚函数（在 VTable 模式下生成实现）
- [x] 测试用例验证 (`test_vtable_director.i`)

---

## 开发优先级

1. **P0 (必须)**: Phase 1-4 ✅
2. **P1 (重要)**: Phase 5-6 ✅
3. **P2 (需要)**: Phase 7 ✅
4. **P3 (增强)**: Phase 8 ✅
5. **P4 (完善)**: Phase 9 测试和文档（部分待实现）
6. **P5 (优化)**: Phase 10 Bug 修复 ✅
7. **P6 (Director)**: Phase 11 Director 完整实现 ✅
8. **P7 (成员变量)**: Phase 12 成员变量问题修复 ✅
9. **P8 (枚举)**: Phase 14 枚举类型处理修复 ✅
10. **P9 (VTable)**: Phase 15 关联常量 VTable 方案 ✅

---

## 当前状态

**状态**: Phase 1-15 全部完成 ✅

**已完成**:
- Phase 1-12 全部完成
- Phase 13 Director VTable 问题修复完成
- Phase 14 枚举类型处理问题修复完成
- Phase 15 关联常量 VTable 方案完成 ✅

**测试结果**: 16 个测试全部通过

**下一步**: 
1. 编写文档 `Doc/Manual/Rust.html`
2. 添加更多 VTable 模式的测试用例

---

## Phase 16: C++ 特殊成员函数到 Rust Trait 映射 ✅ (2026-04-13 完成)

### 16.1 设计目标

将 C++ 的特殊成员函数自动映射到 Rust 的标准 trait，使生成的 Rust 类型更符合 Rust 惯用语法。

### 16.2 运算符重载映射 ✅

已实现的运算符映射：
- `operator+` → `std::ops::Add`
- `operator-` (二元) → `std::ops::Sub`
- `operator-` (一元) → `std::ops::Neg`
- `operator*` → `std::ops::Mul`
- `operator/` → `std::ops::Div`
- `operator==` → `std::cmp::PartialEq`
- `operator<` → `std::cmp::PartialOrd`
- `operator[]` → `std::ops::Index`
- `operator+=` → `std::ops::AddAssign`
- `operator-=`, `operator*=`, `operator/=` 同理

### 16.5 实现计划 ✅

- [x] **Phase 16.1**: 基础架构 ✅
  - [x] 添加运算符检测函数 `isOperatorMethod(Node *n)`
  - [x] 添加 `getOperatorKindFromRustName()` 检测重命名后的运算符
  - [x] 添加 `isRenamedOperatorMethod()` 检测重命名后的方法名

- [x] **Phase 16.2**: Trait 实现 ✅
  - [x] 实现 `emitOperatorTraitImpls()` 函数
  - [x] 生成 `impl Add/Sub/Mul/Div` 等 trait
  - [x] 生成 `impl PartialEq/PartialOrd` trait
  - [x] 生成 `impl Index` trait
  - [x] 在 `classHandler()` 中调用运算符 trait 生成

- [ ] **Phase 16.2**: 算术运算符
  - [ ] 实现 `Add`, `Sub`, `Mul`, `Div`, `Rem` trait 生成
  - [ ] 处理返回值类型（Self vs 新类型）
  - [ ] 处理左右操作数类型不同的情况

- [ ] **Phase 16.3**: 比较运算符
  - [ ] 实现 `PartialEq` trait 生成
  - [ ] 实现 `PartialOrd` trait 生成
  - [ ] 考虑 `Eq` 和 `Ord`（需要全序关系）

- [ ] **Phase 16.4**: 位运算符
  - [ ] 实现 `BitAnd`, `BitOr`, `BitXor`, `Not`
  - [ ] 实现 `Shl`, `Shr`

- [ ] **Phase 16.5**: 索引和调用
  - [ ] 实现 `Index`, `IndexMut`
  - [ ] 评估 `Fn` traits 可行性（可能需要替代方案）

- [ ] **Phase 16.6**: Copy/Clone/Default
  - [ ] 实现 `Clone` trait 生成（调用拷贝构造函数）
  - [ ] 添加 `%feature("rust:copy")` 支持
  - [ ] 实现 `Default` trait（无参构造函数）

- [ ] **Phase 16.7**: 类型转换
  - [ ] 实现 `From` trait 生成
  - [ ] 处理 `operator T()` 转换运算符

### 16.6 示例

**C++ 代码**:
```cpp
class Vector3 {
public:
    Vector3 operator+(const Vector3& rhs) const;
    Vector3 operator-(const Vector3& rhs) const;
    bool operator==(const Vector3& rhs) const;
    float operator[](int index) const;
    float& operator[](int index);
};
```

**期望生成的 Rust 代码**:
```rust
impl Add for Vector3 {
    type Output = Vector3;
    fn add(self, rhs: Vector3) -> Vector3 {
        unsafe { ffi::Vector3_add(self.ptr, rhs.ptr) }
    }
}

impl PartialEq for Vector3 {
    fn eq(&self, other: &Self) -> bool {
        unsafe { ffi::Vector3_eq(self.ptr, other.ptr) }
    }
}

impl Index<i32> for Vector3 {
    type Output = f32;
    fn index(&self, index: i32) -> &f32 {
        // 需要特殊处理，因为返回引用
    }
}
```

### 16.7 注意事项

1. **所有权问题**: `Add::add(self, rhs)` 消耗 self，与 C++ 的 const 方法语义不同
2. **返回引用**: `Index` 返回引用，但 FFI 不能安全返回引用到 C++ 内部数据
3. **生命周期**: 比较运算符需要生命周期标注
4. **泛型运算符**: C++ 模板运算符 vs Rust 泛型 trait 实现

---

## Phase 17: STL 容器类型绑定 (优先实现) 🔄

### 17.1 现状对比

| STL 类型 | C# | Go | Rust | 优先级 |
|---------|-----|-----|------|-------|
| `std::vector<T>` | ✅ | ✅ | ❌ | **P0** |
| `std::string` | ✅ | ✅ | ❌ | **P0** |
| `std::map<K,V>` | ✅ | ✅ | ❌ | **P1** |
| `std::set<T>` | ✅ | ❌ | ❌ | **P1** |
| `std::pair<T,U>` | ✅ | ✅ | ❌ | **P1** |
| `std::deque<T>` | ✅ | ✅ | ❌ | P2 |
| `std::list<T>` | ✅ | ✅ | ❌ | P2 |
| `std::unordered_map<K,V>` | ✅ | ❌ | ❌ | P2 |
| `std::unordered_set<T>` | ✅ | ❌ | ❌ | P2 |
| `std::array<T,N>` | ✅ | ✅ | ❌ | P2 |
| `std::complex<T>` | ✅ | ❌ | ❌ | P3 |
| `std::string_view` | ✅ | ❌ | ❌ | P3 |
| `std::wstring` | ✅ | ❌ | ❌ | P3 |
| `std::shared_ptr<T>` | ✅ | ❌ | ✅ | - |
| `std::unique_ptr<T>` | ✅ | ❌ | ✅ | - |

### 17.2 设计原则

1. **Rust 惯用映射**: STL 容器映射到 Rust 标准库或常用 crate
2. **零拷贝优先**: 尽可能避免数据拷贝
3. **迭代器支持**: 生成 Rust Iterator trait 实现
4. **所有权清晰**: 明确所有权语义

### 17.3 映射方案

| C++ STL | Rust 类型 | 说明 |
|---------|----------|------|
| `std::vector<T>` | `Vec<T>` | 直接映射，需生成转换方法 |
| `std::string` | `String` | 直接映射，UTF-8 兼容 |
| `std::map<K,V>` | `std::collections::HashMap<K,V>` 或 BTreeMap | 有序性决定选择 |
| `std::set<T>` | `std::collections::HashSet<T>` 或 BTreeSet | 有序性决定选择 |
| `std::pair<T,U>` | `(T, U)` | 映射到 Rust 元组 |
| `std::deque<T>` | `std::collections::VecDeque<T>` | 双端队列 |
| `std::list<T>` | 自定义或 `std::collections::LinkedList<T>` | 链表较少使用 |
| `std::unordered_map<K,V>` | `std::collections::HashMap<K,V>` | 无序映射 |
| `std::unordered_set<T>` | `std::collections::HashSet<T>` | 无序集合 |

### 17.4 实现计划

#### Phase 17.1: std::string 绑定 (最基础) ✅

**文件**: `Lib/rust/std_string.i`

- [x] 创建 `Lib/rust/std_string.i`
- [ ] 实现 `std::string` → `SwigString` 专门类型的映射，同时支持双向转换成String
- [x] 实现 `const std::string&` → `&SwigString` 映射（但最好还是用指针，rust的生命周期很难搞）
- [x] 处理异常（空字符串）
- [x] 添加测试用例 (`std_string_test.i`)

#### Phase 17.2: std::vector 绑定 (最常用) ✅

**文件**: `Lib/rust/std_vector.i`

- [x] 创建 `Lib/rust/std_vector.i`
- [x] 实现基本方法（size, empty, push_back, clear）
- [x] 实现 Index/IndexMut trait（通过 getitem/setitem）
- [x] 实现 Iterator trait（通过 IntoIterator）
- [x] 实现 From/Into 转换（Vec<T> ↔ std::vector<T>）
- [x] 添加测试用例 (`std_vector_test.i`)
- [ ] 实现 Iterator trait
- [ ] 实现 From/Into 转换（Vec<T> ↔ std::vector<T>）
- [ ] 添加测试用例

#### Phase 17.3: std::pair 绑定 ✅

**文件**: `Lib/rust/std_pair.i`

- [x] 创建 `Lib/rust/std_pair.i`
- [x] 映射到 Rust 元组 `(T, U)`
- [x] 实现 first/second 访问（通过 .0/.1）
- [x] 添加 From/Into 转换

#### Phase 17.4: std::map 绑定 ✅

**文件**: `Lib/rust/std_map.i`

- [x] 创建 `Lib/rust/std_map.i`
- [x] 使用 BTreeMap（有序）映射 std::map
- [x] 使用 HashMap 映射 std::unordered_map
- [x] 实现 Index/IndexMut trait（通过 getitem/setitem）
- [x] 实现迭代器（通过 IntoIterator）
- [x] 实现 keys(), values(), entries() 方法
- [x] 添加测试用例 (`std_map_test.i`)

#### Phase 17.5: std::set 绑定 ✅

- [x] 创建 `Lib/rust/std_set.i`
- [x] 映射到 BTreeSet（有序）
- [x] 实现插入、删除、查找
- [x] 添加测试用例

#### Phase 17.6: 其他容器

- [ ] `std::deque<T>` → `VecDeque<T>`
- [ ] `std::list<T>` → `LinkedList<T>` 或自定义
- [ ] `std::unordered_map<K,V>` → `HashMap<K,V>`
- [ ] `std::unordered_set<T>` → `HashSet<T>`
- [ ] `std::array<T,N>` → `[T; N]`

### 17.5 参考实现

- C# 实现: `Lib/csharp/std_*.i` (18个文件)
- Go 实现: `Lib/go/std_*.i` (9个文件)
- 已有 Rust 实现: `Lib/rust/std_shared_ptr.i`, `Lib/rust/std_unique_ptr.i`

### 17.6 测试计划

- [x] 创建 `Examples/test-suite/rust/std_string_test.i`
- [x] 创建 `Examples/test-suite/rust/std_vector_test.i`
- [x] 创建 `Examples/test-suite/rust/std_map_test.i`
- [ ] 创建 `Examples/test-suite/rust/std_pair_test.i` (可选)

---

## 开发优先级（更新）

1. **P0 (必须)**: Phase 1-4 ✅
2. **P1 (重要)**: Phase 5-6 ✅
3. **P2 (需要)**: Phase 7 ✅
4. **P3 (增强)**: Phase 8 ✅
5. **P4 (完善)**: Phase 9 测试和文档（部分待实现）
6. **P5 (优化)**: Phase 10 Bug 修复 ✅
7. **P6 (Director)**: Phase 11 Director 完整实现 ✅
8. **P7 (成员变量)**: Phase 12 成员变量问题修复 ✅
9. **P8 (枚举)**: Phase 14 枚举类型处理修复 ✅
10. **P9 (VTable)**: Phase 15 关联常量 VTable 方案 ✅
11. **P10 (STL绑定)**: Phase 17 STL 容器类型绑定 ✅ **已完成核心容器**
12. **P11 (运算符)**: Phase 16 C++ 特殊成员函数到 Rust Trait 映射

---

## 最后更新
2026-04-13 (完成 Phase 17 STL 容器绑定核心部分)
- 基本完成 std::string 绑定 (`Lib/rust/std_string.i`)
- 完成 std::vector 绑定 (`Lib/rust/std_vector.i`)
- 完成 std::pair 绑定 (`Lib/rust/std_pair.i`)
- 完成 std::map 绑定 (`Lib/rust/std_map.i`)
- 添加测试用例
- Phase 1-17 核心功能全部完成 ✅
- 下一步：Phase 17.5 std::set 和 Phase 16 运算符映射

---

## Phase 20: 代码生成修复 (2026-04-14) 🔄

### 20.1 已完成的修复 ✅

- [x] 添加 `cleanRustName()` 函数处理无效标识符字符
- [x] 修复 `emitOverloadSuffix()` 处理 `std::string` 类型
- [x] 修复参数名清理（移除 `::` 等无效字符）
- [x] 修复构造函数命名清理

### 20.2 待修复的问题 (通过 rustc 检测)

#### 20.2.1 FFI 函数重复声明
- [ ] **问题**: `Rust_global_check_status__SWIG_0` 定义了两次
- [ ] **原因**: 全局函数的 FFI 声明被重复生成
- [ ] **位置**: `test_swig.rs` 第 366 行和 370 行

#### 20.2.2 Trait 方法重复定义
- [ ] **问题**: `OverloadTestTrait::get_value` 定义了两次
- [ ] **原因**: const 和非 const 版本的方法名相同
- [ ] **解决方案**: 修改 `emitRustTrait()` 方法计数逻辑，为 const 方法添加 `_const` 后缀

#### 20.2.3 全局函数重复定义
- [ ] **问题**: `global_check_status_int` 定义了两次
- [ ] **原因**: 全局函数包装被重复生成
- [ ] **位置**: `test_swig.rs` 第 1285 行和 1289 行

#### 20.2.4 Director trait 方法名问题
- [ ] **问题**: 方法名如 `on_message_std::string` 包含 `::`
- [ ] **影响**: 无效的 Rust 标识符

#### 20.2.5 VTable 字段名问题
- [ ] **问题**: 字段名如 `on_message_std::string` 包含 `::`
- [ ] **影响**: 无效的 Rust 标识符

#### 20.2.6 未定义类型 `std_string`
- [ ] **问题**: 代码使用了 `std_string` 类型但未定义
- [ ] **解决方案**: 生成类型定义或专门定义 `SwigString`，支持双向转换成String

### 20.3 修复优先级

1. **P0**: FFI 函数重复声明（影响编译）✅
2. **P0**: Trait 方法重复定义（影响编译）✅
3. **P0**: 全局函数重复定义（影响编译）✅
4. **P1**: Director trait/VTable 方法名清理 ✅
5. **P1**: `std_string` 类型定义 ✅

### 20.4 设计决策：const/non-const 重载处理 ✅

**问题**：Rust trait 方法不能仅通过 self 类型（`&self` vs `&mut self`）区分重载

**决策**：**只保留第一个出现的版本，跳过后续相同签名的 const/non-const 重载**

**实现**：
- 添加 `generated_signatures` Hash 跟踪已生成的方法签名
- 当遇到相同签名时跳过（保留第一个）

### 20.5 问题修复状态 ✅

| 问题 | 状态 |
|------|------|
| FFI 函数重复声明 | ✅ 已修复 |
| 全局函数重复定义 | ✅ 已修复 |
| const/non-const 方法重复 | ✅ 已修复（只保留第一个） |
| Director trait 方法名 | ✅ 已修复 |
| VTable 字段名 | ✅ 已修复 |
| `std_string` 未定义 | ✅ 已修复（返回 `String` 类型） |

---

## Phase 21: 新发现的代码生成问题 (2026-04-14) 🔄

### 21.1 String 类型冲突 🔴 P0

**问题**:
```rust
fn whoami(&self) -> String {
    String { ptr: unsafe { ffi::Rust_Base_whoami__SWIG_0(self.ptr) } }
}
```

`String` 是 Rust 标准库类型，不能用作结构体包装器。

- [ ] **解决方案**: 生成自定义类型 `SwigString` 
- [ ] **位置**: `emitRustImpl()` 中处理返回值类型

### 21.2 继承关系未正确实现 🔴 P0

**问题**:
```rust
pub trait DerivedTrait: BaseTrait { ... }
impl DerivedTrait for Derived { ... }  // 错误：Derived 没有实现 BaseTrait
```

- [ ] **解决方案**: 为派生类同时生成基类 trait 的实现
- [ ] **位置**: `emitRustImpl()` 中处理继承关系

### 21.3 bool 类型转换问题 🟡 P1

**问题**:
```rust
unsafe { ffi::Rust_BasicTypes_bool_val_set__SWIG_0(self.ptr, bool_val) }
// 错误：expected `u8`, found `bool`
```

- [ ] **解决方案**: 添加类型转换 `bool_val as u8` 或修改 typemap
- [ ] **位置**: `emitRustImpl()` 中生成 FFI 调用的逻辑

---

## Phase 22: 命名空间支持分析与限制 (2026-04-14) ✅

### 22.1 已完成的分析

- [x] 分析 C++ namespace → Rust mod 映射关系
- [x] 改进 `addOpenMod`/`addCloseMod` 支持嵌套命名空间
- [x] 修复 `emitRustSafeWrapper` 去重逻辑（使用 `namespace::symname` 作为 key）
- [x] 发现核心限制：Rust 不允许重复定义同一个 `mod`
- [x] 在 `RUST_DESIGN.md` 添加限制说明文档
- [x] 在 `Lib/rust/rust.swg` 添加使用说明

### 22.2 当前命名空间支持状态

| 功能 | 状态 | 说明 |
|------|------|------|
| `-namespace <name>` 选项 | ✅ | 将所有内容包装到一个模块 |
| 嵌套命名空间解析 | ✅ | 支持 `.` 分隔符 |
| `%feature("nspace")` | ⚠️ 默认禁用 | 会生成重复 `mod` 定义 |

### 22.3 核心限制

**问题**：Rust 不允许重复定义同一个 `mod`

```cpp
// C++ 合法
namespace math { struct Vector { ... }; }
namespace math { struct Matrix { ... }; }  // 扩展 math

// Rust 不合法
pub mod math { pub struct Vector { ... } }
pub mod math { pub struct Matrix { ... } }  // 错误！重复定义
```

### 22.4 当前推荐方案

1. 使用 `-namespace mylib` 包装所有内容
2. 使用 `%rename` 添加前缀避免冲突
3. 分开处理不同命名空间（多个 SWIG 运行）

### 22.5 未来改进（Phase 23）

- [ ] 收集阶段：遍历 AST 收集所有命名空间及其内容
- [ ] 组织阶段：构建命名空间树，合并同名命名空间
- [ ] 生成阶段：每个命名空间生成一个 `pub mod` 块
- [ ] 需要数据结构：`Hash *namespace_content`, `Hash *opened_namespaces`

---

## Phase 23: SwigString FFI 实现完善 (2026-04-15) 🔄

### 23.1 已完成的工作 ✅

#### 23.1.1 修复 getRustUserType() use-after-free 崩溃 ✅
- **问题**: `base` 指针在处理 `std::string` 和 `char*` 后被删除，但 switch 语句的 `T_USER` 和 `default` 分支仍尝试使用它
- **修复**: 在这些分支中重新调用 `SwigType_base(t)` 获取新的 base

#### 23.1.2 添加 `rustcode` section 注册 ✅
- **文件**: `Source/Modules/rust.cxx`
- **修改**: 添加 `Swig_register_filebyname("rustcode", f_wrapper_code)`
- **用途**: 允许 `%insert("rustcode")` 将 Rust 代码插入到生成的 `.rs` 文件中

#### 23.1.3 SwigString FFI 函数完整实现 ✅
- **文件**: `Lib/rust/std_string.i`
- **已实现的 C++ 辅助函数**:
  - `SwigString_new()`, `SwigString_from_bytes()`, `SwigString_from_c_str()`
  - `SwigString_delete()`, `SwigString_c_str()`, `SwigString_data()`
  - `SwigString_len()`, `SwigString_is_empty()`, `SwigString_clear()`
  - `SwigString_append()`, `SwigString_append_c_str()`, `SwigString_assign()`
  - `SwigString_capacity()`, `SwigString_reserve()`, `SwigString_compare()`
  - `SwigString_clone()`, `SwigString_resize()`, `SwigString_substr()`
  - `SwigString_find()`, `SwigString_rfind()`
  - `SwigString_push_back()`, `SwigString_pop_back()`
  - `SwigString_at()`, `SwigString_set_at()`

#### 23.1.4 新增 Typemap 定义 ✅
| Typemap | 用途 | 示例 |
|---------|------|------|
| `rsin` | Rust 参数传递方式 | `$input.ptr` |
| `rsout` | Rust 返回值包装 | `SwigString { ptr: $result }` |

### 23.2 待完成的工作

#### 23.2.1 rust.cxx 中使用 Typemap 替代硬编码 (P0) ✅ 已完成
**已实现的修复**:

1. **新增辅助函数**:
   - `getRustInputConversion(Parm *p, String *arg_name)` - 使用 `rsin` typemap
   - `getRustOutputConversion(Node *n, String *result_var)` - 使用 `rsout` typemap
   - `emitFFIParams(ParmList *params, bool skip_self)` - 统一生成 FFI 参数列表

2. **替换的硬编码位置**:
   - [x] `emitRustImpl()` 中的参数传递
   - [x] `emitRustSafeWrapper()` 中的参数传递
   - [x] 构造函数中的参数传递
   - [x] 基类方法实现中的参数传递

3. **额外修复**:
   - [x] `Lib/rust/std_string.i` 中删除重复的 `impl Into<String>` (Rust 自动从 From 生成)
   - [x] `emitRustSafeWrapper()` 中添加 SwigString 返回值包装

**已知限制**: 成员变量 getter 返回引用 (`&SwigString`) 仍有类型不匹配问题，需要修改返回类型为 owned 类型。

#### 23.2.2 std::string 成员方法 FFI 绑定 (P1)
- [x] 基础方法: new, from_bytes, delete, c_str, len, is_empty, clear, append
- [x] 容量方法: capacity, reserve, resize
- [x] 查找方法: find, rfind
- [x] 修改方法: push_back, pop_back, at, set_at
- [ ] 高级方法: insert, erase, replace, find_first_of, find_last_of

#### 23.2.3 双向转换优化 (P1)
- [x] `From<String>` - 基础实现
- [x] `Into<String>` - 基础实现
- [x] `to_string_lossy()` - UTF-8 容错
- [ ] 零拷贝优化（如果可能）

### 23.3 测试计划

- [x] 创建 `std_string_ffi_test.i` 测试文件
- [ ] 测试空字符串创建/销毁
- [ ] 测试从 Rust String 创建 SwigString
- [ ] 测试 SwigString 转换为 Rust String
- [ ] 测试字符串操作方法
- [ ] 测试 UTF-8 边界情况

---

## 开发优先级（更新 2026-04-15）

| 优先级 | Phase | 描述 | 状态 |
|--------|-------|------|------|
| P0 | Phase 1-15 | 基础框架和核心功能 | ✅ 完成 |
| P0 | Phase 23.2.1 | rust.cxx 使用 Typemap 替代硬编码 | ✅ 完成 |
| P1 | Phase 24 | 继承链 trait 实现修复 | ✅ 完成 |
| P1 | Phase 25 | long/size_t 类型映射修复 | ✅ 完成 |
| P1 | Phase 26 | 枚举返回类型 FFI 转换修复 | ✅ 完成 |
| P1 | Phase 23.2.2 | std::string 高级方法 FFI | 部分完成 |
| P2 | Phase 16 | 运算符映射完善 | 部分完成 |
| P2 | Phase 23.2.3 | 双向转换优化 | 待实现 |
| P2 | Phase 27 | 成员变量 getter 返回引用问题 | 待修复 |
| P3 | Phase 17.6 | 其他 STL 容器 | 待实现 |
| P4 | Phase 9.3 | 文档编写 | 待实现 |

---

## Phase 24: 继承链 trait 实现修复 ✅ (2026-04-15 完成)

### 24.1 问题描述

**场景**: 三层继承 `GrandDerived -> Derived -> Base`

**问题**: `GrandDerived` 只实现了 `DerivedTrait`，缺少 `BaseTrait` 实现

**Rust 编译错误**:
```
error[E0277]: the trait bound `GrandDerived: BaseTrait` is not satisfied
  --> inherit_basic.rs:296:28
   |
296| impl GrandDerivedTrait for GrandDerived { ... }
   |                            ^^^^^^^^^^^^ the trait `BaseTrait` is not implemented
```

### 24.2 原因分析

原 `emitRustImpl()` 代码只处理直接基类：
```cpp
if (derived_flag && baseclass && Len(baseclass) > 0) {
  Printf(f_wrapper_code, "impl %sTrait for %s {\n", baseclass, name);
  // 只为直接基类生成 trait 实现
}
```

### 24.3 修复方案

**文件**: `Source/Modules/rust.cxx`

**修改**: 使用工作列表算法收集所有祖先类，然后为每个生成 trait 实现

```cpp
// 收集所有祖先类
List *all_ancestors = NewList();
List *worklist = NewList();
Append(worklist, baseclass);
int worklist_index = 0;

while (worklist_index < Len(worklist)) {
  String *current_base = Getitem(worklist, worklist_index);
  worklist_index++;
  
  // 添加到祖先列表
  if (!found_in_ancestors) {
    Append(all_ancestors, current_base);
    // 查找当前基类的父类
    Node *base_node = classLookup(current_base);
    if (base_node) {
      List *parent_bases = Getattr(base_node, "bases");
      // 将父类加入工作列表
    }
  }
}

// 为每个祖先生成 trait 实现
for (String *ancestor : all_ancestors) {
  Printf(f_wrapper_code, "impl %sTrait for %s {\n", ancestor, name);
  // ... 生成方法实现
}
```

### 24.4 生成的代码示例

```rust
// GrandDerived 现在正确实现所有祖先 trait
impl GrandDerivedTrait for GrandDerived {
    fn grand_method(&mut self) -> i32 { ... }
}

impl DerivedTrait for GrandDerived {
    fn derived_method(&mut self) -> i32 { ... }
    fn virtual_method(&mut self) -> i32 { ... }
}

impl BaseTrait for GrandDerived {  // 新增！
    fn base_method(&mut self) -> i32 { ... }
    fn virtual_method(&mut self) -> i32 { ... }
}
```

---

## Phase 25: long/size_t 类型映射修复 ✅ (2026-04-15 完成)

### 25.1 问题描述

**Windows 平台**: `long` 是 4 字节，`i64` 是 8 字节

**编译错误**:
```
error[E0308]: mismatched types
   --> primitive_types_simple.rs:121:42
    |
121 |     unsafe { ffi::Rust_test_long__SWIG_0(l) }
    |              --------------------------- ^ expected `i32`, found `i64`
```

### 25.2 修复方案

**文件**: `Lib/rust/rusttype.swg`

```swig
// 修改前
%typemap(rusttype) long "i64"
%typemap(rusttype) unsigned long "u64"

// 修改后 - 使用平台自适应类型
%typemap(rusttype) long "c_long"
%typemap(rsffitype) long "c_long"

%typemap(rusttype) unsigned long "c_ulong"
%typemap(rsffitype) unsigned long "c_ulong"
```

同样修复 `size_t` 和 `ptrdiff_t`:
```swig
%typemap(rusttype) size_t "usize"
%typemap(rsffitype) size_t "usize"

%typemap(rusttype) ptrdiff_t "isize"
%typemap(rsffitype) ptrdiff_t "isize"
```

---

## Phase 26: 枚举返回类型 FFI 转换修复 ✅ (2026-04-15 完成)

### 26.1 问题描述

类方法返回枚举类型时，FFI 返回 `c_int`，但方法签名要求枚举类型：

```rust
// 生成的代码 (有问题)
fn get_status(&self) -> EnumClass_Status {
    unsafe { ffi::Rust_EnumClass_get_status__SWIG_0(self.ptr) }  // 返回 i32
}
```

### 26.2 修复方案

**文件**: `Source/Modules/rust.cxx`

在 `emitRustImpl()` 中添加枚举类型检测和转换：

```cpp
// 检测枚举返回类型
bool is_enum_type = false;
String *return_type_name = NULL;

if (return_type) {
  // 方法 1: SWIG 内置检测
  if (SwigType_isenum(return_type)) {
    is_enum_type = true;
    return_type_name = cleanTypeName(SwigType_base(return_type));
  } else {
    // 方法 2: 类型字符串检查
    String *type_str = SwigType_str(return_type, 0);
    if (type_str && Strstr(type_str, "enum ") == Char(type_str)) {
      is_enum_type = true;
      return_type_name = cleanTypeName(SwigType_base(return_type));
    }
    // 方法 3: 排除法检测自定义枚举
    else if (ret_type && !is_basic_type(ret_type) && 
             SwigType_type(return_type) != T_USER) {
      is_enum_type = true;
      return_type_name = Copy(ret_type);
    }
  }
}

// 生成转换代码
if (is_enum_type && return_type_name) {
  Printf(f_wrapper_code, "unsafe { std::mem::transmute::<i32, %s>(ffi::%s(...)) }", 
         return_type_name, wname);
}
```

### 26.3 生成的代码示例

```rust
fn get_status(&self) -> EnumClass_Status {
    unsafe { std::mem::transmute::<i32, EnumClass_Status>(
        ffi::Rust_EnumClass_get_status__SWIG_0(self.ptr)
    )}
}
```

---

## Phase 27: 待修复问题

### 27.1 成员变量 getter 返回引用

**问题**: 成员变量 getter 返回 `&SwigString` 但 FFI 返回指针

**解决方案**: 修改返回类型为 owned 类型 `SwigString`

### 27.2 其他待完成

- [x] std::string 高级方法: insert, erase, replace, find_first_of, find_last_of
- [ ] 其他 STL 容器: deque, list, unordered_map, unordered_set
- [ ] 编写 Rust 绑定文档 `Doc/Manual/Rust.html`

---

## Phase 28: std::string 高级方法 FFI 绑定 ✅ (2026-04-15 完成)

### 28.1 新增的高级方法

| 方法 | 说明 |
|------|------|
| `insert`, `insert_str`, `insert_char` | 在位置插入字节/字符串/字符 |
| `erase` | 删除指定范围的字符 |
| `replace`, `replace_str` | 替换指定范围的字符 |
| `find_first_of` | 查找第一个匹配字符集中的任一字符 |
| `find_last_of` | 查找最后一个匹配字符集中的任一字符 |
| `find_first_not_of` | 查找第一个不在字符集中的字符 |
| `find_last_not_of` | 查找最后一个不在字符集中的字符 |
| `shrink_to_fit` | 收缩容量以适应大小 |
| `front`, `back` | 获取首/尾字符 |
| `swap` | 交换两个字符串内容 |

### 28.2 修改的文件

- `Lib/rust/std_string.i` - 添加高级方法的 C++ FFI 函数和 Rust 实现

---

## Phase 29: std::wstring 支持 ✅ (2026-04-15 完成)

### 29.1 创建的文件

| 文件 | 说明 |
|------|------|
| `Lib/rust/std_wstring.i` | std::wstring 类型映射和 SwigWString 包装 |
| `Lib/rust/rusttype.swg` | 添加 wchar_t/wchar_t* 类型映射 |
| `Examples/test-suite/rust/std_wstring_test.i` | 测试文件 |

### 29.2 SwigWString 功能

| 功能 | 说明 |
|------|------|
| **FFI 函数** | new, delete, from_utf16, from_utf32, len, clear, append, clone, compare 等 |
| **平台检测** | `wchar_size()` 返回 wchar_t 大小（Windows=2, Unix=4） |
| **数据访问** | `as_u16_slice()` / `as_u32_slice()` |
| **trait 实现** | Default, Drop, Clone, Debug, PartialEq, Eq, PartialOrd, Ord |

### 29.3 修改的文件

- `Source/Modules/rust.cxx` - 添加 SwigWString 类型检测和返回值处理
- `Lib/rust/rusttype.swg` - 添加 wchar_t 类型映射

### 29.4 设计要点

- **不提供与 Rust String 的转换**（按要求）
- **wchar_t 映射到 u16**（Windows 平台）
- **支持 UTF-16 和 UTF-32 输入**

---

## Phase 30: 代码重构计划 🔄 (待实现)

### 30.1 问题背景

当前 `rust.cxx` 中硬编码了 `std::string` 和 `std::wstring` 的处理逻辑，这不符合 SWIG 的设计模式。

**对比其他语言模块**:

| 语言模块 | `.cxx` 文件 | `.i` 文件 |
|---------|------------|----------|
| C# | 只处理字面量转义 | `std_string.i`, `std_wstring.i` |
| Java | 不处理 std::string | `std_wstring.i` |
| Python | 不处理 std::string | `std_string.i`, `pywstrings.swg` |
| **Rust (当前)** | 硬编码 SwigString/SwigWString | 有但不完整 |

### 30.2 重构目标

将 `rust.cxx` 中的硬编码逻辑移到 typemap 文件中：

**当前代码** (rust.cxx 硬编码):
```cpp
if (Cmp(return_type, "SwigString") == 0) {
  Printf(f_wrapper_code, "SwigString { ptr: unsafe { ffi::%s(...", wname);
}
```

**目标代码** (依赖 typemap):
```cpp
String *rsout = Swig_typemap_lookup("rsout", n, result_var, 0);
if (rsout) {
  // 使用 typemap 中的转换代码
  Printf(f_wrapper_code, "%s", rsout);
}
```

### 30.3 需要修改的文件

1. **`Lib/rust/std_string.i`** - 完善 typemap:
   ```swig
   %typemap(rsin) std::string "$input.ptr"
   %typemap(rsout) std::string "SwigString { ptr: $result }"
   ```

2. **`Lib/rust/std_wstring.i`** - 完善 typemap:
   ```swig
   %typemap(rsin) std::wstring "$input.ptr"
   %typemap(rsout) std::wstring "SwigWString { ptr: $result }"
   ```

3. **`Source/Modules/rust.cxx`** - 移除硬编码:
   - 移除 `is_string_type`, `is_wstring_type` 特殊检测
   - 移除 `return_is_swigstring`, `return_is_swigwstring` 变量
   - 改用统一的 typemap 查找机制

### 30.4 重构优先级

| 优先级 | 任务 | 影响 |
|--------|------|------|
| P1 | 完善 rsin/rsout typemap | 用户可自定义类型映射 |
| P1 | 移除 rust.cxx 硬编码 | 符合 SWIG 设计模式 |
| P2 | 测试验证重构 | 确保功能不变 |

### 30.5 重构好处

- **符合 SWIG 设计哲学** - 类型处理逻辑集中在 typemap
- **用户可扩展** - 用户可以自定义类型映射
- **代码更清晰** - rust.cxx 只做通用 typemap 查找
- **维护更简单** - 新增类型只需添加 .i 文件

---

## 最后更新
2026-04-15 (Phase 28-29: std::string 高级方法、std::wstring 支持、重构计划)
