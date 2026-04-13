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
