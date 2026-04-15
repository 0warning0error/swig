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

## 当前状态 (2026-04-12 最新)

### ✅ 本次会话修复的问题

#### 1. 枚举类型生成修复 (2026-04-12)
- **问题**: 枚举定义没有正确生成到 Rust 代码中
  - 全局枚举 `Color` 和类内枚举 `EnumClass::Status` 未生成
  - 生成了错误的常量如 `pub const RED: i32 = RED;`
- **原因**: 
  - SWIG 枚举值子节点的 nodeType 是 `"enumitem"` 而不是 `"enumvalue"`
  - 调用 `Language::enumDeclaration(n)` 会触发 `constantWrapper` 生成错误常量
- **修复**:
  - 修改 `enumDeclaration()` 使用正确的节点类型 `"enumitem"`
  - 不调用 `Language::enumDeclaration(n)`，直接处理枚举值
  - 类内枚举生成独立名称（如 `EnumClass_Status`）
  - 修改 `cleanTypeName()` 处理 C++ 作用域分隔符 `::`

#### 2. 枚举类型处理修复 (之前会话)
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
2026-04-13 (完成 Phase 16-18 运算符 Trait 完善和 STL 容器扩展)

### Phase 18: STL 容器扩展 (2026-04-13 完成)

**新增文件**:
- `Lib/rust/std_deque.i` - std::deque → VecDeque 映射
- `Lib/rust/std_list.i` - std::list → LinkedList 映射
- `Lib/rust/std_unordered_map.i` - std::unordered_map → HashMap 映射
- `Lib/rust/std_unordered_set.i` - std::unordered_set → HashSet 映射

**支持的容器映射**:
| C++ 容器 | Rust 类型 | 说明 |
|---------|----------|------|
| std::vector<T> | Vec<T> | 动态数组 |
| std::deque<T> | VecDeque<T> | 双端队列 |
| std::list<T> | LinkedList<T> | 双向链表 |
| std::map<K,V> | BTreeMap | 有序映射 |
| std::set<T> | BTreeSet | 有序集合 |
| std::unordered_map<K,V> | HashMap | 无序映射 |
| std::unordered_set<T> | HashSet | 无序集合 |

### Phase 16: 运算符 Trait 实现 ✅ (2026-04-13 完成)

**修复**: 运算符参数类型处理问题
- 引用类型参数现在正确获取类名
- 基本类型参数直接传递值
- 类类型参数使用 `.ptr` 字段

**验证**: test_operator.i 生成的代码正确：
- `impl Add for Vector2` - rhs: Vector2
- `impl Mul for Vector2` - rhs: f64
- `impl PartialEq for Vector2` - other: &Self
- `impl PartialOrd for Point` - other: &Self

### Phase 16: 运算符映射基础架构 ✅ (2026-04-13 完成)

**实现内容**:
- 添加 `isOperatorMethod()` - 检测方法是否是 C++ 运算符重载
- 添加 `getOperatorKind()` - 获取运算符类型（如 "+", "-", "=="）
- 添加 `getRustOperatorTrait()` - 获取对应的 Rust trait 名称
- 添加 `getRustOperatorMethodName()` - 获取 Rust trait 方法名
- 添加 `getRustOperatorName()` - 获取有效的 Rust 方法名

**运算符映射表**:
| C++ 运算符 | Rust 方法名 | Rust trait |
|-----------|------------|------------|
| `operator+` | `op_add` | `std::ops::Add` |
| `operator-` (二元) | `op_sub` | `std::ops::Sub` |
| `operator-` (一元) | `op_neg` | `std::ops::Neg` |
| `operator*` | `op_mul` | `std::ops::Mul` |
| `operator/` | `op_div` | `std::ops::Div` |
| `operator==` | `op_eq` | `std::cmp::PartialEq` |
| `operator!=` | `op_ne` | `std::cmp::PartialEq` |
| `operator<` | `op_lt` | `std::cmp::PartialOrd` |
| `operator<=` | `op_le` | `std::cmp::PartialOrd` |
| `operator>` | `op_gt` | `std::cmp::PartialOrd` |
| `operator>=` | `op_ge` | `std::cmp::PartialOrd` |
| `operator[]` | `op_index` | `std::ops::Index` |
| `operator+=` | `op_add_assign` | `std::ops::AddAssign` |

**测试文件**: `test_operator.i` - Vector2 和 Point 类运算符测试

### Phase 17.5: std::set 绑定 ✅ (2026-04-13 完成)

**新增文件**: `Lib/rust/std_set.i`

**支持内容**:
- `std::set<T>` → `BTreeSet<T>` (有序集合)
- `std::unordered_set<T>` → `HashSet<T>` (无序集合)
- 基本方法：size, empty, insert, erase, clear, contains
- 扩展方法：to_vec, insert_new, remove

**测试文件**: `Examples/test-suite/rust/std_set_test.i`

### Phase 14: 枚举类型处理修复 ✅

**问题**: 枚举定义没有正确生成到 Rust 代码中
- 全局枚举 `Color` 和类内枚举 `EnumClass::Status` 未生成
- 生成了错误的常量如 `pub const RED: i32 = RED;`

**原因**: 
- SWIG 枚举值子节点的 nodeType 是 `"enumitem"` 而不是 `"enumvalue"`
- 调用 `Language::enumDeclaration(n)` 会触发 `constantWrapper` 生成错误常量

**修复**:
- [x] 修改 `enumDeclaration()` 使用正确的节点类型 `"enumitem"`
- [x] 不调用 `Language::enumDeclaration(n)`，避免触发错误的 `constantWrapper`
- [x] 为类内枚举生成独立名称（如 `EnumClass_Status`）
- [x] 修改 `cleanTypeName()` 处理 C++ 作用域分隔符 `::`
- [x] 所有 16 个测试用例通过

---

## 2026-04-12 本次会话修复的问题

### 1. rust.cxx 语法错误修复 ✅
- **问题**: 第 3144 行 `Printf` 语句和 `if` 条件分支被错误连接在同一行
- **修复**: 重新组织代码，添加正确的 `if (director_thin_flag)` 和 `else` 分支

### 2. emitRustTrait 参数处理修复 ✅
- **问题**: 错误地跳过第一个指针/引用类型参数（认为是 self 参数）
- **原因**: 类成员函数的 `params` 列表不包含 self 指针
- **修复**: 移除跳过第一个参数的逻辑

### 3. emitRustTrait const 方法检测修复 ✅
- **问题**: const 方法检测逻辑与 emitRustImpl 不一致
- **修复**: 使用与 emitRustImpl 一致的检测方式（`SwigType_isconst` 和 `Strstr(decl, "r.q(const)")`）

### 4. emitRustConstructor 参数处理修复 ✅
- **问题**: 重载构造函数没有传递参数给 FFI
- **修复**: 使用更可靠的参数遍历方式，使用 `getRustUserType` 作为 typemap fallback

### 5. 构造函数命名修复 ✅
- **问题**: 重载构造函数名称如 `new_CallbackBase` 而不是 `new_CallbackBase`
- **修复**: 正确生成类型后缀

### 6. use std::os::raw::* 导入 ✅
- **状态**: 已正确生成在 FFI 模块和包装代码中

### 后续优化项

#### 关联常量 VTable 方案 ✅ (2026-04-13 完成)
- 新增 `-director-vtable` 命令行选项
- 使用 Rust 关联常量为每个实现类型提供静态 VTable
- 通过 blanket impl 自动为所有 `Director + Sized` 类型实现 `VTableProvider`
- 生成泛型 thunk 函数作为 VTable 中的函数指针
- 零开销抽象，比 `Box<dyn Trait>` 更高效
- 需要 Rust 1.20+

#### 默认参数 Builder 模式
- 为 C++ 默认参数生成 Rust Builder 模式
- 详见 RUST_DESIGN.md 第11章

---

## 2026-04-13 Phase 17: STL 容器绑定 ✅

### 新增文件

| 文件 | 说明 |
|------|------|
| `Lib/rust/std_string.i` | std::string → String 映射 |
| `Lib/rust/std_vector.i` | std::vector<T> → Vec<T> 映射 |
| `Lib/rust/std_pair.i` | std::pair<T, U> → (T, U) 映射 |
| `Lib/rust/std_map.i` | std::map<K, V> → BTreeMap 映射 |

### std::string 绑定

```swig
%typemap(rusttype) std::string "String"
%typemap(rusttype) const std::string & "&str"
```

- 支持 std::string → String (owned)
- 支持 const std::string& → &str (borrowed)
- 支持异常处理

### std::vector 绑定

```swig
%template(IntVector) std::vector<int>;
RUST_VECTOR_TRAITS(int, IntVector)
```

- 提供基本方法：size, empty, push_back, clear
- 提供 Index/IndexMut trait（通过 getitem/setitem）
- 提供 IntoIterator trait
- 提供 From/Into 与 Vec<T> 转换
- 提供 Rust 风格方法：len, is_empty, contains, reverse, sort

### std::map 绑定

```swig
%template(IntIntMap) std::map<int, int>;
RUST_MAP_TRAITS(int, int, IntIntMap)
```

- std::map → BTreeMap (有序)
- std::unordered_map → HashMap (无序)
- 提供 keys(), values(), entries() 方法
- 提供 From/Into 与 BTreeMap 转换

### 测试用例

- `Examples/test-suite/rust/std_string_test.i`
- `Examples/test-suite/rust/std_vector_test.i`
- `Examples/test-suite/rust/std_map_test.i`

---

## 2026-04-13 本次会话：Phase 16 运算符 Trait 实现 ✅

### 实现内容

1. **新增辅助函数**:
   - `getOperatorKindFromRustName()` - 反向映射：从重命名后的方法名（如 `op_add`）返回运算符类型（如 `+`）
   - `isRenamedOperatorMethod()` - 检测方法名是否是重命名后的运算符方法

2. **新增核心函数** `emitOperatorTraitImpls(Node *n)`:
   - 遍历类的所有成员函数
   - 检测运算符方法（包括原始名 `operator +` 和重命名后 `op_add`）
   - 为每个运算符生成对应的 Rust 标准库 trait 实现

3. **支持的运算符映射**:
   | C++ 运算符 | Rust Trait | 方法签名 |
   |-----------|------------|---------|
   | `operator+` | `std::ops::Add` | `fn add(&self, rhs: Rhs) -> Self::Output` |
   | `operator-` (二元) | `std::ops::Sub` | `fn sub(&self, rhs: Rhs) -> Self::Output` |
   | `operator-` (一元) | `std::ops::Neg` | `fn neg(&self) -> Self::Output` |
   | `operator*` | `std::ops::Mul` | `fn mul(&self, rhs: Rhs) -> Self::Output` |
   | `operator/` | `std::ops::Div` | `fn div(&self, rhs: Rhs) -> Self::Output` |
   | `operator==` | `std::cmp::PartialEq` | `fn eq(&self, other: &Self) -> bool` |
   | `operator<` | `std::cmp::PartialOrd` | `fn partial_cmp(&self, other: &Self) -> Option<Ordering>` |
   | `operator[]` | `std::ops::Index` | `fn index(&self, index: Idx) -> &Self::Output` |
   | `operator+=` | `std::ops::AddAssign` | `fn add_assign(&mut self, rhs: Rhs)` |

4. **生成的代码示例**:
```rust
impl std::ops::Add for Vector2 {
    type Output = Vector2;
    fn add(&self, rhs: Vector2) -> Self::Output {
        Vector2 { ptr: unsafe { ffi::Rust_Vector2_op_add__SWIG_0(self.ptr, rhs.ptr) } }
    }
}

impl std::cmp::PartialEq for Vector2 {
    fn eq(&self, other: &Self) -> bool {
        unsafe { ffi::Rust_Vector2_op_eq__SWIG_0(self.ptr, other.ptr) }
    }
}
```

5. **使用方式**:
   - 在 SWIG 接口文件中使用 `%rename_operators` 宏重命名运算符
   - 代码生成器自动检测并生成 trait 实现
   - 用户可以直接使用 Rust 运算符语法：`v1 + v2`, `v1 == v2`, `v1 < v2`

### 待改进项

- [ ] 修复 trait 实现中的参数类型（使用正确类型而非 `*mut c_void`）
- [ ] 处理 `operator[]` 返回引用的生命周期问题
- [ ] 添加更多运算符测试用例

---

## 2026-04-13 Director VTable 优化 ✅

### 问题背景

原 VTable 模式下，C++ Director 类会逐个复制 Rust 端的函数指针到成员变量：

```cpp
// 旧实现：每个虚函数一个成员变量
class SwigDirector_CallbackBase : public CallbackBase {
public:
    bool (*onEvent_int)(void*, int);  // 复制的函数指针
    bool (*getResult)(void*, int*);   // 复制的函数指针
    void *swig_rust_director_;
};

// 构造时需要逐个复制
extern "C" void *SwigDirector_CallbackBase_new_director_vtable(void *vtable, void *rust_director) {
    VTableFuncPtr *entries = reinterpret_cast<VTableFuncPtr *>(vtable);
    director->onEvent_int = entries[0];   // 复制
    director->getResult = entries[1];     // 复制
    // ...
}
```

### 优化方案

C++ 端定义与 Rust 一致的 VTable 结构体，只存储一个指针：

```cpp
// 优化后：只存储 VTable 指针
struct SwigDirector_CallbackBase_VTable {
    bool (*onEvent_int)(void*, int);
    bool (*getResult)(void*, int*);
};

class SwigDirector_CallbackBase : public CallbackBase {
public:
    const SwigDirector_CallbackBase_VTable *vtable_;  // 只存一个指针！
    void *swig_rust_director_;
    
    virtual void onEvent(int id) override {
        if (vtable_ && vtable_->onEvent_int) {
            vtable_->onEvent_int(swig_rust_director_, id);
        }
    }
};

// 构造时直接存储指针，无需复制
extern "C" void *SwigDirector_CallbackBase_new_director_vtable(
    const SwigDirector_CallbackBase_VTable *vtable, void *rust_director) {
    director->vtable_ = vtable;  // 只存指针
    director->swig_rust_director_ = rust_director;
}
```

### 优化效果

| 方面 | 优化前 | 优化后 |
|------|--------|--------|
| 构造时复制 | N 次函数指针复制 | 0 次（只存指针） |
| 成员变量数量 | N + 1 | 2 |
| 代码复杂度 | 逐个复制逻辑 | 直接使用指针 |
| 内存布局 | 分散的成员变量 | 紧凑的 VTable 结构 |

### 实现修改

1. **`classDirectorInit()`**: 生成 C++ VTable 结构体前置声明
2. **`classDirectorMethod()`**: 虚函数从 `vtable_->method` 取函数指针
3. **`classDirectorEnd()`**: 
   - 只输出 `vtable_` 指针成员
   - 在类后定义 VTable 结构体
   - `new_director_vtable` 直接存储指针
4. **FFI 声明**: 使用 `*const VTable` 类型而非 `*mut c_void`

### 测试验证

- 测试文件: `test_vtable_opt.i`
- 生成代码验证通过
- C++ 端只存储一个 VTable 指针

---

## 2026-04-13 Phase 19: 异常处理支持 ✅

### 设计目标

将 C++ 异常转换为 Rust 惯用的 `Result<T, SwigError>` 模式。

### 实现方案

**架构**:
```
C++ 异常 → C 错误码 + 错误消息 → Rust Result<T, SwigError>
```

**修改的文件**:

1. **`Lib/rust/exception.i`** - C++ 端异常捕获和错误存储
   - 定义错误码常量 (SWIG_RUST_RuntimeError, SWIG_RUST_IndexError 等)
   - 线程本地错误状态存储
   - 标准 C++ 异常类型映射
   - `RUST_EXCEPTION_HANDLER` 宏用于包装函数

2. **`Source/Modules/rust.cxx`** - Rust 端错误处理代码生成
   - 新增 `-exception` 和 `-no-exception` 命令行选项
   - 新增 `exception_flag` 成员变量
   - 新增 `emitSwigError()` 函数生成 Rust 错误类型
   - 在 `emitRustFile()` 中生成错误检查 FFI 函数

### 生成的 Rust 代码

**SwigErrorCode 枚举**:
```rust
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[repr(C)]
pub enum SwigErrorCode {
    Ok = 0,
    MemoryError = 1,
    IOError = 2,
    RuntimeError = 3,
    IndexError = 4,
    // ...
}
```

**SwigError 结构体**:
```rust
#[derive(Debug, Clone)]
pub struct SwigError {
    pub code: SwigErrorCode,
    pub message: String,
}

impl std::error::Error for SwigError {}
impl fmt::Display for SwigError { ... }
```

**错误检查函数**:
```rust
impl SwigError {
    pub fn check() -> Option<Self> { ... }
    pub fn clear() { ... }
}
```

**便捷宏**:
```rust
macro_rules! swig_check_error {
    () => {{ ... }};
}
```

### 使用方法

**1. 在 SWIG 接口文件中启用异常处理**:
```swig
%include <rust/exception.i>
%exception { RUST_EXCEPTION_HANDLER }
```

**2. 生成的 Rust 代码可以检查错误**:
```rust
let result = obj.method_that_may_throw();
if let Some(error) = SwigError::check() {
    println!("Error: {}", error);
}
```

**3. 使用 Result 模式**:
```rust
fn safe_call() -> Result<(), SwigError> {
    obj.method_that_may_throw();
    swig_check_error!()
}
```

### 异常类型映射

| C++ 异常类型 | Rust 错误码 |
|-------------|------------|
| `std::bad_alloc` | `MemoryError` |
| `std::runtime_error` | `RuntimeError` |
| `std::invalid_argument` | `ValueError` |
| `std::out_of_range` | `IndexError` |
| `std::overflow_error` | `OverflowError` |
| 其他 `std::exception` | `RuntimeError` |
| 未知异常 | `UnknownError` |

### 测试验证

- 测试文件: `test_exception.i`
- 编译通过
- 生成正确的 Rust 代码包含 SwigError 类型

---

## 2026-04-14 本次会话修复进度

### 已完成的修复

#### 1. 添加 `cleanRustName()` 函数 ✅
- **目的**: 清理 Rust 标识符中的无效字符（如 `::`、`<`、`>`、`*`、`&` 等）
- **位置**: `Source/Modules/rust.cxx` 第 1232 行附近
- **功能**: 将 `std::string` 转换为 `std_string`，处理模板参数等

#### 2. 修复 `emitOverloadSuffix()` ✅
- **问题**: 生成的后缀包含 `std::string`，导致无效的 Rust 标识符
- **修复**: 
  - 特殊处理 `std::string` 类型，返回 `_string` 而非 `_std::string`
  - 对其他复杂类型使用 `cleanRustName()` 清理

#### 3. 修复参数名问题 ✅
- **问题**: 静态成员变量参数名如 `BasicTypes::static_counter` 是无效的 Rust 语法
- **修复**: 在 `emitRustSafeWrapper()` 和 `emitRustConstructor()` 中使用 `cleanRustName()` 清理参数名

#### 4. 修复构造函数命名问题 ✅
- **问题**: 重载构造函数名如 `new_std::string` 包含无效字符
- **修复**: 使用 `cleanRustName()` 清理构造函数后缀

### 待修复的问题（通过 rustc 检测）

运行 `rustc test_swig.rs` 发现以下编译错误：

#### 1. FFI 函数重复声明
```
error[E0428]: the name `Rust_global_check_status__SWIG_0` is defined multiple times
```
- **原因**: 全局函数 `global_check_status` 的 FFI 声明被重复生成
- **位置**: `test_swig.rs` 第 366 行和 370 行

#### 2. Trait 方法重复定义
```
error[E0428]: the name `get_value` is defined multiple times
```
- **原因**: `OverloadTestTrait` 中 const 和非 const 版本的 `get_value` 方法名相同
- **位置**: `test_swig.rs` 第 618-619 行
- **解决方案**: 需要修改 `emitRustTrait()` 中的方法计数逻辑，为 const 方法添加 `_const` 后缀

#### 3. 全局函数重复定义
```
error[E0428]: the name `global_check_status_int` is defined multiple times
```
- **原因**: 全局函数的包装被重复生成
- **位置**: `test_swig.rs` 第 1285 行和 1289 行

### 其他待修复问题

#### 4. Director trait 方法名问题
- **问题**: 方法名如 `on_message_std::string` 包含 `::`
- **影响**: 无效的 Rust 标识符
- **位置**: `CallbackTestDirector` trait

#### 5. VTable 字段名问题
- **问题**: 字段名如 `on_message_std::string` 包含 `::`
- **影响**: 无效的 Rust 标识符
- **位置**: `CallbackTestVTable` struct

#### 6. 未定义类型 `std_string`
- **问题**: 代码使用了 `std_string` 类型但未定义
- **影响**: 编译错误
- **解决方案**: 需要生成 `std_string` 类型定义或使用 `String` 类型

---

## 解决方案分析 (2026-04-14)

### 问题 1 & 3: FFI 函数重复声明 & 全局函数重复定义

**根因分析**：
- 可能是同一个函数节点被处理了两次（通过不同的 handler 路径）
- 或者缺少"已处理"标记的去重机制

**解决方案**：
1. 在 `functionWrapper()` 中添加 `wrap:name` 缓存，跳过已处理的函数
2. 检查是否有多个 handler 同时触发了同一函数的处理

---

### 问题 2: Trait 中 const/non-const 方法重复

**根因分析**：
```cpp
// test_swig.h
int get_value();           // 非const
int get_value() const;     // const版本，返回双倍值
```

当前 `method_counts` 使用 `sym:name` 作为 key，两者都是 `get_value`，导致：
- `total_count = 2`（认为有 2 个重载）
- `emitOverloadSuffix(params)` 返回空字符串（参数列表相同）
- 结果：两个方法都叫 `get_value`，产生 Rust 编译错误

**设计决策：发出警告，要求用户自己重命名**

理由：
1. **用户控制权**：用户对 const/non-const 版本可能有不同的语义需求
2. **避免意外命名**：自动添加 `_const` 后缀可能产生不直观的方法名
3. **与 SWIG 惯例一致**：其他语言绑定也经常要求用户显式处理复杂重载

**实现方案**：
```
Warning: Const/non-const overload detected for 'OverloadTest::get_value'
- Non-const version: get_value()
- Const version: get_value() const
In Rust, trait methods cannot be distinguished by self type alone.
Please use %rename to give unique names, e.g.:
  %rename(get_value_const) OverloadTest::get_value() const;
```

**修改位置**：`emitRustTrait()` 中添加检测和警告逻辑

---

### 问题 4 & 5: Director trait/VTable 方法名包含 `::`

**根因分析**：
`classDirectorMethod()` 中生成的后缀如 `_std::string` 包含非法字符

**解决方案**：
- 在 `classDirectorMethod()` 中，对生成的后缀调用 `cleanRustName()` 进行清理

---

### 问题 6: 未定义类型 `std_string`

**根因分析**：
代码中生成了 `std_string` 类型（如 `*mut std_string`），但没有对应的类型定义

**解决方案**：
1. **添加类型定义**：生成一个 `std_string` 结构体包装 `std::string`
2. **或者改用 String**：修改类型映射，直接使用 Rust 的 `String` 类型
3. **或者使用不透明指针**：用 `*mut c_void` 作为不透明句柄

---

### 推荐的修复顺序

1. **先实现 const/non-const 重载警告机制**（问题 2）- 让用户能明确处理
2. **然后修复 FFI/函数重复**（问题 1、3）- 需要调试确认根因
3. **再修复 Director 命名**（问题 4、5）- 在已有 `cleanRustName` 基础上应用
4. **最后处理 `std_string` 类型**（问题 6）- 设计决策



---

## 2026-04-14 本次会话修复记录 ✅

### 已完成的修复

#### 1. FFI 函数重复声明 ✅
- **问题**: `Rust_global_check_status__SWIG_0` 定义了两次
- **原因**: 全局函数在命名空间外和 `MYTEST` 命名空间内都有定义
- **修复**: 
  - 添加 `generated_ffi_names` Hash 跟踪已生成的 FFI 函数
  - 在 `emitRustFFIDeclaration()` 中添加去重检查

#### 2. 全局函数重复定义 ✅
- **问题**: `global_check_status_int` 定义了两次
- **原因**: 同上，命名空间内外都有同名函数
- **修复**: 
  - 添加 `generated_wrapper_names` Hash 跟踪已生成的包装函数
  - 在 `emitRustSafeWrapper()` 中添加去重检查

#### 3. const/non-const 方法重复 ✅
- **问题**: `get_value()` 有 const 和非 const 版本，生成两个同名方法
- **修复**: 
  - 修复 `SwigType_isconst(decl)` 检测逻辑（之前错误地检查字符串）
  - 添加 `generated_signatures` Hash 跟踪已生成的方法签名
  - 当遇到相同签名的 const/non-const 重载时，**只保留第一个版本**

#### 4. Director/VTable 方法名中的 `::` 字符 ✅
- **问题**: 方法名如 `on_message_std::string` 包含无效的 `::` 字符
- **修复**: 
  - 对 `callback_suffix`、`vtable_field_suffix`、`trait_method_suffix` 使用 `cleanRustName()` 清理

#### 5. `std_string` 未定义类型 ✅
- **问题**: 代码中生成了 `std_string` 类型但未定义
- **修复**: 
  - 在 `getRustUserType()` 中添加 `std::string` 的特殊处理
  - 当类型是 `std::string` 时返回 `String` 而非 `std_string`

---

## 当前 SWIG Rust 代码生成问题汇总 (2026-04-14)

修复了上述问题后，还有以下新问题需要处理：

### 1. String 类型冲突 🔴 P0

**问题**:
```rust
fn whoami(&self) -> String {
    String { ptr: unsafe { ffi::Rust_Base_whoami__SWIG_0(self.ptr) } }
}
```

`String` 是 Rust 标准库类型，不能用作结构体包装器。

**解决方案**:
- 生成一个自定义类型如 `SwigString` 包装 `std::string`
- 或者对于返回 `std::string` 的方法，直接返回 `String`（需要正确的类型转换逻辑）

**位置**: `emitRustImpl()` 中处理返回值类型的逻辑

---

### 2. 继承关系未正确实现 🔴 P0

**问题**:
```rust
pub trait DerivedTrait: BaseTrait { ... }
impl DerivedTrait for Derived { ... }  // 错误：Derived 没有实现 BaseTrait
```

**原因**: `Derived` 结构体只持有自己的指针，没有同时实现 `BaseTrait`

**解决方案**:
- 为派生类同时生成基类 trait 的实现
- 或者使用 Deref 模式让 `Derived` 可以当作 `Base` 使用

**位置**: `emitRustImpl()` 中处理继承关系

---

### 3. bool 类型转换问题 🟡 P1

**问题**:
```rust
// test_swig.rs:504
unsafe { ffi::Rust_BasicTypes_bool_val_set__SWIG_0(self.ptr, bool_val) }
// 错误：expected `u8`, found `bool`
```

FFI 函数期望 `u8` (c_uchar)，但传入了 Rust `bool`。

**解决方案**:
- 在生成调用代码时添加类型转换：`bool_val as u8` 或 `u8::from(bool_val)`
- 或者修改 typemap 让 FFI 层使用 `bool` 类型

**位置**: `emitRustImpl()` 中生成 FFI 调用的逻辑

---

### 修复优先级

| 优先级 | 问题 | 影响 |
|--------|------|------|
| P0 | String 类型冲突 | 编译错误 |
| P0 | 继承关系未实现 | 编译错误 |
| P1 | bool 类型转换 | 编译错误 |

---

## 最后更新
2026-04-14 (命名空间支持分析与限制说明)

### 本次会话完成的工作 ✅

#### 1. C++ Namespace → Rust mod 映射分析
- 确认映射关系：C++ `namespace` → Rust `pub mod`
- 嵌套命名空间支持：`Outer::Inner` → `pub mod Outer { pub mod Inner { ... } }`

#### 2. `addOpenMod`/`addCloseMod` 改进
- 支持嵌套命名空间（使用 `.` 分隔符）
- 正确计算嵌套深度

#### 3. `emitRustSafeWrapper` 去重逻辑修复
- 使用 `namespace::symname` 作为去重 key
- 不同命名空间的同名函数不再被错误去重

#### 4. 发现核心限制
- **Rust 不允许重复定义同一个 `mod`**
- C++ 可以多次扩展同名命名空间，但 Rust 不行
- 这是语言级别的限制，无法在当前架构下完美解决

#### 5. 添加限制说明文档
- 在 `Lib/rust/rust.swg` 添加详细说明
- 在 `RUST_DESIGN.md` 添加 "14.7 当前实现限制" 章节

### 当前命名空间支持状态

| 功能 | 状态 | 说明 |
|------|------|------|
| `-namespace <name>` 选项 | ✅ | 将所有内容包装到一个模块 |
| 嵌套命名空间解析 | ✅ | 支持 `.` 分隔符 |
| `%feature("nspace")` | ⚠️ 默认禁用 | 会生成重复 `mod` 定义 |

### 推荐使用方式

1. 使用 `-namespace mylib` 将所有内容包装到统一模块
2. 使用 `%rename` 添加前缀避免命名冲突
3. 分开处理不同命名空间（多个 SWIG 运行）

### 待后续改进

完整命名空间支持需要架构级改进：
1. 收集阶段：遍历 AST 收集所有命名空间及其内容
2. 组织阶段：构建命名空间树，合并同名命名空间
3. 生成阶段：每个命名空间生成一个 `pub mod` 块

---

## 2026-04-15 本次会话：SwigString FFI 实现与 Bug 修复

### 1. getRustUserType() use-after-free 修复 ✅

**问题**: 在 `Source/Modules/rust.cxx` 的 `getRustUserType()` 函数中，`base` 指针在处理 `std::string` 和 `char*` 后被删除，但 switch 语句的 `T_USER` 和 `default` 分支仍尝试使用它，导致访问违规崩溃 (0xC0000005)。

**症状**: 处理包含 `size_t` 类型或 `std::string` 类定义的接口文件时 SWIG 崩溃。

**修复**: 
```cpp
// 在删除 base 后设置为 NULL
Delete(base);
base = NULL;

// 在 switch 的 T_USER 和 default 分支中重新获取 base
String *fresh_base = SwigType_base(t);
String *clean = cleanTypeName(fresh_base);
Delete(fresh_base);
return clean;
```

### 2. 添加 `rustcode` Section 注册 ✅

**问题**: `Lib/rust/std_string.i` 使用 `%insert("rustcode")` 但 SWIG 不认识这个 section，报错 "Unknown target 'rustcode' for %insert directive"。

**修复**: 在 `Source/Modules/rust.cxx` 的 `top()` 函数中添加：
```cpp
Swig_register_filebyname("rustcode", f_wrapper_code);
```

**用途**: 允许 `%insert("rustcode")` 将 Rust 代码直接插入到生成的 `.rs` 文件的安全包装层中。

### 3. SwigString 类型映射基础实现 ✅

**文件**: `Lib/rust/std_string.i`

**设计决策**:
- `std::string` 映射到自定义 `SwigString` 类型（而非直接用 Rust `String`）
- 支持完整的 `std::string` 接口
- 提供 `From/Into` trait 实现与 Rust `String` 的双向转换

**类型映射表**:
| C++ 类型 | Rust 用户类型 | FFI 类型 |
|---------|-------------|----------|
| `std::string` | `SwigString` | `*mut c_void` |
| `const std::string&` | `&SwigString` | `*const c_void` |
| `std::string&` | `&SwigString` | `*mut c_void` |
| `std::string*` | `Option<SwigString>` | `*mut c_void` |
| `const std::string*` | `Option<&SwigString>` | `*const c_void` |

**已生成的 trait 实现**:
- `Display`, `Debug` - 格式化输出
- `PartialEq`, `Eq`, `PartialOrd`, `Ord` - 比较
- `Hash` - 哈希
- `Default` - 默认值
- `From<String>`, `From<&str>` - 从 Rust 字符串创建
- `From<SwigString> for String` - 转换为 Rust 字符串
- `Into<String>` - 隐式转换
- `Deref`, `Borrow<[u8]>` - 字节访问

### 4. SwigString FFI 完整实现 ✅ (2026-04-15)

**文件**: `Lib/rust/std_string.i`

**设计改进**: 使用 typemap 机制而非硬编码

**新增 Typemap**:
| Typemap | 用途 | 示例 |
|---------|------|------|
| `rsin` | Rust 参数传递方式 | `$input.ptr` |
| `rsout` | Rust 返回值包装 | `SwigString { ptr: $result }` |

**C++ 辅助函数** (已生成):
```cpp
extern "C" {
    void* SwigString_new();
    void* SwigString_from_bytes(const char* data, size_t len);
    void* SwigString_from_c_str(const char* s);
    void SwigString_delete(void* ptr);
    const char* SwigString_c_str(const void* ptr);
    const char* SwigString_data(const void* ptr);
    size_t SwigString_len(const void* ptr);
    int SwigString_is_empty(const void* ptr);
    void SwigString_clear(void* ptr);
    void SwigString_append(void* ptr, const char* data, size_t len);
    size_t SwigString_capacity(const void* ptr);
    void SwigString_reserve(void* ptr, size_t capacity);
    int SwigString_compare(const void* ptr1, const void* ptr2);
    void* SwigString_clone(const void* ptr);
    void SwigString_resize(void* ptr, size_t new_len, char fill_char);
    void* SwigString_substr(const void* ptr, size_t pos, size_t len);
    long SwigString_find(const void* ptr, const char* needle, size_t pos);
    long SwigString_rfind(const void* ptr, const char* needle, size_t pos);
    void SwigString_push_back(void* ptr, char c);
    int SwigString_pop_back(void* ptr);
    char SwigString_at(const void* ptr, size_t index);
    void SwigString_set_at(void* ptr, size_t index, char c);
}
```

**Rust SwigString 实现**:
```rust
pub struct SwigString {
    ptr: *mut std::ffi::c_void,
}

impl SwigString {
    pub fn new() -> Self;
    pub fn from_str(s: &str) -> Self;
    pub fn from_bytes(bytes: &[u8]) -> Self;
    pub fn len(&self) -> usize;
    pub fn is_empty(&self) -> bool;
    pub fn as_bytes(&self) -> &[u8];
    pub fn into_string(self) -> String;
    pub fn to_string_lossy(&self) -> String;
    pub fn append(&mut self, bytes: &[u8]);
    pub fn clear(&mut self);
    pub fn reserve(&mut self, capacity: usize);
    pub fn find(&self, needle: &str, pos: usize) -> Option<usize>;
    // ... 更多方法
}
```

### 5. rust.cxx 中的 SwigString 处理修复 🔄 (进行中)

**问题**: `rust.cxx` 中硬编码了 SwigString 的处理逻辑

**当前修复**:
- 参数传递: `.as_ptr() as *const c_char` → `.ptr`
- 返回值包装: 需要构造 `SwigString { ptr: ... }`

**待修复位置** (8 处):
- `L3273`, `L3302`, `L3340`, `L3368`, `L3396` - emitRustImpl 中的参数传递
- `L3594`, `L3615` - emitRustImpl 中的返回值处理
- `L3975` - 其他位置

**正确的设计方向**:
应该通过 typemap 机制来处理，而不是在 rust.cxx 中硬编码：
```swig
%typemap(rsin) std::string "$input.ptr"
%typemap(rsout) std::string "SwigString { ptr: $result }"
```

然后 rust.cxx 应该使用 `Swig_typemap_lookup("rsin", ...)` 来获取转换代码。

### 6. 测试验证

**成功的测试**:
- `simple_test.i` - 基本类型
- `size_t_test.i` - `size_t` 类型（之前崩溃，现在通过）
- `string_test.i` - `std::string` 类定义（之前崩溃，现在通过）
- `test_swigstring.i` - 完整的 SwigString 生成

**生成的代码示例** (`test_swigstring.rs`):
```rust
pub struct SwigString {
    ptr: *mut std::ffi::c_void,
}

impl SwigString {
    pub const NPOS: usize = usize::MAX;
    pub fn new() -> Self { ... }
    pub fn from_c_str(s: &str) -> Self { ... }
    pub fn as_ptr(&self) -> *const std::os::raw::c_char { ... }
    pub fn len(&self) -> usize { ... }
    pub fn into_string(self) -> String { ... }
}

impl From<String> for SwigString { ... }
impl From<SwigString> for String { ... }
```

---

## 后续工作计划

### P0 - 必须
1. **SwigString 参数/返回值处理** 🔄 进行中
   - 需要在 rust.cxx 中使用 typemap 而非硬编码
   - 修复 `.as_ptr()` → `.ptr` 的参数传递
   - 修复返回值构造 `SwigString { ptr: ... }`
   
2. **继承关系修复** - 派生类需要同时实现基类 trait

### P1 - 重要
3. **bool 类型转换** - FFI 调用时添加 `bool as u8` 转换
4. **std::string 成员方法 FFI** - 完整的字符串操作方法

### P2 - 增强
5. **运算符映射完善** - 更多运算符 trait
6. **双向转换优化** - UTF-8 验证和零拷贝

---

## 设计改进建议

### Typemap 优先原则

**问题**: 当前 rust.cxx 中有大量硬编码的类型检测逻辑，如：
```cpp
bool is_string_param = pbase && (Strstr(pbase, "basic_string") || 
                                 Strcmp(pbase, "string") == 0 ||
                                 Strcmp(pbase, "std::string") == 0);
```

**建议**: 应该通过 typemap 机制来处理类型转换：
```swig
// 在 std_string.i 中定义
%typemap(rsin) std::string "$input.ptr"
%typemap(rsout) std::string "SwigString { ptr: $result }"
```

```cpp
// 在 rust.cxx 中使用
String *rsin = Swig_typemap_lookup("rsin", p, lname, 0);
if (rsin) {
    // 使用 typemap 中的转换代码
    Replaceall(rsin, "$input", pname);
    Printf(f_wrapper_code, "%s", rsin);
} else {
    // 默认行为
    Printf(f_wrapper_code, "%s", pname);
}
```

**优点**:
- 类型处理逻辑集中在 typemap 文件中
- 用户可以自定义类型转换
- 符合 SWIG 的设计哲学
- 减少 rust.cxx 中的硬编码

---

## 最后更新
2026-04-15 (继承链修复、类型映射修复、枚举返回类型修复)

### Phase 24: 继承链 trait 实现修复 ✅

**问题**: `GrandDerived` 类继承了 `Derived`，`Derived` 继承了 `Base`。生成的代码中：
- `GrandDerived` 只实现了 `DerivedTrait`
- 缺少 `BaseTrait` 实现
- 导致 Rust 编译错误：`the trait bound 'GrandDerived: BaseTrait' is not satisfied`

**原因**: 原 `emitRustImpl()` 只为直接基类生成 trait 实现，没有递归处理整个继承链

**修复** (`Source/Modules/rust.cxx`):
1. 收集所有祖先类（使用工作列表算法）
2. 为每个祖先类生成 `impl AncestorTrait for Derived`

**生成的代码示例**:
```rust
// GrandDerived 现在正确实现了所有祖先 trait
impl GrandDerivedTrait for GrandDerived { ... }
impl DerivedTrait for GrandDerived { ... }
impl BaseTrait for GrandDerived { ... }  // 新增！
```

---

### Phase 25: long/unsigned long 类型映射修复 ✅

**问题**: Windows 上 `long` 是 4 字节，但 typemap 映射到 `i64`（8 字节）
- FFI 层 `c_long` 在 Windows 上是 `i32`
- 导致类型不匹配编译错误

**修复** (`Lib/rust/rusttype.swg`):
```swig
// 修改前
%typemap(rusttype) long "i64"
%typemap(rusttype) unsigned long "u64"

// 修改后 - 使用平台自适应类型
%typemap(rusttype) long "c_long"
%typemap(rusttype) unsigned long "c_ulong"
```

同样修复了 `size_t` 和 `ptrdiff_t`:
```swig
%typemap(rusttype) size_t "usize"
%typemap(rsffitype) size_t "usize"  // 统一使用 usize
```

---

### Phase 26: 枚举返回类型 FFI 转换修复 ✅

**问题**: 类方法返回枚举类型时，FFI 返回 `c_int`，但方法签名要求枚举类型
```rust
fn get_status(&self) -> EnumClass_Status {
    unsafe { ffi::Rust_EnumClass_get_status__SWIG_0(self.ptr) }  // 错误：返回 i32
}
```

**修复** (`Source/Modules/rust.cxx`):
1. 在 `emitRustImpl()` 中添加枚举类型检测
2. 使用 `std::mem::transmute::<i32, EnumType>()` 进行转换

**生成的代码**:
```rust
fn get_status(&self) -> EnumClass_Status {
    unsafe { std::mem::transmute::<i32, EnumClass_Status>(
        ffi::Rust_EnumClass_get_status__SWIG_0(self.ptr)
    )}
}
```

**枚举检测逻辑**:
1. `SwigType_isenum()` - SWIG 内置检测
2. 类型字符串检查 `"enum "` 前缀
3. 排除法：非基本类型、非指针、非引用、非用户类的自定义类型

---

### 测试验证通过的用例 ✅

| 测试文件 | 说明 | 状态 |
|---------|------|------|
| `inherit_basic.rs` | 三层继承 | ✅ |
| `primitive_types_simple.rs` | 基本类型 + long | ✅ |
| `class_methods.rs` | 类方法 | ✅ |
| `overload_test.rs` | 函数重载 | ✅ |
| `enums_test.rs` | 枚举类型 | ✅ |
| `template_test.rs` | 模板实例化 | ✅ |

---

### 待完成的工作

| 优先级 | 任务 | 状态 |
|--------|------|------|
| P2 | 成员变量 getter 返回引用问题 | ✅ 已修复 |
| P3 | std::string 高级方法 FFI | ✅ 完成 |
| P3 | std::wstring 支持 | ✅ 完成 |
| P3 | 其他 STL 容器绑定 | 待实现 |
| P4 | 编写 Rust 绑定文档 | 待实现 |
| P4 | 代码重构（移除硬编码） | 待实现 |

---

## 2026-04-15 本次会话：Phase 27 成员变量 getter 返回引用修复 ✅

### 问题描述

当 C++ 类的成员变量是 `std::string` 类型时，生成的成员变量 getter 存在类型不匹配问题：

**问题代码**:
```rust
// 错误的生成代码
pub fn name(&self) -> &SwigString {
    unsafe { ffi::Rust_StringMember_name_get__SWIG_0(self.ptr) }  // 返回 *const c_void
}
```

**根因**:
- C++ getter 返回 `const std::string&`（内部成员的引用）
- 类型映射定义 `rusttype` 为 `&SwigString`
- FFI 函数返回 `*const c_void`（原始指针）
- Rust 无法直接将原始指针作为引用返回

### 修复方案

成员变量 getter 不应返回引用（Rust 无法安全持有 C++ 内部数据的引用），改为返回 owned 类型并克隆字符串：

**修复后的代码**:
```rust
pub fn name(&self) -> SwigString {
    SwigString { ptr: unsafe { swig_string_ffi::SwigString_clone(ffi::Rust_StringMember_name_get__SWIG_0(self.ptr)) } }
}
```

### 修改的文件

`Source/Modules/rust.cxx`:
1. 添加 `return_is_swigstring_ref` 变量检测 `&SwigString` 返回类型
2. 对于成员变量 getter，将 `&SwigString` 转换为 `SwigString`（owned）
3. 使用 `SwigString_clone` 复制字符串，避免持有 C++ 内部指针

### 测试验证

```
16 passed, 0 failed, 0 skipped
```

---

## 2026-04-15 本次会话：Phase 28 std::string 高级方法 ✅

### 新增的高级方法

| 方法 | 说明 |
|------|------|
| `insert`, `insert_str`, `insert_char` | 在位置插入字节/字符串/字符 |
| `erase` | 删除指定范围的字符 |
| `replace`, `replace_str` | 替换指定范围的字符 |
| `find_first_of` / `find_last_of` | 查找字符集中的字符 |
| `find_first_not_of` / `find_last_not_of` | 查找不在字符集中的字符 |
| `shrink_to_fit` | 收缩容量 |
| `front`, `back` | 获取首/尾字符 |
| `swap` | 交换两个字符串内容 |

### 修改的文件

- `Lib/rust/std_string.i` - 添加高级方法的 C++ FFI 函数和 Rust 实现

---

## 2026-04-15 本次会话：Phase 29 std::wstring 支持 ✅

### 创建的文件

| 文件 | 说明 |
|------|------|
| `Lib/rust/std_wstring.i` | std::wstring 类型映射和 SwigWString 包装 |
| `Examples/test-suite/rust/std_wstring_test.i` | 测试文件 |

### SwigWString 功能

- **平台检测**: `wchar_size()` 返回 2 (Windows) 或 4 (Unix)
- **数据访问**: `from_utf16()`, `from_utf32()`, `as_u16_slice()`, `as_u32_slice()`
- **trait 实现**: Default, Drop, Clone, Debug, PartialEq, Eq, PartialOrd, Ord

### 修改的文件

- `Source/Modules/rust.cxx` - 添加 SwigWString 类型检测
- `Lib/rust/rusttype.swg` - 添加 wchar_t/wchar_t* 类型映射

### 设计要点

- 不提供与 Rust String 的转换
- wchar_t 映射到 u16（Windows 平台）
- 支持 UTF-16 和 UTF-32 输入

---

## 2026-04-15 待重构项

### 问题：rust.cxx 硬编码不符合 SWIG 设计模式

**当前做法** (不符合其他语言模块的做法):
- 在 `rust.cxx` 中硬编码 SwigString/SwigWString 的检测和包装

**其他语言的做法**:
- `.cxx` 只处理字面量转义
- `std::string`/`std::wstring` 类型映射都在 `.i` 文件中

**重构计划** (详见 RUST_TODO.md Phase 30):
1. 完善 `rsin`/`rsout` typemap
2. 移除 rust.cxx 中的硬编码
3. 改用统一的 typemap 查找机制

