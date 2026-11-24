# QtCore Support Implementation Plan

> Execute tasks inline in this session, with validation and one commit after each completed task. User has authorized commits. Do not push.

**Goal:** 为 nlohmann::json 提供内建 QtCore 接口，保持普通上游转换语义。

**Architecture:** 保留 header-only、内部 STL 存储和原 target。Qt 转换按职责放入独立头文件，basic_json 增加入口并调用它们。生成单头文件后与多头文件验证相同契约。

**Tech Stack:** C++11/C++20、Qt 5.15.2、MSVC v143、CMake、doctest、Python amalgamate。

**Spec:** [设计](../specs/2026-11-24-qt-core-support-design.md)

## Global Constraints

- QtCore 必需，不新增开关，不修改 LibQtJson。
- 普通 get 的数值语义不变，严格转换单独命名。
- 不直接编辑 single_include 生成文件；不修改未授权文件。
- 每项验证通过后使用明确文件列表暂存并提交；提交受环境策略限制时报告，不绕过。

## Task 0: Design and plan

- [x] 检查分支和工作区，完成设计审阅。
- [x] 记录任务、接口、验证方式和提交边界。
- [x] 提交设计与计划：`2db48bb89 docs: 确认 QtCore 扩展设计与实施计划`。

## Task 1: QtCore build integration

Files: `CMakeLists.txt`, `cmake/config.cmake.in`, `cmake/pkg-config.pc.in`。

- [x] 使用 `find_package(Qt5 5.15 REQUIRED COMPONENTS Core)`，在原 INTERFACE target 链接 `Qt5::Core`，安装包调用 `find_dependency`，pkg-config 声明 `Requires: Qt5Core`。
- [x] 配置独立 build 目录，构建并运行转换、构造、解析、序列化、对象、迭代、Pointer 和 ordered_json 基线。
- [x] 记录结果、更新清单并提交：`build: 接入必需的 QtCore 依赖`。

## Task 2: Qt text and object interfaces

Files: `include/nlohmann/QtJsonText.hpp`, `include/nlohmann/QtJsonAdapters.hpp`, `include/nlohmann/json.hpp`, JSON Pointer/iterator headers, `tests/src/unit-qt.cpp`, generated headers and BUILD.bazel。

Interfaces: strict UTF-16/UTF-8 helpers; `parse/accept(QString|QByteArray)`; `dump_utf8/dump_qstring`; QString object overloads; Pointer constructor; `key_qstring()`。

- [x] 添加 doctest 用例：QString 和 QByteArray 的 const/非 const 左值与右值、emoji、NUL、非法编码、allow_exceptions=false、QString 对象键及标准重载。
- [x] 构建用例确认缺失接口失败；实现文本辅助和 Qt 值适配分派，再实现成员、路径和迭代器入口。
- [x] 生成头文件，运行 Qt 专项与受影响基线；检查生成文件和清单同步。
- [x] 更新记录并提交：`feat: 原生支持 Qt 文本和对象操作`。

## Task 3: Qt value and container adapters

Files: Qt adapter headers, `tests/src/unit-qt.cpp`, generated headers and BUILD.bazel。

Interfaces: templated `adl_serializer` for Qt types, `encode_base64/decode_base64`。

- [x] 用例验证嵌套 QList/QVector/QSet/QMap/QHash、日期时间和时区、URL、UUID、QVariant 白名单、QJson 精度和 Base64 规范性；失败时输出不变。
- [x] 分别实现容器、业务值、QVariant、QJson 桥接与显式二进制接口，不接管领域 ADL。
- [x] 验证普通 QList<int> 从 `[1.5]` 读取为 1，裸 QByteArray 值转换在编译期不可用。
- [x] 生成头文件，运行 Qt 专项和转换回归；更新记录并提交：`feat: 支持 QtCore 值与容器转换`。

## Task 4: Strict conversion

Files: `include/nlohmann/QtJsonStrict.hpp`, Qt error header, `json.hpp`, Qt tests and generated headers。

Interfaces: `get_strict<T>()`, `get_to_strict(T&)`, `read_required<T>(QString)`, `value_strict<T>(QString,T)`; `qt_error` retains category and Pointer。

- [x] 用例验证整数上下界、符号、1.0、1.5、2^53+1、2^53+2、double/float 精度、NaN/Infinity、嵌套路径、缺失/null/非法值区别。
- [x] 对数值实现不依赖 long double 精度的范围/往返检查；对 Qt 容器递归调用严格读取；不支持目标编译期诊断。
- [x] 验证失败保留 get_to_strict 输出，普通 get 回归不变；生成头文件。
- [x] 更新记录并提交：`feat: 增加严格转换和字段读取接口`。

## Task 5: Packaging and documentation

Files: Qt usage docs, README entry, installed-consumer tests, validation record, generated headers。

- [ ] C++11 Debug、C++20 Release、多头/单头专项和相关回归。
- [ ] 包含顺序、多翻译单元、安装消费程序：`find_package(nlohmann_json CONFIG REQUIRED)` 后只链接 `nlohmann_json::nlohmann_json`。
- [ ] 使用指南说明类型格式、错误、binary 语义、Qt5 与平台限制；不把任务过程写进产品说明。
- [ ] 连续生成校验文件指纹一致，检查最终 diff、提交边界和剩余工作区。
- [ ] 更新记录并提交：`docs: 完善 Qt 扩展用法和安装验证`。
