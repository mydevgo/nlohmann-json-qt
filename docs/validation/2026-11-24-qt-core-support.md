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
