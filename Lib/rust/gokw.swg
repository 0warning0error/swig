/* Rename keywords.  */

#define GOKW(x) %keywordwarn("'" `x` "' is a Rust keyword",rename="X%s")  `x`
#define GOBN(x) %builtinwarn("'" `x` "' conflicts with a built-in name in Rust")  "::"`x`

// 标准关键字
GOKW(as);      // 强制类型转换，或 use 和 extern crate 包和模块引入语句中的重命名
GOKW(break);   // 立刻退出循环
GOKW(const);   // 定义常量或原生常量指针（constant raw pointer）
GOKW(continue); // 继续进入下一次循环迭代
GOKW(crate);   // 链接外部包
GOKW(dyn);     // 动态分发特征对象
GOKW(else);    // 作为 if 和 if let 控制流结构的 fallback
GOKW(enum);    // 定义一个枚举类型
GOKW(extern);  // 链接一个外部包，或者一个宏变量（该变量定义在另外一个包中）
GOKW(false);   // 布尔值 false
GOKW(fn);      // 定义一个函数或函数指针类型 (function pointer type)
GOKW(for);     // 遍历一个迭代器或实现一个 trait 或者指定一个更高级的生命周期
GOKW(if);      // 基于条件表达式的结果来执行相应的分支
GOKW(impl);    // 为结构体或者特征实现具体功能
GOKW(in);      // for 循环语法的一部分
GOKW(let);     // 绑定一个变量
GOKW(loop);    // 无条件循环
GOKW(match);   // 模式匹配
GOKW(mod);     // 定义一个模块
GOKW(move);    // 使闭包获取其所捕获项的所有权
GOKW(mut);     // 在引用、裸指针或模式绑定中使用，表明变量是可变的
GOKW(pub);     // 表示结构体字段、impl 块或模块的公共可见性
GOKW(ref);     // 通过引用绑定
GOKW(return);  // 从函数中返回
GOKW(Self);    // 实现特征类型的类型别名
GOKW(self);    // 表示方法本身或当前模块
GOKW(static);  // 表示全局变量或在整个程序执行期间保持其生命周期
GOKW(struct);  // 定义一个结构体
GOKW(super);   // 表示当前模块的父模块
GOKW(trait);   // 定义一个特征
GOKW(true);    // 布尔值 true
GOKW(type);    // 定义一个类型别名或关联类型
GOKW(unsafe);  // 表示不安全的代码、函数、特征或实现
GOKW(use);     // 在当前代码范围内(模块或者花括号对)引入外部的包、模块等
GOKW(where);   // 表示一个约束类型的从句
GOKW(while);   // 基于一个表达式的结果判断是否继续循环

// 保留关键字（保留做将来使用）
GOKW(abstract); // 保留关键字
GOKW(async);    // 保留关键字
GOKW(await);    // 保留关键字
GOKW(become);   // 保留关键字
GOKW(box);      // 保留关键字
GOKW(do);       // 保留关键字
GOKW(final);    // 保留关键字
GOKW(macro);    // 保留关键字
GOKW(override); // 保留关键字
GOKW(priv);     // 保留关键字
GOKW(try);      // 保留关键字
GOKW(typeof);   // 保留关键字
GOKW(unsized);  // 保留关键字
GOKW(virtual);  // 保留关键字
GOKW(yield);    // 保留关键字

GOBN(map);

#undef GOKW
