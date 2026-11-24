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

## Task 4：严格转换与字段

- `get_strict/get_to_strict/read_required/value_strict` 覆盖整数符号与边界、float 拒绝、浮点精度、非有限值、嵌套 Qt 容器路径、null/缺失及失败保留输出。
- 整数转浮点使用有效二进制位检查，覆盖 2^53+1 拒绝、2^53+2 接受和 2^63 精确接受，不依赖 MSVC long double 提供额外精度。
- 首轮 Qt 专项 5 个用例、268 个断言通过；实例化无符号严格读取产生的 C4127 已通过等价边界比较消除。
- `task4-ctest.log`：Qt、普通 conversions2、disabled_exceptions 和数据 fixture 共 4/4 通过。
- 标准库容器及领域 ADL 不在严格目标白名单内；编译期负向验证在安装消费阶段执行。

## Task 5：交付验证

### 配置与结果

| 形式 | 配置 | 结果 |
|---|---|---|
| 多头文件 | C++11 Debug | 11 个专项及数据 fixture 共 12/12 通过 |
| 多头文件 | C++20 Release | 10 个专项及数据 fixture 共 11/11 通过 |
| 单头文件 | C++20 Release | 同样的 10 个专项及数据 fixture 共 11/11 通过 |
| 安装多头文件消费方 | Debug，Qt 在前/JSON 在前/json_fwd 在前、多翻译单元 | 1/1 通过 |
| 安装单头文件消费方 | Release，相同消费程序 | 1/1 通过 |

专项为 Qt、constructor1/2、conversions2、deserialization、serialization、element_access1、iterators1、json_pointer、ordered_json；Debug 另含 disabled_exceptions。未运行完整上游测试全集。

最后自查补充了自定义 32 位内部整数存储读取为 64 位严格目标、QDateTime 元数据缺失路径、callback、QSet/QVector/QHash 严格递归、NUL 对象键及 dump replace/ignore。范围比较改用 intmax_t/uintmax_t，防止目标边界被窄化；QDateTime 元数据改为带字段路径的受检读取。QVariant 长分派按标量与业务元类型拆分，投影规则不变。

上述影响 Qt 功能的改动后，三种配置均重新构建 Qt 专项并通过 6 个用例、282 个断言；未受影响的上游专项复用对应配置已通过的结果。最终 Qt 专项日志为 `build-qt-multi/final-review-debug-test.log`、`build-qt-multi/final-review-release-test.log`、`build-qt-single/final-review-test.log`。原回归日志为两个构建目录中的 `final-*-ctest.log` / `final-ctest.log`，退出码均为 0。

### 安装与负向编译

- 安装目录为 `build-install-multi` / `build-install-single`，消费构建目录为 `build-consumer-multi` / `build-consumer-single`。
- 消费方只链接 `nlohmann_json::nlohmann_json`，没有自行添加 Qt include、compile definitions 或 link libraries；包含顺序、默认 Qt 宏和领域 ADL 验证成功。
- 两个消费配置的正向编译检查成功；`get_strict<std::vector<int>>()` 因不受支持目标的 static_assert 被拒绝，具有可抛出 ADL swap 的目标因没有可用 get_to_strict 重载被拒绝。CMakeConfigureLog.yaml 保留具体诊断。
- Qt 运行 DLL 通过 PATH 显式提供，安装测试不声称部署了 Qt runtime。

### 生成与边界

- AStyle 版本按 `tools/astyle/requirements.txt` 锁定为 3.4.13；生成 json/json_fwd/json_view 及 Bazel 清单后再生成，SHA-256 无变化。
- Qt 专项可执行文件 SHA-256 记录在 `build-qt-multi/final-binary-sha256.json`；构建输出保留在各 build 目录。
- README 新增 fork 入口，产品使用说明在 `docs/QtCore.md`；测试与执行过程只在本记录及计划中保存。
- 未修改 LibQtJson，未推送远端，未创建 PR。Qt6、其他操作系统/编译器、C++ modules、Bazel 实际构建和其他包管理器未验证。
