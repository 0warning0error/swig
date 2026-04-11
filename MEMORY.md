# SWIG Rust 绑定开发进度

## 已完成的工作

### Phase 1: 基础框架搭建 ✅

#### 1.1 创建 Rust 语言模块
- **文件**: `Source/Modules/rust.cxx` (~2800 行)
- **状态**: 完成
- **实现内容**:
  - `RUST` 类继承 `Language` 基类
  - 命令行参数处理: `-crate-name`, `-module`, `-safe-wrapper`, `-ffi-only`, `-no-directors`, `-noproxy`, `-trait-overload`
  - `top()` 初始化输出文件（支持 Director 头文件）
  - `functionWrapper()` 生成 C 包装函数和 Rust FFI 声明
  - `classHandler()`, `constructorHandler()`, `destructorHandler()` 类处理
  - `enumDeclaration()`, `constantWrapper()` 枚举和常量处理
  - `variableHandler()` 变量处理框架
  - `memberfunctionHandler()`, `staticmemberfunctionHandler()` 成员函数处理
  - `membervariableHandler()`, `staticmembervariableHandler()` 成员变量处理
  - `getRustType()` 获取 Rust FFI 类型
  - `getRustUserType()` 获取 Rust 用户可见类型
  - `processRustType()` 处理类型映射中的特殊标记（如 SWIGENUM）
  - `cleanTypeName()` 清理类型名称，去除 enum/struct/class 关键字

#### 1.2 注册语言模块
- **文件**: `Source/Modules/swigmain.cxx`
- **状态**: 完成
- **修改内容**:
  - 添加 `Language *swig_rust(void);` 声明
  - 在 `modules[]` 数组添加 `{"-rust", swig_rust, "Rust", Experimental}`

#### 1.3 创建 Lib/rust 目录结构
- **文件**: 
  - `Lib/rust/rust.swg` - 主配置文件（含 %rust_import, %rust_code 宏定义）
  - `Lib/rust/rusttype.swg` - 完整类型映射（~280 行）
  - `Lib/rust/rustrun.swg` - 运行时支持
- **状态**: 完成

#### 1.4 更新构建系统
- **文件**: `Source/Makefile.am`
- **状态**: 完成
- **修改**: 添加 `Modules/rust.cxx` 到编译列表

### Phase 2: 类型映射系统 ✅

**`Lib/rust/rusttype.swg`** 实现了完整的类型映射:

| Typemap | 用途 |
|---------|------|
| `rusttype` | Rust 用户可见类型 |
| `rsffitype` | Rust FFI 层类型 |
| `cout` | C 包装函数输出类型 |
| `in` | C 输入参数转换 |
| `out` | C 返回值转换 |
| `typecheck` | 重载函数类型检查 |

**已映射的类型**:
- 基本整数类型: bool, char, short, int, long, long long (含 unsigned)
- 浮点类型: float, double
- 特殊类型: size_t, ptrdiff_t, void
- 指针类型: void*, const void*, SWIGTYPE*, const SWIGTYPE*
- 引用类型: SWIGTYPE&, const SWIGTYPE&, SWIGTYPE&&
- 数组类型: SWIGTYPE[], SWIGTYPE[ANY], char[ANY]
- 字符串类型: char*, const char*
- 枚举类型: enum SWIGTYPE
- 成员函数指针: SWIGTYPE (CLASS::*)

### Phase 3-5: 函数包装、类处理、继承 ✅

已完成：
- 参数处理、返回值处理、成员函数支持
- const/non-const 成员函数区分
- 静态成员函数支持
- 单继承和多继承支持

### Phase 7: 枚举和常量 ✅

- **枚举**: `enumDeclaration()` 生成 `#[repr(C)]` Rust enum
- **常量**: `constantWrapper()` 生成 Rust const 声明

---

## 当前状态 (2026-04-11 最新)

### ✅ 本次会话修复的问题

#### 1. 枚举类型处理修复
- **问题**: 枚举类型生成的代码存在问题：
  - 空枚举被生成（没有值）
  - 匿名枚举名称无效（如 `$unnamed2$`）
  - 返回类型包含 `enum` 关键字（如 `enum foo2`）
- **修复**:
  - 修改 `enumDeclaration()` 跳过匿名枚举或生成常量
  - 添加 `cleanTypeName()` 函数去除 `enum`/`struct`/`class` 关键字
  - 修改 `emitOverloadSuffix()` 正确处理枚举类型后缀
  - 匿名枚举返回类型使用 `i32` 作为 fallback

#### 2. 重载函数生成修复
- **问题**: 重载函数生成多个同名函数导致 Rust 编译错误
- **修复**:
  - 修改 `emitRustConstructor()` 为重载构造函数添加类型后缀（如 `new_int()`, `new_f64()`）
  - 修改 `emitRustSafeWrapper()` 为重载全局函数添加类型后缀
  - 改进 `emitOverloadSuffix()` 正确识别和处理枚举类型

#### 3. 类型映射修复
- **修复**: 枚举类型映射使用 `SWIGENUM` 标记
- **修复**: 在 `functionWrapper()` 中正确处理枚举返回类型
- **修复**: 在 `emitRustSafeWrapper()` 中使用 `processRustType()` 处理参数类型

### ✅ 本次会话新增的功能

#### 1. 命名空间支持 (Phase 8.4)
- **新增**: `-namespace <name>` 命令行选项
- **新增**: `namespce`, `current_nspace` 成员变量
- **新增**: `addOpenMod()`, `addCloseMod()` 方法生成 Rust `pub mod` 声明
- **新增**: `getNSpace()`, `outputDirectory()` 辅助方法
- **修改**: `classHandler()` 和 `enumDeclaration()` 支持命名空间

#### 2. 模板支持 (Phase 8.1)
- **状态**: SWIG 核心已支持 `%template` 指令
- **验证**: `test_template.i` 测试通过，正确生成 `IntContainer` 和 `DoubleContainer`

#### 3. 智能指针支持 (Phase 8.2)
- **新增**: `Lib/rust/std_shared_ptr.i` - std::shared_ptr 类型映射
- **新增**: `Lib/rust/std_unique_ptr.i` - std::unique_ptr 类型映射
- **设计**: FFI 层作为 `*mut c_void` 不透明句柄

#### 4. 异常处理支持 (Phase 8.3)
- **新增**: `Lib/rust/exception.i` - 异常处理框架
- **设计**: 
  - C++ 异常捕获并存储到线程本地状态
  - 提供 `SWIG_RustGetLastError()`, `SWIG_RustGetLastErrorMsg()` FFI 函数
  - 标准异常类型映射 (std::exception, std::runtime_error, etc.)

### ✅ 之前会话修复的内容

#### 1. emitRustTrait 崩溃修复
- **问题**: `Swig_typemap_lookup("rusttype", p, Getattr(p, "lname"), 0)` 在 `lname` 为 NULL 时崩溃
- **原因**: 在 `emitRustTrait` 中，参数节点尚未经过 `emit_parameter_variables` 处理，`lname` 属性未设置
- **修复**: 添加 `getRustUserType()` 函数，当 typemap 查找失败时使用该函数生成用户可见类型

#### 2. emitRustTrait 参数输出修复
- **问题**: 参数名与 `self` 连接在一起，如 `fn setValue(&mut selfv: i32)`
- **原因**: 输出参数时没有在 `self` 后添加逗号
- **修复**: 在输出第一个非 self 参数前添加逗号

#### 3. emitRustImpl 参数处理修复
- **问题**: impl 块中参数丢失，FFI 调用缺少参数
- **原因**: 错误地跳过了第一个参数（以为是 self 指针）
- **修复**: 对于类成员函数，原始参数列表不包含 this 指针，不应跳过任何参数

#### 4. 添加 getRustUserType 函数
- **目的**: 返回用户可见的 Rust 类型（如 `i32`, `i64`），而非 FFI 类型（如 `c_int`, `c_long`）
- **位置**: 在 `emitRustTrait` 和 `emitRustImpl` 中使用

### ✅ 验证通过的测试用例

| 测试文件 | 类描述 | 状态 |
|---------|--------|------|
| `test_class_simple.i` | 简单类 + 构造函数 + 无参方法 | ✅ |
| `test_void_method.i` | 无参 void 方法 | ✅ |
| `test_int_method.i` | 无参返回 int 方法 | ✅ |
| `test_mixed_members.i` | 只有成员变量的类 | ✅ |
| `test_param_void.i` | 带参数 void 方法 `setValue(int v)` | ✅ |
| `test_param_method.i` | 带参数 void 方法 | ✅ |
| `test_return_method.i` | 带返回值方法 | ✅ |
| `test_template.i` | 模板类实例化 `%template` | ✅ |
| `test_namespace.i` | 命名空间 `namespace` | ✅ |
| `test_shared_ptr.i` | 智能指针 `std::shared_ptr` | ✅ |

---

## 待完成的工作

### Phase 6: Director 支持 ✅ (2026-04-11 完成)
- [x] Director 框架初始化 (`classDirectorInit`)
- [x] 虚函数转发 (`classDirectorMethod`)
- [x] C++ 桥接类生成 (`SwigDirector_XXX`)
- [x] Rust trait 对象存储 (`new_with_trait`)
- [x] 安全回调设计（`&self` 默认，文档说明重入风险）

### Phase 8: 高级特性
- [x] **函数重载处理** ✅
  - 实现类型后缀方案：`bar_int`, `bar_str`, `bar_int_int`
  - 添加 `emitOverloadSuffix()` 和 `countOverloads()` 辅助方法
- [x] **Trait 泛化方案** ✅ (2026-04-11)
  - 添加 `-trait-overload` 命令行选项
  - 实现 `buildOverloadInfo()` 构建重载信息
  - 实现 `emitOverloadTraits()` 生成重载 trait 定义
  - 实现 `emitOverloadTraitImpls()` 为每个重载生成 trait 实现
  - 实现 `emitOverloadEntryPoints()` 生成统一入口方法
  - 单参数重载直接使用参数类型
  - 多参数重载使用元组类型 `(T1, T2, ...)`
  - 支持 `type Output` 关联类型处理不同返回类型
- [x] **模板类实例化** ✅ (2026-04-11)
  - SWIG 核心已支持 `%template` 指令
  - 验证通过：`test_template.i`
- [x] **智能指针** ✅ (2026-04-11)
  - `Lib/rust/std_shared_ptr.i` - std::shared_ptr 支持
  - `Lib/rust/std_unique_ptr.i` - std::unique_ptr 支持
- [x] **异常处理** ✅ (2026-04-11)
  - `Lib/rust/exception.i` - 异常捕获和错误码传递
- [x] **命名空间** ✅ (2026-04-11)
  - 映射到 Rust `pub mod`
  - `-namespace` 命令行选项
- [ ] 默认参数 Builder 模式（后续优化：Trait 泛化方案）

### Phase 9: 测试和文档
- [x] 简单函数测试通过
- [x] 简单类测试通过（无参方法）
- [x] 带参数方法代码生成
- [x] Director 测试通过 (`test_director.i`)
- [x] 函数重载测试通过 (`test_overload.i`)
- [x] 添加到 test-suite ✅ (2026-04-11)
  - 创建 `Examples/test-suite/rust/` 目录
  - 创建 `Makefile.in` 和 `run_tests.py`
  - **Rust 特定测试** (11个):
    - `const_var` - 常量和变量测试
    - `class_methods` - 类方法测试（void、参数、返回值、静态、const）
    - `inherit_basic` - 继承测试（单继承、多级继承、虚函数）
    - `enums_test` - 枚举测试（基本枚举、带值枚举、类内枚举）
    - `namespace_test` - 命名空间测试（嵌套命名空间）
    - `pointer_ref` - 指针和引用测试
    - `overload_test` - 函数重载测试
    - `template_test` - 模板实例化测试
    - `director_test` - Director（虚函数回调）测试
    - `static_members` - 静态成员和全局变量测试
    - `primitive_types_simple` - 简化基本类型测试
  - **通用测试** (5个): enums, struct_value, template_basic, inherit, overload_simple
  - **总计**: 16 个测试全部通过
- [ ] 编写 `Doc/Manual/Rust.html`

---

## 构建说明

```bash
# Windows (Visual Studio)
cmake -B build -S . -A x64 -DWITH_PCRE=OFF
cmake --build build --config Release

# 运行测试
$env:SWIG_LIB="D:\code\cpp\swig\Lib"
.\build\Release\swig.exe -rust -c++ -module test_simple test_simple.i
```

---

## 最后更新
2026-04-11 (完善测试用例：16个测试全部通过)
