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

---

## 2026-04-15 Rust 头部属性插入功能 ✅

### 需求背景

生成的 Rust 代码经常产生编译警告，如：
- `non_camel_case_types` - C++ 风格的类型命名
- `non_snake_case` - C++ 风格的函数命名
- `dead_code` - 未使用的代码
- `unused_imports` - 未使用的导入

需要一种机制在生成的 `.rs` 文件开头插入 `#![allow(...)]` 属性来屏蔽这些警告。

### 实现方案

提供两种方式插入头部属性：

#### 方式 1：命令行选项 `-allow-warnings`

```bash
swig -rust -c++ -allow-warnings -module mymodule input.i
```

自动插入：
```rust
#![allow(non_camel_case_types)]
#![allow(non_snake_case)]
#![allow(dead_code)]
#![allow(unused_imports)]
#![allow(unused_variables)]
#![allow(unused_mut)]
```

#### 方式 2：`%pragma(rust) header="..."`

```swig
%module mymodule
%pragma(rust) header="#![allow(non_camel_case_types, non_snake_case, dead_code, unused_imports)]"
```

插入用户自定义的头部代码。

### 修改的文件

**`Source/Modules/rust.cxx`**:
1. 添加成员变量 `rust_header` 和 `allow_warnings_flag`
2. 在构造函数中初始化
3. 添加 `-allow-warnings` 命令行选项
4. 实现 `pragmaDirective()` 函数处理 `%pragma(rust) header="..."`
5. 在 `emitRustFile()` 中**先于 banner** 输出头部内容（确保在文件绝对第一行）

### 生成的代码结构

```rust
#![allow(non_camel_case_types)]
#![allow(non_snake_case)]
#![allow(dead_code)]
#![allow(unused_imports)]
#![allow(unused_variables)]
#![allow(unused_mut)]

// This file was automatically generated by SWIG (https://www.swig.org).
// Version 4.5.0
// ...

mod ffi {
    // ...
}
```

### 注意事项

- `#![allow(...)]` 在 banner 注释**之前**，确保是文件的绝对第一行
- 如果同时使用 `%pragma` 和 `-allow-warnings`，`%pragma` 优先

---

## 2026-04-15 Phase 32: 静态成员函数重复生成修复 ✅

### 32.1 问题描述

对于静态成员函数如 `StaticA::get_version()`，SWIG 生成了两个 C 包装函数：
- `Rust_StaticA_get_version__SWIG_0()` - 带类名前缀
- `Rust_get_version__SWIG_0()` - 不带前缀

如果另一个类有相同签名的静态方法，就会产生命名冲突。

### 32.2 问题原因

`staticmemberfunctionHandler()` 中同时调用了：
1. `Language::staticmemberfunctionHandler(n)` - 基类内部会调用 `functionWrapper()`
2. `functionWrapper(n)` - 再次生成包装函数

### 32.3 修复内容

**文件**: `Source/Modules/rust.cxx`

1. **移除重复调用**: 不再手动调用 `functionWrapper(n)`，只保留基类调用
2. **修正静态方法命名**: 从 `symname` 中移除类名前缀，使静态方法名符合 Rust 惯用方式
   - 修改前: `impl StaticA { pub fn StaticA_get_version() -> ... }`
   - 修改后: `impl StaticA { pub fn get_version() -> ... }`

### 32.4 修复验证

两个类有同名静态方法时：
```rust
// FFI 层 - 函数名包含类名，不会冲突
pub fn Rust_ClassA_get_version__SWIG_0() -> *const c_char;
pub fn Rust_ClassB_get_version__SWIG_0() -> *const c_char;

// 安全包装层 - 在不同 impl 块中，方法名可以相同
impl ClassA { pub fn get_version() -> &str { ... } }
impl ClassB { pub fn get_version() -> &str { ... } }
```

---

## 2026-04-15 Phase 33: 数组参数类型处理修复 ✅

### 33.1 问题描述

数组参数 `const int para[]` 被错误转换：
- 生成的类型如 `int const []` 或 `&[int const []]` 是无效的 Rust 类型
- 应该映射为 `*const i32`（因为 C 中数组参数 decay 到指针）

### 33.2 修复内容

**文件**: `Source/Modules/rust.cxx`

1. **getRustUserType() 数组处理**: 添加 `SwigType_isarray()` 检测，数组类型转换为指针
   - `const int arr[]` → `*const i32`
   - `int arr[]` → `*mut i32`

2. **getRustUserType() 指针处理**: 为基本类型的指针生成精确类型
   - `const int* p` → `*const i32`
   - `int* p` → `*mut i32`
   - 类类型指针仍使用 `*mut c_void`

3. **emitRustSafeWrapper() typemap fallback**: 当 typemap 返回泛型指针时，尝试获取更精确类型

4. **emitOverloadSuffix() 数组处理**: 数组参数生成正确的后缀如 `_cint_ptr`

5. **emitRustSafeWrapper() 跳过逻辑修复**: 全局指针参数函数不再被错误跳过

6. **Lib/rust/rusttype.swg**: 移除数组类型的 rusttype typemap，使用 fallback 机制

### 33.3 修复验证

```rust
// const 指针参数
void test_const(const int* p);
// 生成: pub fn test_const_int(p: *const i32)

// 非 const 指针参数
void test_nonconst(int* p);
// 生成: pub fn test_nonconst_int(p: *mut i32)

// 数组参数 (decay 到指针)
void test_array(const int arr[]);
// 生成: pub fn test_array_cint_ptr(arr: *const i32)
```

---

## 2026-04-16 Phase 34: C++ Director 数组参数语法修复 ✅

### 34.1 问题描述

C++ Director 类中数组参数生成了无效语法：

```cpp
// 错误生成
void SwigDirector_YDListener::notifyGroupMaxOrderRef(int const [] groupMaxOrderRef)

// 正确应该是
void SwigDirector_YDListener::notifyGroupMaxOrderRef(int const groupMaxOrderRef[])
```

### 34.2 问题原因

在 `classDirectorMethod()` 中，使用 `SwigType_str(type, 0)` 获取类型字符串，然后手动拼接参数名：

```cpp
String *pt = SwigType_str(Getattr(p, "type"), 0);  // 输出 "int const []"
String *pn = Getattr(p, "name");                    // 输出 "groupMaxOrderRef"
Printf(f_directors, "%s %s", pt, pn);               // 输出 "int const [] groupMaxOrderRef" ❌
```

### 34.3 修复内容

**文件**: `Source/Modules/rust.cxx`

使用 `SwigType_str(type, name)` 让 SWIG 正确放置数组参数名：

```cpp
String *pt = SwigType_str(pt_raw, pn);  // 输出 "int const groupMaxOrderRef[]" ✅
Printf(f_directors, "%s", pt);
```

### 34.4 修复位置

- `classDirectorMethod()` 中的 C++ 方法声明生成
- `classDirectorMethod()` 中的 C++ 方法实现生成

### 34.5 修复验证

```cpp
// yd_wrap.h
virtual void notifyGroupMaxOrderRef(int const groupMaxOrderRef[]);

// yd_wrap.cxx
void SwigDirector_YDListener::notifyGroupMaxOrderRef(int const groupMaxOrderRef[]) {
    if (vtable_ && vtable_->notifyGroupMaxOrderRef_cint_ptr) {
        vtable_->notifyGroupMaxOrderRef_cint_ptr(swig_rust_director_, groupMaxOrderRef);
    }
}
```

---

## 当前状态总结 (2026-04-16)

### 已完成的 Phase

| Phase | 内容 | 状态 |
|-------|------|------|
| Phase 1-15 | 基础框架和核心功能 | ✅ 完成 |
| Phase 16 | 运算符 Trait 实现 | ✅ 完成 |
| Phase 17 | STL 容器绑定 | ✅ 完成 |
| Phase 18 | STL 容器扩展 | ✅ 完成 |
| Phase 19 | 异常处理支持 | ✅ 完成 |
| Phase 20-22 | 代码生成修复 | ✅ 完成 |
| Phase 23 | SwigString FFI 实现 | ✅ 完成 |
| Phase 24-26 | 类型映射修复 | ✅ 完成 |
| Phase 27-29 | std::wstring 和高级方法 | ✅ 完成 |
| Phase 30 | 代码重构计划 | 待实现 |
| Phase 31 | Rust 头部属性插入 | ✅ 完成 |
| Phase 32 | 静态成员函数重复生成修复 | ✅ 完成 |
| Phase 33 | 数组参数类型处理修复 | ✅ 完成 |
| Phase 34 | C++ Director 数组参数语法修复 | ✅ 完成 |
| Phase 35 | Typemap 变量替换 ($rustclassname) | ✅ 完成 |
| Phase 36 | Rust 关键字冲突处理 | ✅ 完成 |

### 待完成的工作

1. **C variadic 参数处理**: 跳过或警告 variadic 函数
2. **比较运算符链问题**: 处理 Rust 不支持的运算符链式比较
3. **文档编写**: `Doc/Manual/Rust.html`
4. **测试完善**: 添加更多测试用例

### 构建说明

```bash
# Windows (Visual Studio)
cmake -B build -S . -A x64 -DWITH_PCRE=OFF
cmake --build build --config Release

# 运行 SWIG
$env:SWIG_LIB="D:\code\cpp\swig\Lib"
.\build\Release\swig.exe -rust -c++ -allow-warnings -module mymodule mymodule.i
```

---

## 2026-04-16 本次会话：Typemap 变量替换与关键字冲突修复

### Phase 35: Typemap 变量替换 ($rustclassname) ✅

**问题背景**:
- 生成 yd.rs 时出现 `$*1_type`、`$&1_type` 等 typemap 变量未被替换
- 导致 Rust 编译错误：`expected type, found '$'`

**参考实现**: C# 的 `$csclassname` 和 Java 的 `$javaclassname`

**解决方案**:

1. **新增 `substituteRustClassname()` 函数** (`rust.cxx:1236-1300`):
   ```cpp
   bool substituteRustClassname(SwigType *pt, String *tm) {
     // 处理 $*rustclassname - 移除一层指针/引用
     // 处理 $&rustclassname - 添加一层指针
     // 处理 $rustclassname - 直接使用类型名
   }
   ```

2. **新增 `getRustTypeWithSubstitution()` 函数** (`rust.cxx:1302-1326`):
   - 封装 typemap 查找 + 变量替换
   - 如果替换后仍有 `$` 符号，fallback 到 `getRustUserType()`

3. **更新 typemap 文件**:
   - `Lib/rust/rusttype.swg`: 使用 `$rustclassname` 变量
   - `Lib/rust/rustrun.swg`: 使用 `$rustclassname` 变量

4. **修复所有使用 typemap 的位置**:
   - `emitRustSafeWrapper()` 参数类型
   - `functionWrapper()` 返回类型
   - `emitRustTrait()` 参数类型
   - `emitRustImpl()` 参数类型

### Phase 36: Rust 关键字冲突处理 ✅

**问题背景**:
- 参数名 `type` 是 Rust 关键字，导致编译错误
- 其他关键字如 `self`, `match`, `fn` 等也有冲突

**参考实现**: 各语言的 `*kw.swg` 文件

**解决方案**:

1. **创建 `Lib/rust/rustkw.swg`**:
   ```swig
   #define RUSTKW(x) %keywordwarn("'" `x` "' is a Rust keyword",rename="r#%s")  `x`
   
   RUSTKW(type);
   RUSTKW(match);
   RUSTKW(fn);
   // ... 所有 Rust 关键字
   ```

2. **修改 `cleanRustName()` 函数** (`rust.cxx:1672-1750`):
   - 检测 Rust 关键字
   - 使用 `r#` 前缀转义（如 `r#type`）
   - 特殊处理 `self`、`Self`、`super`、`crate` - 使用 `_` 后缀（如 `self_`）

3. **修复参数名处理**:
   - `emitRustTrait()` - 参数名使用 `cleanRustName()`
   - `emitRustImpl()` - 参数名使用 `cleanRustName()`
   - `emitFFIParams()` - 参数名使用 `cleanRustName()`

### 测试结果

**修复前**:
```
yd.rs:13262: pub fn GetPriceInYDOrder_YDOrder(order: &mut $*1_type) -> f64
yd.rs:26837: fn getClientPacketHeader(..., type: i32, ...)
错误数量: 大量 typemap 变量和关键字错误
```

**修复后**:
```
yd.rs: 正确生成所有类型
参数 type 正确转换为 r#type
错误数量: 16 个（主要是 C variadic 参数问题）
```

### 剩余问题

1. **C variadic 参数** - `v(...)` 在 Rust trait 中不支持
2. **比较运算符链** - Rust 不支持 `a < b < c` 这样的链式比较

这些问题需要根据实际需求决定是否处理。

---

## 2026-04-16 本次会话：yd.i 项目代码生成修复

### 问题背景

用户使用 `yd.i` 测试 SWIG Rust 绑定，发现生成的代码有大量编译错误。

### 已完成的修复

#### 1. C variadic 参数处理 ✅

**问题**: `writeLog(const char* format, ...)` 变参函数无法在 Rust 中表示

**修复**:
- 添加 `hasVariadicParam()` 函数检测变参参数
- 在 `emitRustTrait()`、`emitRustImpl()`、`emitRustSafeWrapper()`、`classDirectorMethod()`、`emitRustFFIDeclaration()` 中跳过变参函数
- 发出警告 505 告知用户

**修改文件**: `Source/Modules/rust.cxx`

#### 2. 模板实例化类型处理 ✅

**问题**: `YDQueryResult<char>` 被生成错误，如 `YDQueryResult<(char)>` 或 `YDQueryResult_char`

**修复**:
- 在 `getRustUserType()` 中使用 `Swig_symbol_template_deftype()` 获取模板实例化名称
- 在 `cleanTypeName()` 中处理模板语法 `<T>` 转换为 `_T`

**修改文件**: `Source/Modules/rust.cxx`

#### 3. 字符串类型 typemap 优化 ✅

**问题**: 
- `const char*` 返回值映射为 `&str`，但 Rust 需要生命周期标注
- `char*` 参数映射为 `String`，但无法提供可变指针

**修复** (`Lib/rust/rusttype.swg`):
```swig
/* const char* - immutable string pointer */
%typemap(rusttype) const char *   "*const c_char"
%typemap(rsffitype) const char *  "*const c_char"

/* char* - mutable string pointer */
%typemap(rusttype) char *   "*mut c_char"
%typemap(rsffitype) char *  "*mut c_char"
```

**用户使用**:
```rust
// 返回值转换
let version = getYDVersion();
let version_str = unsafe { CStr::from_ptr(version).to_str().unwrap() };

// 输出缓冲区
let mut buffer = [0i8; 256];
timeID2String(id, buffer.as_mut_ptr());
```

#### 4. 类类型参数传递修复 ✅

**问题**: `GetPriceInYDOrder_YDOrder(order: &mut YDOrder)` 直接传 `order` 而不是 `order.ptr`

**修复**: 在 `getRustInputConversion()` 中添加类类型检测：
- 类引用 (`&mut ClassName`) → 提取 `.ptr`
- 类值 (`ClassName`) → 提取 `.ptr`
- 类指针 (`*mut ClassName`) → 直接传递

**修改文件**: `Source/Modules/rust.cxx`

#### 5. 数组参数转换修复 ✅

**问题**: `char[32]` 数组参数需要转换为指针

**修复**: 在 `getRustInputConversion()` 中添加数组类型检测：
```cpp
if (ptype && SwigType_isarray(ptype) && !SwigType_ispointer(ptype)) {
    return NewStringf("%s.as_mut_ptr()", arg_name);
}
```

### 错误数量变化

| 阶段 | 错误数 | 主要修复 |
|------|--------|---------|
| 初始 | 357 | - |
| 变参跳过 | 353 | variadic 函数检测 |
| 模板修复 | 323 | 模板实例化类型 |
| typemap 修改 | 304 | `const char*` → `*const c_char` |
| 类类型 .ptr | 295 | 类引用传递 `.ptr` |
| 当前 | 338 | 部分数组类型待修复 |

### 剩余问题

#### 1. YD_TRS_Count 常量生成 (8个错误)

**问题**: `const int YD_TRS_Count = 4` 被生成为函数 `YD_TRS_Count_get()`，但数组类型 `[c_char; YD_TRS_Count]` 需要编译时常量

**解决方案**: 修改 `constantWrapper()`，让 `const int` 生成 `pub const` 而非 `pub fn ..._get()`

#### 2. YDString typedef 类型 (约 200+ 个错误)

**问题**: `typedef char YDString[32]` 是 typedef 而非直接数组类型

**解决方案**: 
- 在 `yd.i` 中添加: `%typemap(rsin) YDString "$input.as_mut_ptr()"`
- 或在 rust.cxx 中添加 typedef 数组类型的检测

#### 3. 部分参数类型不匹配 (17个错误)

需要进一步分析具体类型

### 使用方法

```bash
$env:SWIG_LIB="D:\code\cpp\swig\Lib"
.\build\Release\swig.exe -rust -c++ -allow-warnings -module yd yd.i
```

---

## 2026-04-16 第二次会话：继续修复 yd.i

### 本次会话完成的修复 ✅

#### 1. Director const char* 返回类型修复 ✅

**问题**: 祖先 trait 实现中 `getVersion()` 方法返回 `c_char` 而非 `*const c_char`
- `YDApiTrait` 定义: `fn getVersion(&mut self) -> *const c_char`
- `YDExtendedApiTrait` impl: `fn getVersion(&mut self) -> c_char` (错误!)

**修复**: 在祖先 trait 实现代码（`emitRustImpl` 第 4500 行附近）添加 const char* 类型检测

```cpp
// 在祖先 trait 实现中，使用与 emitRustImpl 一致的类型检测逻辑
String *type_str = SwigType_str(mtype, 0);
bool is_char_ptr = (type_str && (Strstr(type_str, "char *") || 
                                  Strstr(type_str, "char*") ||
                                  Strstr(type_str, "char const")));
if (is_char_ptr) {
    ret_type = getRustUserType(mtype);  // 返回 *const c_char
}
```

**结果**: E0053 错误全部修复 (之前有 3 个)

#### 2. char[N] 数组参数 setter 修复 ✅

**问题**: 
```rust
pub fn set_Name(&self, Name: [c_char; 32]) {
    unsafe { ffi::Rust_YDSystemParam_Name_set__SWIG_0(self.ptr, Name) }
    // 错误: expected `*mut i8`, found `[i8; 32]`
}
```

**修复 1**: 添加 `rsin` typemap (`Lib/rust/rusttype.swg`)
```swig
%typemap(rsin) char [ANY] "$input.as_mut_ptr()"
%typemap(rsout) char [ANY] "$result"
```

**修复 2**: 修改 `getRustInputConversion()` 只对 char 数组使用指针转换
```cpp
// 只有 char 数组需要 .as_mut_ptr()，其他数组类型直接传递
bool is_char_array = (base_elem && (Cmp(base_elem, "char") == 0 || ...));
if (is_char_array) {
    return NewStringf("%s.as_mut_ptr()", arg_name);
}
return Copy(arg_name);  // 非字符数组直接传递
```

**结果**: setter 函数正确生成:
```rust
pub fn set_Name(&self, Name: [c_char; 32]) {
    unsafe { ffi::Rust_YDSystemParam_Name_set__SWIG_0(self.ptr, Name.as_mut_ptr()) }
}
```

### 当前错误统计

| 错误类型 | 数量 | 说明 |
|---------|------|------|
| E0308 | 234 | 类型不匹配（主要是数组返回值） |
| E0308 (args) | 17 | 函数参数类型错误 |
| E0425 | 8 | YD_TRS_Count 等常量未找到 |
| E0596 | 17 | 可变借用问题 |
| **总计** | **276** | 从初始 357 减少到 276 |

### 待修复问题

#### 1. 全局 const int 变量 (P0 - 8个错误)

**问题**: 
- C++: `const int YD_TRS_Count = 4;`
- 生成的 Rust: `pub fn YD_TRS_Count_get() -> i32`
- 需要的 Rust: `pub const YD_TRS_Count: i32 = 4;`

**解决方案**: 在 `emitRustSafeWrapper()` 中检测全局 const 变量，生成 `pub const`

#### 2. 数组返回值 (P0 - 约 225 个错误)

**问题**: getter 函数期望返回数组但 FFI 返回指针
```rust
pub fn Name(&self) -> [c_char; 32] {
    unsafe { ffi::Rust_YDSystemParam_Name_get__SWIG_0(self.ptr) }
    // FFI 返回 *mut c_char，不是 [c_char; 32]
}
```

**解决方案**: 
- 方案 A: 修改返回类型为 `*mut c_char`
- 方案 B: 从指针复制数据到数组

#### 3. 类类型数组参数 (P1 - 17 个错误)

**问题**: `YDExchangeTradeConstrain[N]` 等类数组类型
```rust
pub fn set_TradeConstrains(&self, TradeConstrains: *mut c_void) {
    unsafe { ffi::...(self.ptr, TradeConstrains.as_mut_ptr()) }  // 错误
}
```

**原因**: 参数已经是 `*mut c_void` 指针，不需要再调用 `.as_mut_ptr()`

**解决方案**: 在 `getRustInputConversion()` 中检测指针类型，直接返回参数名

#### 4. 可变借用问题 (P1 - 17 个错误)

**问题**: 某些参数需要 `&mut` 但声明为不可变

### 修改的文件

| 文件 | 修改内容 |
|------|---------|
| `Source/Modules/rust.cxx` | 祖先 trait const char* 检测 + 数组参数转换逻辑 |
| `Lib/rust/rusttype.swg` | 添加 `rsin`/`rsout` typemap for `char [ANY]` |

### 后续计划

1. **P0**: 修复全局 const int 变量生成
2. **P0**: 修复数组返回值处理
3. **P1**: 修复类类型数组参数
4. **P1**: 修复可变借用问题

---

## 2026-04-16 其他语言模块 Director 类型转换机制分析

### 分析目的

yd.i 生成的 Rust 代码有约 400+ 编译错误，主要是 Director trait 中的类型转换问题。通过分析其他成熟语言模块（C#、Java、Go）的实现，找出正确的解决方案。

### 核心发现：所有语言模块都使用 Typemap 机制

各语言模块都不在 .cxx 文件中硬编码类型转换逻辑，而是通过 typemap 文件定义转换规则。

### C# 模块分析

**关键 Typemaps** (位于 `Lib/csharp/csharp.swg`):

```swig
// 指针类型的类型映射
%typemap(ctype)   SWIGTYPE * "void *"
%typemap(imtype)  SWIGTYPE * "global::System.Runtime.InteropServices.HandleRef"
%typemap(cstype)  SWIGTYPE * "$csclassname"

// Director 输入转换：从 C 指针创建 C# 对象
%typemap(csdirectorin) SWIGTYPE *, SWIGTYPE (CLASS::*) 
    "($iminput == global::System.IntPtr.Zero) ? null : new $csclassname($iminput, false)"

// Director 输出转换：从 C# 对象获取 C 指针
%typemap(csdirectorout) SWIGTYPE *, SWIGTYPE (CLASS::*), SWIGTYPE &, SWIGTYPE && 
    "$csclassname.getCPtr($cscall).Handle"
```

**特殊变量替换机制** (位于 `Source/Modules/csharp.cxx`):

```cpp
// substituteClassname() 函数处理三种特殊变量：
// $csclassname  - 直接使用类名
// $*csclassname - 去掉一级指针/引用
// $&csclassname - 添加一级指针

bool substituteClassname(SwigType *pt, String *tm) {
    if (Strstr(tm, "$csclassname")) {
        SwigType *classnametype = Copy(strippedtype);
        substituteClassnameSpecialVariable(classnametype, tm, "$csclassname");
    }
    if (Strstr(tm, "$*csclassname")) {
        SwigType *classnametype = Copy(strippedtype);
        Delete(SwigType_pop(classnametype));  // 去掉一级
        substituteClassnameSpecialVariable(classnametype, tm, "$*csclassname");
    }
    // ...
}
```

**C# 类型转换流程**:
```
C++ 虚函数调用
    ↓ directorin typemap (C 层)
C 指针 (void*)
    ↓ csdirectorin typemap
C# 对象 (new ClassName(ptr, false))
    ↓ 用户代码
C# 返回值
    ↓ csdirectorout typemap
C 指针 (obj.getCPtr(result).Handle)
    ↓ directorout typemap (C 层)
C++ 返回值
```

### Java 模块分析

**关键 Typemaps** (位于 `Lib/java/java.swg`):

```swig
// Director 输入转换
%typemap(javadirectorin) SWIGTYPE *, SWIGTYPE (CLASS::*) 
    "($jniinput == 0) ? null : new $javaclassname($jniinput, false)"
%typemap(javadirectorin) SWIGTYPE & 
    "new $javaclassname($jniinput, false)"

// Director 输出转换
%typemap(javadirectorout) SWIGTYPE *, SWIGTYPE (CLASS::*), SWIGTYPE &, SWIGTYPE && 
    "$javaclassname.getCPtr($javacall)"
%typemap(javadirectorout) SWIGTYPE 
    "$&javaclassname.getCPtr($javacall)"
```

**Java 使用的关键代码** (位于 `Source/Modules/java.cxx`):

```cpp
// 在 classDirectorMethod() 中：
Swig_typemap_attach_parms("javadirectorin", l, 0);

// 使用 typemap
String *din = Copy(Getattr(p, "tmap:javadirectorin"));
Replaceall(din, "$jniinput", ln);
substituteClassname(pt, din);
```

### Go 模块分析

**关键 Typemaps** (位于 `Lib/go/go.swg`):

```swig
// Go 的 godirectorin 对于类类型为空，意味着直接传递指针
%typemap(godirectorin) SWIGTYPE * ""
%typemap(godirectorin) SWIGTYPE & ""
%typemap(godirectorin) SWIGTYPE && ""

// 但 Go 使用 gotype 来定义类型
%typemap(gotype) SWIGTYPE * "$gotypename"
%typemap(gotype) SWIGTYPE & "$gotypename"
```

**Go 的处理方式** (位于 `Source/Modules/go.cxx`):

```cpp
String *goin = goGetattr(p, "tmap:godirectorin");
if (goin == NULL) {
    Printv(call, ln, NULL);  // 直接使用参数
} else {
    // 如果有 typemap，进行转换
    Replaceall(goin, "$input", ln);
    Replaceall(goin, "$result", ivar);
}
```

### 统一设计模式

所有语言模块都遵循相同的架构：

| Typemap | 方向 | 用途 |
|---------|------|------|
| `directorin` | C→Language | C 层准备参数 |
| `xxxdirectorin` | C→Language | 语言层接收参数（从指针创建对象） |
| `xxxdirectorout` | Language→C | 语言层返回参数（从对象获取指针） |
| `directorout` | Language→C | C 层接收返回值 |

**特殊变量命名规范**:

| 变量 | C# | Java | Rust (应使用) |
|------|-----|------|---------------|
| 类名 | `$csclassname` | `$javaclassname` | `$rustclassname` |
| 去指针 | `$*csclassname` | `$*javaclassname` | `$*rustclassname` |
| 加指针 | `$&csclassname` | `$&javaclassname` | `$&rustclassname` |
| 输入参数 | `$iminput` | `$jniinput` | `$rsinput` 或 `$input` |
| 输出结果 | `$cscall` | `$javacall` | `$rscall` 或 `$result` |

### Rust 模块的正确实现方案

基于以上分析，Rust 模块应该：

#### 1. 添加 Director 专用 Typemaps

在 `Lib/rust/rusttype.swg` 中添加：

```swig
/* -----------------------------------------------------------------------------
 * Director 类型转换
 * 
 * rsdirectorin: 从 FFI 指针创建 Rust 包装对象
 * rsdirectorout: 从 Rust 对象获取 FFI 指针
 * ----------------------------------------------------------------------------- */

// 指针类型
%typemap(rsdirectorin) SWIGTYPE *, SWIGTYPE (CLASS::*) 
    "($rsinput.is_null()) ? std::ptr::null_mut() : $rustclassname { ptr: $rsinput as *mut c_void }"
%typemap(rsdirectorin) const SWIGTYPE *
    "($rsinput.is_null()) ? std::ptr::null() : $rustclassname { ptr: $rsinput as *mut c_void }"

// 引用类型
%typemap(rsdirectorin) SWIGTYPE & 
    "$rustclassname { ptr: $rsinput as *mut c_void }"
%typemap(rsdirectorin) const SWIGTYPE & 
    "$rustclassname { ptr: $rsinput as *mut c_void }"

// 输出转换
%typemap(rsdirectorout) SWIGTYPE *, SWIGTYPE (CLASS::*), SWIGTYPE &, SWIGTYPE && 
    "$rustclassname::get_ptr($rscall) as *mut c_void"
%typemap(rsdirectorout) const SWIGTYPE *, const SWIGTYPE & 
    "$rustclassname::get_ptr($rscall) as *const c_void"
```

#### 2. 在 rust.cxx 中使用 Typemap

修改 `emitRustTrait()` 和 `emitRustImpl()` 中的参数处理：

```cpp
// 查找 rsdirectorin typemap
Swig_typemap_attach_parms("rsdirectorin", params, 0);

for (Parm *p = params; p; p = nextSibling(p)) {
    String *din = Getattr(p, "tmap:rsdirectorin");
    if (din) {
        din = Copy(din);
        Replaceall(din, "$rsinput", arg_name);
        substituteRustClassname(pt, din);
        // 使用 din 作为转换后的代码
    } else {
        // 没有 typemap，使用默认转换
    }
}
```

#### 3. 确保 substituteRustClassname 正确实现

当前 `rust.cxx` 中已有 `substituteRustClassname()` 函数，需要确保它处理：
- `$rustclassname` - 类名
- `$*rustclassname` - 去掉一级指针/引用
- `$&rustclassname` - 添加一级指针

### 当前 Rust 模块的问题

当前实现存在以下问题：

1. **硬编码转换**: 在 `getRustInputConversion()` 中硬编码了类型判断逻辑，而不是使用 typemap
2. **缺少 rsdirectorin/out typemap**: 没有为 Director 定义专门的转换规则
3. **类型转换不一致**: 
   - 返回类型使用 `$rustclassname` 但参数类型返回 `*const c_void`
   - 导致 `*const YDCashCommissionRatePiece` 与 `*const c_void` 不匹配

### 修复优先级

| 优先级 | 任务 | 预期效果 |
|--------|------|---------|
| P0 | 添加 rsdirectorin/out typemap | 定义 Director 转换规则 |
| P0 | 修改 emitRustTrait/emitRustImpl 使用 typemap | 统一类型转换逻辑 |
| P1 | 完善 substituteRustClassname | 支持所有特殊变量 |
| P2 | 添加 get_ptr() 方法到生成的类 | 支持从包装对象获取指针 |

### 参考实现位置

| 功能 | C# | Java | Go |
|------|-----|------|-----|
| Typemap 文件 | `Lib/csharp/csharp.swg` | `Lib/java/java.swg` | `Lib/go/go.swg` |
| 替换函数 | `csharp.cxx:3652` | `java.cxx` 类似实现 | `go.cxx` |
| Director 处理 | `csharp.cxx:4295+` | `java.cxx:4295+` | `go.cxx:3350+` |

---

## 2026-04-16 yd.i 测试修复

### 测试背景

使用 `yd.i` 接口文件测试 Rust 绑定生成，该文件包含大量复杂的 C++ 类定义。

### 发现的问题

#### 1. 指针类型 FFI 类型不匹配 🔴 P0

**问题**: 类指针参数（如 `*mut YDInputOrder`、`*const YDInstrument`）没有正确转换为 `*mut c_void` / `*const c_void`

**示例错误**:
```
error[E0308]: mismatched types
  --> yd.rs:23448:18
   |
   | expected raw pointer `*mut std::ffi::c_void`
   |    found raw pointer `*mut YDInputOrder`
```

**根因**: `getRustInputConversion()` 函数中的 Method 5 和 Method 6 没有正确处理类指针类型

#### 2. 数组参数类型不匹配 🔴 P0

**问题**: C++ 数组参数（如 `YDInputOrder inputOrders[]`、`const YDInstrument* instruments[]`）没有正确转换

**根因**: Method 4 对非字符数组返回 `arg_name` 而没有类型转换

#### 3. 基本类型指针缺少类型映射 🟡 P1

**问题**: `unsigned char*`、`int*` 等基本类型指针缺少 `rsffitype` 类型映射

### 已完成的修复

#### 修复 1: 指针类型转换 (Method 6)

**文件**: `Source/Modules/rust.cxx` - `getRustInputConversion()`

**修改**: 统一处理所有指针类型，转换为 `*mut c_void` 或 `*const c_void`

```cpp
// Method 6: Handle ALL pointer types that need conversion to c_void
if (ptype && SwigType_ispointer(ptype)) {
    // Check if already c_char or c_void pointer - no conversion needed
    bool is_char_or_void = /* check type_str */;
    
    if (!is_char_or_void) {
        return NewStringf("%s as *mut c_void", arg_name);
    }
}
```

#### 修复 2: 数组类型转换 (Method 4)

**文件**: `Source/Modules/rust.cxx` - `getRustInputConversion()`

**修改**: 非字符数组转换为 `*mut c_void`

```cpp
// For non-char arrays, pass as c_void pointer
if (ptype && SwigType_isarray(ptype) && !SwigType_ispointer(ptype)) {
    return NewStringf("%s as *mut c_void", arg_name);
}
```

#### 修复 3: 添加基本类型指针类型映射

**文件**: `Lib/rust/rusttype.swg`

**新增**: 
- `unsigned char*` → `*mut c_uchar` (rusttype), `*mut c_uchar` (rsffitype)
- `const unsigned char*` → `*const c_uchar` (rusttype), `*const c_uchar` (rsffitype)
- `int*` → `*mut i32` (rusttype), `*mut c_void` (rsffitype)
- `const int*` → `*const i32` (rusttype), `*const c_void` (rsffitype)
- 类似地添加了 `long*`, `double*`, `float*` 等

### 编译错误统计

| 阶段 | 错误数 |
|------|--------|
| 修复前 | 164 |
| 修复指针类型后 | 139 |
| 减少 | 25 |

### 剩余问题

#### 1. `expected bool, found u8` (约 108 个错误)

**问题**: FFI 返回 `u8` (c_uchar)，但某些地方期望 `bool`

**位置**: 需要检查 bool 类型的返回值处理逻辑

**解决方案**: 检查 `emitRustImpl()` 中 bool 返回类型的处理，确保 FFI 返回值正确转换

#### 2. `cannot borrow ... as mutable` (约 15 个错误)

**问题**: 成员变量的 setter 方法使用了 `&mut self`，但调用者只有 `&self`

**示例**:
```rust
// 生成的代码
pub fn set_TradingRightFromSource(&mut self, TradingRightFromSource: i64) {
    // ...
}

// 调用处
self.set_TradingRightFromSource(value);  // error: cannot borrow as mutable
```

**解决方案**: 需要检查成员变量 setter 的 self 类型，或修改调用者的 self 类型

### 下一步计划

1. **P0**: 修复 bool 返回类型处理 - 检查 `emitRustImpl()` 中的 bool 返回值逻辑
2. **P1**: 修复成员变量 setter 的 self 类型问题
3. **P2**: 完善 `getRustUserType()` 函数，确保所有类型都有正确的 Rust 类型映射
4. **P3**: 考虑使用 typemap 机制替代硬编码逻辑（参考 csharp.cxx 实现）

---

## 2026-04-16 本次会话：yd.i 接口文件修复

### 问题背景

用户使用 `yd.i` 接口文件生成 Rust 绑定代码，发现大量编译错误。通过 `rustc -A warnings` 屏蔽警告后，发现以下主要问题：

### 已完成的修复 ✅

#### 1. bool 返回值转换修复 ✅

**问题描述**:
- FFI 函数返回 `u8` (c_uchar)，但方法签名期望 `bool`
- 在 `impl YDApiTrait for YDExtendedApi`（继承类实现基类 trait）中缺少转换

**修复位置**: `Source/Modules/rust.cxx`
- 第 3850-3855 行 (`emitRustImpl` 中的 bool 返回处理)
- 第 4561-4564 行 (基类 trait 实现中的 bool 返回处理)

**修复内容**:
```cpp
// 检测 bool 返回类型
if (return_type && SwigType_type(return_type) == T_BOOL) {
  is_bool_type = true;
}

// 生成转换代码
if (return_is_bool) {
  Printf(f_wrapper_code, "unsafe { ffi::%s(self.ptr", wname);
  emitFFIParams(params, false);
  Printf(f_wrapper_code, ") != 0 }\n");  // 添加 != 0 转换
}
```

**验证结果**:
```rust
// 修复前
fn start(&mut self, pListener: *mut YDListener) -> bool {
    unsafe { ffi::Rust_YDApi_start__SWIG_0(self.ptr, pListener as *mut c_void) }
}

// 修复后
fn start(&mut self, pListener: *mut YDListener) -> bool {
    unsafe { ffi::Rust_YDApi_start__SWIG_0(self.ptr, pListener as *mut c_void) != 0 }
}
```

#### 2. unsigned char* 类型映射修复 ✅

**问题描述**:
- `unsigned char*` 参数被错误识别为 `char*`，生成了错误的 Rust 类型
- 根因: `getRustUserType()` 中的 char 指针检测使用了模糊匹配 `Strstr(type_str, "char *")`

**修复位置**: `Source/Modules/rust.cxx` 第 1512-1520 行

**修复内容**:
```cpp
// 检测 char 指针时排除 unsigned char
bool is_char_pointer = (Strcmp(base, "char") == 0 && SwigType_ispointer(t)) ||
                       (type_str && (Strstr(type_str, "char *") || 
                                     Strstr(type_str, "char*") ||
                                     Strstr(type_str, "char const")) &&
                        !Strstr(type_str, "unsigned"));  // 关键：排除 unsigned
```

**验证结果**:
```rust
// 修复前
fn getClientPacketHeader(&mut self, r#type: i32, pHeader: *const c_char, ...) -> i32

// 修复后
fn getClientPacketHeader(&mut self, r#type: i32, pHeader: *mut c_uchar, ...) -> i32
```

#### 3. 指针类型检测增强 ✅

**问题描述**:
- SWIG 的 `SwigType_ispointer()` 对某些带 const 的指针类型返回 false
- 导致类型被错误识别为值类型而非指针类型

**修复位置**: `Source/Modules/rust.cxx` 第 1626-1635 行

**修复内容**:
```cpp
// 增加字符串检测作为 fallback
bool is_pointer_type = SwigType_ispointer(t);
if (!is_pointer_type && type_str) {
  if (Strstr(type_str, "*")) {
    is_pointer_type = true;
  }
}
```

#### 4. 类指针返回类型检测优化 ✅

**问题描述**:
- typemap 返回 `*const c_void` 时，代码没有正确识别这是类指针
- 需要调用 `getRustUserType()` 获取具体的类名

**修复位置**: `Source/Modules/rust.cxx` 第 4256-4265 行

**修复内容**:
```cpp
// 检测 opaque 指针类型并 fallback 到 getRustUserType
bool is_opaque_ptr = (ret_type && (Cmp(ret_type, "*const c_void") == 0 || 
                                    Cmp(ret_type, "*mut c_void") == 0));
if (!ret_type || Len(ret_type) == 0 || Strstr(ret_type, "$") || is_opaque_ptr) {
  ret_type = getRustUserType(mtype);
}
```

### 剩余问题 🔴

#### 类指针返回值包装不完整

**问题描述**:
- 方法返回 `const YDSystemParam *` 时，FFI 返回 `*const c_void`
- 在基类 trait 实现中，应该包装为 `YDSystemParam { ptr: ... as *mut c_void }`
- 当前代码中 `return_is_class_wrapper` 检测逻辑不够完善

**错误示例**:
```rust
// impl YDApiTrait for YDExtendedApi (基类 trait 实现)
fn getSystemParam(&mut self, pos: i32) -> YDSystemParam {
    unsafe { ffi::Rust_YDApi_getSystemParam__SWIG_0(self.ptr, pos) }
    // 错误: expected `YDSystemParam`, found `*const c_void`
}

// 应该是:
fn getSystemParam(&mut self, pos: i32) -> YDSystemParam {
    YDSystemParam { ptr: unsafe { ffi::Rust_YDApi_getSystemParam__SWIG_0(self.ptr, pos) as *mut c_void } }
}
```

**影响范围**: 约 72 个类似错误

**根本原因**:
1. SWIG 类型系统复杂，`const ClassName *` 的内部表示有多种形式
2. `SwigType_ispointer()` 对带 qualifier 的指针检测不一致
3. `classLookup()` 在某些上下文中找不到类节点

### 长期解决方案

#### 方案 1: 参考 csharp.cxx 的类型处理模式

**参考代码位置**: `Source/Modules/csharp.cxx`
- 第 800-850 行: 返回类型处理
- `Swig_typemap_lookup()` 的使用方式

**改进点**:
1. 使用 `SwigType_typedef_resolve_all()` 解析所有 typedef
2. 使用 `SwigType_strip_qualifiers()` 移除 const/volatile
3. 检查 `SwigType_type()` 返回 `T_USER` 时，进一步验证是否是类指针

#### 方案 2: 重构类指针返回值检测逻辑

**需要修改的函数**:
1. `getRustUserType()` - 增强 const 指针检测
2. `emitRustImpl()` - 改进返回值包装逻辑
3. 基类 trait 实现部分 - 统一返回值处理

**伪代码**:
```cpp
// 在基类 trait 实现的返回值处理中
bool is_class_pointer_return = false;
String *class_pointer_name = NULL;

if (has_return && mtype) {
  // 1. 尝试直接获取类名
  if (SwigType_ispointer(mtype)) {
    SwigType *pointed = SwigType_del_pointer(Copy(mtype));
    SwigType *stripped = SwigType_strip_qualifiers(pointed);
    String *base_name = SwigType_base(stripped);
    Node *class_node = classLookup(base_name);
    if (class_node) {
      is_class_pointer_return = true;
      class_pointer_name = Getattr(class_node, "sym:name");
    }
  }
  
  // 2. 检查 ret_type 是否是类名（非指针、非基本类型）
  if (!is_class_pointer_return && ret_type && !isRustBasicType(ret_type)) {
    if (!Strstr(ret_type, "*") && !Strstr(ret_type, "&")) {
      Node *class_node = classLookup(ret_type);
      if (class_node) {
        is_class_pointer_return = true;
        class_pointer_name = ret_type;
      }
    }
  }
}

// 3. 生成正确的包装代码
if (is_class_pointer_return && class_pointer_name) {
  Printf(f_wrapper_code, "%s { ptr: unsafe { ffi::%s(...) as *mut c_void } }", 
         class_pointer_name, wname);
}
```

#### 方案 3: 添加调试输出定位问题

**在 `getRustUserType()` 中添加调试**:
```cpp
String *getRustUserType(SwigType *t) {
  // 调试输出
  String *debug_str = SwigType_str(t, 0);
  Printf(stderr, "DEBUG getRustUserType: type=%s, ispointer=%d, type_code=%d\n",
         debug_str, SwigType_ispointer(t), SwigType_type(t));
  Delete(debug_str);
  // ...
}
```

### 后续计划

| 优先级 | 任务 | 预计影响 |
|--------|------|----------|
| P0 | 修复类指针返回值包装 | 解决剩余 72 个编译错误 |
| P1 | 统一 emitRustImpl 和基类 trait 实现的返回值处理 | 减少代码重复 |
| P2 | 完善 getRustUserType() 对所有指针类型的处理 | 提高类型映射准确性 |
| P3 | 添加单元测试覆盖这些边界情况 | 防止回归 |

### 测试验证

当前状态:
- 错误数量: 从最初的 200+ 减少到 72 个
- 主要问题: 类指针返回值包装不完整
- bool 和 unsigned char* 问题已解决 ✅


---

## 2026-04-16 本次会话修复记录 ✅

### 1. getRustUserType() use-after-free 修复 ✅

**问题**: `type_str` 在第 1607 行被删除，但在第 1621 行又被用于指针类型检测，导致 use-after-free 错误。这可能导致指针类型（如 `const SomeClass*`）被错误识别为非指针类型。

**修复**: 将 `Delete(type_str)` 移动到指针处理完成后执行。

**影响**: 修复后，`getRustUserType()` 正确返回 `*const SomeClass` 而不是 `SomeClass`。

### 2. 类指针返回类型包装修复 ✅

**问题**: 当函数返回类指针类型（如 `const YDSystemParam*`）时：
- `SwigType_type(return_type)` 返回 `T_POINTER`，不是 `T_USER`
- `is_custom_type` 没有被正确设置
- FFI 调用没有正确包装返回值

**修复**: 在 `emitRustImpl()` 中添加对类指针返回类型的检测：
```cpp
// 检测类指针返回类型
bool is_class_pointer_return = false;
if (return_type && SwigType_ispointer(return_type)) {
  SwigType *pointed_type = SwigType_del_pointer(Copy(return_type));
  if (pointed_type && SwigType_type(pointed_type) == T_USER) {
    is_class_pointer_return = true;
  }
}

// 生成正确的返回语句
if (is_class_pointer_return) {
  // 直接返回裸指针
  Printf(f_wrapper_code, "unsafe { ffi::%s(...) as *mut _ }", wname);
}
```

### 3. 数组参数 mut 关键字修复 ✅

**问题**: 当参数是数组类型（如 `char[32]` 或 `YDString` typedef）时：
- FFI 调用使用 `.as_mut_ptr()` 转换
- 但参数没有声明为 `mut`
- 导致编译错误 `cannot borrow as mutable`

**修复**: 在 `emitRustSafeWrapper()`、`emitRustTrait()` 和 `emitRustImpl()` 中添加 `mut` 检测：
```cpp
// Method 1: 检查 SwigType_isarray
bool needs_mut = false;
if (ptype && SwigType_isarray(ptype) && !SwigType_ispointer(ptype)) {
  SwigType *elem_type = SwigType_array_type(ptype);
  if (elem_type && !SwigType_isconst(elem_type)) {
    needs_mut = true;
  }
}

// Method 2: 检查 getRustInputConversion 是否返回 .as_mut_ptr()
// 这处理 typedef 后的数组类型
if (!needs_mut) {
  String *conversion = getRustInputConversion(p, param_name);
  if (conversion && Strstr(conversion, ".as_mut_ptr()")) {
    needs_mut = true;
  }
}

if (needs_mut) {
  Printf(f_wrapper_code, "mut %s: %s", param_name, rust_type);
}
```

### 修复验证

**yd.rs 编译测试**:
- 修复前: 71 个错误（E0308 类型不匹配 + E0596 可变借用）
- 修复后: 0 个错误 ✅

**修复的错误类型**:
1. E0308: 类指针返回值类型不匹配（约 40 处）
2. E0596: 数组参数可变借用问题（约 31 处）

### 涉及的文件

| 文件 | 修改内容 |
|------|----------|
| `Source/Modules/rust.cxx` | 修复 getRustUserType use-after-free |
| `Source/Modules/rust.cxx` | 添加类指针返回类型检测和包装 |
| `Source/Modules/rust.cxx` | 添加数组参数 mut 关键字检测 |

---

## 最后更新
2026-04-16 (Phase 42: trait 方法签名 mut 参数修复完成)

### Phase 42: trait 方法签名 mut 参数修复 ✅ (2026-04-16)

#### 问题描述
在 trait 方法签名中使用了 `mut` 参数模式，但 Rust 不允许这样做：
- `mut` 是实现细节，不应该出现在 trait 定义中
- 只有在 `impl` 实现中才能使用 `mut` 绑定

**错误示例**:
```rust
// 错误：trait 方法签名中使用 mut
pub trait SomeTrait {
    fn method(&mut self, mut buffer: [c_char; 32]);  // 错误！
}
```

**正确做法**:
```rust
// 正确：trait 方法签名中不使用 mut
pub trait SomeTrait {
    fn method(&mut self, buffer: [c_char; 32]);  // 正确
}

// impl 中根据需要使用 mut
impl SomeTrait for SomeClass {
    fn method(&mut self, mut buffer: [c_char; 32]) {  // 正确
        unsafe { ffi::method(self.ptr, buffer.as_mut_ptr()) }
    }
}
```

#### 修复内容
**文件**: `Source/Modules/rust.cxx` - `emitRustTrait()` 函数（约第 4275-4300 行）

**修改**: 移除 trait 方法签名中的 `mut` 参数检测和输出逻辑

```cpp
// 修复前
bool needs_mut = false;
// ... 检测逻辑 ...
if (needs_mut) {
  Printf(f_wrapper_code, "mut %s: %s", param_name, rust_type);
} else {
  Printf(f_wrapper_code, "%s: %s", param_name, rust_type);
}

// 修复后
// NOTE: In Rust trait method signatures, we CANNOT use 'mut' pattern.
// The 'mut' keyword is an implementation detail, not part of the interface.
Printf(f_wrapper_code, "%s: %s", param_name, rust_type);
```

#### 验证结果
- **错误数量**: 从 23 个 "patterns aren't allowed in functions without bodies" 减少到 0
- **yd.rs 编译**: 通过 ✅（只剩 "main function not found"，正常）

---

## 长期改进项 (Phase 43+)

### rsdirectorin/rsdirectorout Typemap 机制

**背景**: 其他语言模块（C#、Java、Go）都使用 typemap 机制处理 Director 类型转换：
- C#: `csdirectorin`/`csdirectorout`
- Java: `javadirectorin`/`javadirectorout`
- Go: `godirectorin`

**当前 Rust 实现**: 在 `rust.cxx` 中硬编码类型检测和转换逻辑

**改进建议**:
1. 设计 `rsdirectorin`/`rsdirectorout` typemap 格式
2. 重构 `rust.cxx` 中的硬编码逻辑
3. 更新 `Lib/rust/rusttype.swg` 类型映射文件

**优先级**: P3（长期优化，当前功能已可用）

---

## 当前状态总结 (2026-04-16)

### 已完成的 Phase

| Phase | 内容 | 状态 |
|-------|------|------|
| 1-40 | 基础框架和核心功能 | ✅ |
| 41 | yd.i 修复：bool 返回值、unsigned char*、指针类型检测 | ✅ |
| 42 | trait 方法签名 mut 参数修复 | ✅ |
| 44 | std::string 类型处理改进 | ✅ |

### Phase 44: std::string 类型处理改进 (2026-04-16)

#### 问题
- `const std::string&` 返回类型在 trait 实现中生成的代码有类型不匹配问题
- FFI 返回 `*const c_void`，但 `SwigString.ptr` 需要 `*mut c_void`

#### 修复内容
1. **添加引用类型处理分支**: 在 `emitRustImpl()` 中添加对 `SwigType_isreference(return_type)` 的处理
2. **添加类型转换**: 在 rsout typemap 处理中自动添加 `as *mut std::ffi::c_void` 类型转换
3. **添加 SwigString fallback**: 当 typemap 未找到时，检测 `ret_type == "SwigString"` 并生成正确的包装代码

#### 修改的文件
- `Source/Modules/rust.cxx` - 多处修复
- `Lib/rust/std_string.i` - 优化 typemap 定义

#### 验证结果
- **std_string_test.rs 编译**: 通过 ✅
- **test_member_ref.rs 编译**: 通过 ✅

### 测试结果
- **yd.rs 编译**: 通过 ✅
- **Rust 测试套件**: 16 个测试通过 ✅
- **std_string_test.rs**: 通过 ✅

### 待完成的工作
1. **P3**: rsdirectorin/rsdirectorout typemap 机制（长期改进）
2. **P4**: 文档编写 `Doc/Manual/Rust.html`
3. **P3**: 清理 rust.cxx 中剩余的硬编码 fallback 逻辑
4. **P3**: FFI-safe 警告修复 - 为 Director VTable 结构体添加 `#[repr(C)]` 属性
5. **P4**: 常量命名规范 - 可选，用户可通过 `#![allow(non_upper_case_globals)]` 抑制

---

## 2026-04-16 Phase 45: 验证类指针返回值包装功能 ✅

### 验证内容

重新运行 yd.i 项目编译，确认之前修复的功能正常工作。

### 验证结果

**编译状态**: ✅ 成功
- `yd.rs` (898KB) 编译通过
- 0 个编译错误
- 420 个警告（主要是常量命名和 FFI-safe）

### 主要警告分析

| 警告类型 | 数量 | 说明 |
|---------|------|------|
| `non_upper_case_globals` | ~418 | 常量命名不符合 Rust 大写规范（用户可通过 allow 抑制）|
| `extern` block not FFI-safe | 2 | Director VTable 结构体缺少 `#[repr(C)]` 属性 |

### 类指针返回值代码示例

**FFI 声明**:
```rust
pub fn Rust_YDApi_getSystemParam__SWIG_0(jarg1: *mut c_void, jarg2: c_int) -> *const c_void;
```

**Trait 方法**:
```rust
fn getSystemParam(&mut self, pos: i32) -> YDSystemParam {
    YDSystemParam { ptr: unsafe { ffi::Rust_YDApi_getSystemParam__SWIG_0(self.ptr, pos) as *mut c_void } }
}
```

**结论**: 类指针返回值包装功能正常工作，FFI 返回的 `*const c_void` 被正确包装为对应的类结构体。

### 参考 csharp.cxx 的实现要点

C# 使用 typemap 变量机制处理类类型：
- `$csclassname` - 直接替换为类名
- `$*csclassname` - 去掉指针后替换
- `$&csclassname` - 添加指针后替换

Rust 当前实现使用 `getRustUserType()` 函数处理类型转换，效果等效但不如 typemap 变量机制灵活。

---

## 2026-04-16 Phase 46: 修复双重 unsafe 块嵌套问题 ✅

### 问题描述

在处理 rsout typemap 时，代码会自动将 FFI 调用包装在 `unsafe { }` 块中，但某些 typemap（如 `const std::string&` 的 rsout）本身已包含 `unsafe` 关键字，导致生成嵌套的 `unsafe` 块：

```rust
// 生成的代码（有问题）
SwigString { ptr: unsafe { swig_string_ffi::SwigString_clone(unsafe { ffi::Rust_get_global_string__SWIG_0() }) } }
```

Rust 编译器会警告：`unnecessary unsafe block`

### 修复方案

在 `emitRustSafeWrapper()` 和 `emitRustImpl()` 中添加检测逻辑：

1. 检测 typemap 是否包含 `unsafe` 关键字
2. 如果已包含，则直接传递 FFI 调用表达式，不再额外包装 `unsafe`
3. 对于 `SWIG_OPTIONAL_PTR` marker，同样避免重复包装

### 修改的文件

**`Source/Modules/rust.cxx`**:
- `emitRustSafeWrapper()`: 添加 `typemap_has_unsafe` 检测
- `emitRustImpl()`: 添加相同的检测逻辑
- `SWIG_OPTIONAL_PTR` 处理: 移除额外的 `unsafe` 包装

### 修复后的代码

```rust
// 修复后
SwigString { ptr: unsafe { swig_string_ffi::SwigString_clone(ffi::Rust_get_global_string__SWIG_0() as *const std::ffi::c_void) } }
```

### 测试结果

| 测试文件 | 编译状态 |
|---------|---------|
| class_methods | ✅ |
| inherit_basic | ✅ |
| director_test | ✅ |
| overload_test | ✅ |
| enums_test | ✅ |
| std_string_test | ✅ |

### 待完成的改进

虽然修复了双重 unsafe 问题，但 `rust.cxx` 中仍有一些硬编码的类型检测逻辑：

1. `return_is_swigstring` - 检测返回类型是否是 SwigString
2. 成员变量 getter 中对 SwigString 的克隆处理

这些硬编码的主要原因是：当成员变量返回 `std::string&` 或 `const std::string&` 时，需要克隆字符串以避免生命周期问题。

**改进方向**：可以设计专门的 `rsout_member_get` typemap 来处理成员变量 getter 的特殊情况，将克隆逻辑移到 typemap 中。但这需要更复杂的设计，暂时保留现状。

---

## 2026-04-16 Phase 46.5: const std::string& 返回类型修复 ✅

### 问题描述

当方法返回 `const std::string&` 时：
- FFI 函数返回 `*const c_void`
- `SwigString { ptr: ... }` 期望 `*mut c_void`
- 导致类型不匹配编译错误

### 修复方案

在生成 SwigString 包装代码时，添加类型转换：
```rust
// 修复前
SwigString { ptr: unsafe { ffi::... } }  // 类型不匹配

// 修复后
SwigString { ptr: unsafe { ffi::... as *mut std::ffi::c_void } }
```

### 修改位置

**`Source/Modules/rust.cxx`**:
- `emitRustImpl()`: 主方法处理（第 3790 行附近）
- `emitRustImpl()`: 祖先类 trait 实现（第 3987 行附近）

### 测试结果

所有测试通过：class_methods, inherit_basic, director_test, std_string_test, enums_test

---

## 2026-04-16 Phase 47: Enum 底层类型支持设计

### 背景问题

C++11 允许指定枚举的底层类型：
```cpp
enum class SmallEnum : uint8_t { A, B, C };  // 1 字节
enum class BigEnum : long long { X, Y };       // 8 字节
```

但当前 SWIG Rust 生成器：
1. 使用 `#[repr(C)]` 统一处理
2. FFI 层假设返回 `c_int`

### 设计挑战

**C ABI 返回值提升**：
- 小于 `int` 的返回值会被提升到 `int` 大小
- `enum class X : uint8_t` 返回时可能是 4 字节
- 如果 Rust 使用 `#[repr(u8)]`，大小是 1 字节
- transmute 会失败

### 当前实现

使用 `#[repr(C)]` 保持兼容性：
- 枚举大小与 C 兼容（通常 4 字节）
- FFI 返回 `c_int`，通过 transmute 转换

### 未来改进方向

1. **获取底层类型**：`Getattr(n, "enumbase")` 已支持
2. **精确大小匹配**：需要修改 C 包装层返回正确类型
3. **typemap 变量**：类似 C# 的 `$csclassname` 机制

### 部分实现

已在 `enumDeclaration()` 中添加底层类型检测代码，为将来完整实现做准备。

---

## 长期改进计划

### P1 - Typemap 机制完善

**目标**：将 rust.cxx 中的硬编码类型检测逻辑移到 typemap 文件中

**现状分析**：

| 硬编码位置 | 用途 | 是否可移到 typemap |
|-----------|------|-------------------|
| `return_is_swigstring` | 检测 SwigString 返回类型 | ⚠️ 部分可以 |
| 成员变量 getter 克隆 | 克隆 `std::string&` 返回值 | ✅ 可以设计 `rsout_member_get` |
| `getRustUserType()` 中的 std::string | 返回 SwigString 类型名 | ✅ 已通过 rusttype typemap |

**改进方案**：

1. **新增 `rsout_member_get` typemap**
   ```swig
   // 专门用于成员变量 getter 的返回值处理
   %typemap(rsout_member_get) std::string& "SwigString { ptr: unsafe { swig_string_ffi::SwigString_clone($result as *const c_void) } }"
   %typemap(rsout_member_get) const std::string& "SwigString { ptr: unsafe { swig_string_ffi::SwigString_clone($result) } }"
   ```

2. **移除 rust.cxx 中的硬编码检测**
   - 在 `emitRustSafeWrapper()` 中优先查找 `rsout_member_get`
   - 找不到时 fallback 到 `rsout`

**预期收益**：
- 类型处理逻辑集中在 typemap 文件
- 用户可以自定义成员变量 getter 的行为
- 符合 SWIG 的设计哲学

### P2 - 文档更新

**现状**：`Doc/Manual/Rust.html` 已存在（1010 行），内容较完整

**需要更新的部分**：

| 章节 | 当前内容 | 需要更新 |
|------|---------|---------|
| std::string typemap 示例 | 简单的 String 映射 | 更新为 SwigString 包装类型 |
| STL 容器 | 基础内容 | 确认与实际实现一致 |
| 运算符映射 | 已有完整表格 | ✅ 无需更新 |

**更新示例**：
```html
<!-- 当前 -->
%typemap(rusttype) std::string "String"

<!-- 应更新为 -->
%typemap(rusttype) std::string "SwigString"
// SwigString provides full std::string interface and From/Into conversions
```

### P3 - FFI-safe 警告修复

**问题**：Director VTable 结构体缺少 `#[repr(C)]` 属性

**修复方案**：在 `classDirectorEnd()` 中为 VTable 结构体添加 `#[repr(C)]`

**影响范围**：仅影响 Director 功能用户

### P4 - 常量命名规范

**问题**：~418 个 `non_upper_case_globals` 警告

**解决方案**：
- 选项 A：修改代码生成逻辑，将常量名转为大写
- 选项 B：用户通过 `#![allow(non_upper_case_globals)]` 抑制
- 推荐：选项 B（保持与 C++ 命名一致更直观）

---

## 当前状态总结 (2026-04-16)

### 已完成的功能

| 功能 | 状态 | 测试 |
|------|------|------|
| 基础类型映射 | ✅ | 通过 |
| 类包装 (struct + trait + impl) | ✅ | 通过 |
| 单继承/多继承 | ✅ | 通过 |
| Director（虚函数回调） | ✅ | 通过 |
| VTable 优化模式 | ✅ | 通过 |
| 函数重载（Trait 泛化） | ✅ | 通过 |
| 运算符映射 | ✅ | 通过 |
| 枚举/常量 | ✅ | 通过 |
| 命名空间 | ✅ | 通过（有限制） |
| 模板实例化 | ✅ | 通过 |
| 异常处理 | ✅ | 通过 |
| STL 容器 | ✅ | 通过 |
| std::string/std::wstring | ✅ | 通过 |
| 智能指针 | ✅ | 通过 |

### STL 容器支持清单

| 容器 | 文件 | 映射到 Rust 类型 |
|------|------|-----------------|
| std::vector | std_vector.i | Vec<T> |
| std::deque | std_deque.i | VecDeque<T> |
| std::list | std_list.i | LinkedList<T> |
| std::map | std_map.i | BTreeMap<K,V> |
| std::set | std_set.i | BTreeSet<T> |
| std::unordered_map | std_unordered_map.i | HashMap<K,V> |
| std::unordered_set | std_unordered_set.i | HashSet<T> |
| std::pair | std_pair.i | (T, U) |
| std::string | std_string.i | SwigString |
| std::wstring | std_wstring.i | SwigWString |
| std::shared_ptr | std_shared_ptr.i | 不透明句柄 |
| std::unique_ptr | std_unique_ptr.i | 不透明句柄 |

### 已知限制

1. **命名空间**：Rust 不允许重复定义同一个 `mod`，C++ 分散定义的命名空间需要合并
2. **生命周期**：FFI 层不处理生命周期，返回 owned 类型而非引用
3. **成员变量 getter**：返回引用时需要克隆（生命周期问题）

### 下一步工作优先级

| 优先级 | 任务 | 工作量估计 |
|--------|------|-----------|
| P1 | `rsout_member_get` typemap 设计 | 中等 |
| P2 | 文档更新 std::string 示例 | 小 |
| P3 | VTable 添加 `#[repr(C)]` | 小 |
| P4 | 常量命名（可选） | 小 |

