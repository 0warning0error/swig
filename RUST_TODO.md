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
- [x] 枚举类型测试（2026-04-11 修复）

### 9.2 集成测试 ✅ (2026-04-11)
- [x] 创建 `Examples/rust/` 目录
- [x] 编写示例代码
- [x] 添加到 test-suite（`Examples/test-suite/rust/`）
- [x] 测试运行脚本 `run_tests.py`
- [x] Rust 特定测试 (11个):
  - `const_var` - 常量和变量
  - `class_methods` - 类方法类型
  - `inherit_basic` - 单继承和多级继承
  - `enums_test` - 各种枚举
  - `namespace_test` - 嵌套命名空间
  - `pointer_ref` - 指针和引用
  - `overload_test` - 函数重载
  - `template_test` - 模板实例化
  - `director_test` - Director 回调
  - `static_members` - 静态成员
  - `primitive_types_simple` - 基本类型
- [x] 通用测试 (5个): enums, struct_value, template_basic, inherit, overload_simple
- [x] **总计 16 个测试全部通过**

### 9.3 文档
- [ ] 编写 Doc/Manual/Rust.html
- [ ] README 更新

---

## Phase 10: Bug 修复 (2026-04-11)

### 10.1 枚举类型处理修复 ✅
- [x] 跳过匿名枚举（名称以 `$` 开头）或生成常量
- [x] 跳过空枚举（没有值）
- [x] 修复返回类型中的 `enum` 关键字（`enum foo2` → `foo2`）
- [x] 添加 `cleanTypeName()` 函数去除 `enum`/`struct`/`class` 关键字
- [x] 匿名枚举返回类型使用 `i32` 作为 fallback

### 10.2 重载函数生成修复 ✅
- [x] 重载构造函数添加类型后缀（`new()`, `new_int()`, `new_f64()`）
- [x] 重载全局函数添加类型后缀
- [x] 改进 `emitOverloadSuffix()` 正确处理枚举类型

### 10.3 类型映射修复 ✅
- [x] 枚举类型映射使用 `SWIGENUM` 标记
- [x] 在 `functionWrapper()` 中正确处理枚举返回类型
- [x] 在 `emitRustSafeWrapper()` 中使用 `processRustType()` 处理参数类型
- [x] 添加 `processRustType()` 函数处理类型映射中的特殊标记

---

## 开发优先级

1. **P0 (必须)**: Phase 1-4 - 基础框架 + 函数 + 类 ✅
2. **P1 (重要)**: Phase 5-6 - 继承 + Director ✅
3. **P2 (需要)**: Phase 7 - 枚举和常量 ✅
4. **P3 (增强)**: Phase 8 - 高级特性 ✅
5. **P4 (完善)**: Phase 9 - 测试和文档（部分待实现）
6. **P5 (优化)**: Phase 10 - Bug 修复 ✅

---

## 当前状态

**状态**: 核心功能已完成，Trait 泛化方案已实现

**已完成**:
- Phase 1-7 全部完成
- Phase 8 高级特性完成（模板、智能指针、异常、命名空间、Trait 泛化方案）
- Phase 9 测试集成完成（8 个测试用例全部通过）
- Phase 10 Bug 修复完成（枚举、重载、类型映射）
- Director 支持（虚函数回调）
- 函数重载（类型后缀方案 + Trait 泛化方案）

**待实现**:
- 默认参数 Builder 模式
- Phase 9.3 用户文档

**下一步**: 编写用户文档或实现默认参数 Builder 模式
