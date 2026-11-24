# QtCore 扩展验证记录

## 环境与基线

- 分支 `codex/qt-core-support`，上游基线 `322063822`。
- Windows x64，VS2022 MSVC 19.44/v143，Qt 5.15.2 msvc2019_64。
- 构建目录 `build-qt-multi`，生成器 Visual Studio 17 2022，JSON_TestStandards=11，JSON_FastTests=ON。
- C++11 Debug 的 constructor1/2、conversions2、deserialization、serialization、element_access1、iterators1、json_pointer、ordered_json 共 9 个上游专项均已构建。
- 首轮 CTest（`build-qt-multi/baseline-ctest.log`）：测试数据 fixture 和 8 个专项通过，deserialization 的 JSON Lines 有 6 个断言失败。编译日志同时有 C4819；对应源码包含 UTF-8 花色字符，系统代码页为 936。
- 原 target 增加 MSVC `/utf-8` 后重建 deserialization，`baseline-deserialization-utf8.log` 显示 fixture 与专项均通过，退出码 0。其余已通过专项未因核心源码修改失效，后续功能阶段继续回归。
- Qt 文本专项的首次构建失败：接口尚未实现，且源文件有 C4819；后续在 `/utf-8` 下实现并验证。

## Task 1：构建接入

- CMake 原 INTERFACE target 必需链接 Qt5::Core，安装配置查找 Qt5 Core，pkg-config 声明 Qt5Core >= 5.15。
- configure 成功；构建接入及解析基线通过。
- 安装消费、单头文件及新增 Qt 功能尚未在本阶段验证。

## Task 2：Qt 文本与对象接口

- Qt 文本专项：json/ordered_json、左右值输入、const 访问、QString 键/Pointer/迭代器、Unicode/NUL 和非法编码，共 2 个用例、65 个断言通过。
- 首次回归编译发现 `find("x")` 在标准字符串和 QString 间产生歧义；Qt 对象键与 Pointer 构造改为仅匹配实际 QString 的受限模板。
- 修正后 `task2-ctest.log`：10 个专项与数据 fixture 共 11/11 通过，退出码 0。
- 使用仓库锁定的 AStyle 3.4.13 格式化变更源文件；生成 json/json_fwd/json_view 单头文件并更新 BUILD.bazel 清单。
- Windows 下 amalgamate 的 `/` 与 `\\` 路径身份不一致会重复展开根头文件；已统一路径身份。生成命令使用 Python UTF-8 模式，避免本机 GBK 解码失败。
- 单头文件的运行验证保留在打包阶段，不用多头文件测试代替。

## Task 3：QtCore 值与容器

- 已覆盖所有约定 Qt 类型、QString 键嵌套映射、QSet 去重、普通 QList<int> 小数截断、失败时保留容器输出、QVariant 各内置白名单投影、QJson 精度拒绝及 Base64 规范检查。
- UTC、非零固定偏移、系统本地时间和 UTC 命名时区往返通过；未知时区和矛盾偏移被拒绝。
- QSet 首轮编译暴露其没有双参数 insert；按序列追加与集合插入分别分派后通过。
- `task3-ctest.log`：Qt 专项、conversions2、disabled_exceptions 和数据 fixture 共 4/4 通过，退出码 0。禁用异常专项验证的是原有标准接口，不承诺 Qt 错误路径可恢复。
- 多头文件源已用锁定 AStyle 格式化，单头文件与 Bazel 文件清单已生成；单头文件完整验证仍在打包阶段执行。
