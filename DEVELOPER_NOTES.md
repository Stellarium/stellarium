# Stellarium Developer Notes

> 整理自 `MAINTAINER_BUSINESS.md`、`codingStyle.html`、Wiki 中的 Developers 部分。

---

## 编码规范

### 文件与命名
- `.hpp` / `.cpp` 配对，C 兼容代码用 `.h` / `.c`
- 文件名与类名一致：`class StelFontMgr` → `StelFontMgr.hpp`
- 类名：`CamelCase` 大写开头，名词
- 方法名：`camelCase` 小写开头，动词。返回值用 `getXxx()`
- Qt signal 名用被动语态（`valueChanged()`），slot 用主动动词（`handleXxx()`）
- 局部变量/形参：`camelCase` 小写开头，**不要用下划线开头**
- 宏/static const：`ALL_CAPS` 下划线分隔

### 缩进与格式
- **用 tab 缩进，tab 宽度 = 8**
- 大括号**另起一行**（Allman 风格）
- 列宽限制：120 字符
- 可用 astyle 格式化：`astyle --style=ansi -tU source.cpp`
- `.clang-format` 已配置，QtCreator ≥ 13 可直接用

### 空白
- 方法间：1 空行
- 逻辑段间：1 空行
- 源文件段落间：2 空行

### 注释
- 用 Doxygen `//!` 格式
- 公共/保护成员必须在 `.hpp` 中完整注释
- 简短描述一句，到第一个句号为止

### C++ 风格
```cpp
// 用 Qt 容器代替 STL（除非性能关键）
QString 代替 std::string
QIODevice 代替 fopen()
// 用引用代替指针
// C++ 头文件风格
#include <cstdio>   // 好
#include <stdio.h>  // 差
// 用 std::math 函数
double val = std::cos(lat);   // 好
double val = cos(lat);         // 差
```

### 翻译字符串
- `q_("text")` — 翻译并返回 QString
- `qc_("text", "context")` — 带上下文的翻译
- `N_("text")` — 仅标记翻译，不返回翻译
- 不要拼接字符串，用 `QString::arg()`
- 控制台不要输出翻译文本

### enums
```cpp
//! @enum EnumName
enum EnumName {
    EnumValueOne,  //!< 描述
    EnumValueTwo   //!< 描述
};
```

---

## 外部库引入

- 优先 `find_package()`
- Fallback 用 `CPMAddPackage()` + `DOWNLOAD_ONLY YES`
- 提供 alias 匹配 `find_package` 的导出名

---

## Git 工作流

### 分支策略
- **永远不要直接提交到 master**
- 从 master 创建 feature 分支
- Rebase 而非 merge（保持线性历史）
- rebase 前关闭 QtCreator 等 IDE
- rebase 后必须重新测试

### 标准流程
```bash
git checkout -b feature/my-awesome-feature
# ... 开发中 ...
git checkout master
git pull upstream master
git checkout feature/my-awesome-feature
git rebase master        # 如果有冲突，解决后 git rebase --continue
# 重新测试！
git push origin feature/my-awesome-feature
# → 创建 Pull Request
```

### 提交信息关键字
- `[publish]` — 激活 CI 构建发布包
- `[ci skip]` / `[skip ci]` — 跳过 CI

### PR 被合并后
```bash
git branch -d feature/my-awesome-feature
git push origin :feature/my-awesome-feature
```

---

## 资源文件

- 新增 PNG 图片需用 ect 压缩：`ect -9 image.png`
- 不要修改图片 metadata

---

## 版本与发布

- 一年 4 个正式发布（分至点附近）：`YY.0`, `YY.1`, `YY.2`, `YY.3`
- 每周构建 beta（周末 snapshot），版本号如 `1.22.3-410abbe`
- Qt5 系列为 `0.YY.N`，Qt6 系列为 `1.YY.N`
- 用户安装版中是 `1.YY.N`（Qt6），实际版本号 26.1 对应 v26.1
