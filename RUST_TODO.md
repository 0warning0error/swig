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
- [x] `std::string` → 不透明类型或 String

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

## 最后更新
2026-04-13 (实现关联常量 VTable 方案)
- 新增 `-director-vtable` 命令行选项
- 实现零开销的 Director VTable 方案
- 使用 Rust 关联常量为每个实现类型提供静态 VTable
- Phase 1-15 全部完成 ✅
