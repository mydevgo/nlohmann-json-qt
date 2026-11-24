# nlohmann-json-qt

nlohmann-json-qt 是 [nlohmann/json](https://github.com/nlohmann/json) 的 QtCore 扩展版本。在保留 `nlohmann::json` 操作方式及默认 STL 内部存储的基础上，直接支持 Qt 类型、QString 对象键、Qt 文档输入输出和严格转换。

项目采用纯头文件结构，QtCore 是必需依赖。公共入口仍为 `<nlohmann/json.hpp>`，同时支持 `nlohmann::json` 和维护对象成员插入顺序的 `nlohmann::ordered_json`。

## 功能概览

| 能力 | 主要接口或类型 |
|---|---|
| Qt 文档解析与校验 | `parse(QString/QByteArray)`、`accept(QString/QByteArray)` |
| Qt 文档输出 | `dump_utf8()` 返回 QByteArray，`dump_qstring()` 返回 QString |
| QString 对象键与路径 | `[]`、`at`、`find`、`contains`、`count`、`erase`、`value`、JSON Pointer |
| Qt 迭代器键 | 迭代器和 `items()` 迭代项的 `key_qstring()` |
| 文本与容器转换 | QString、QChar、QStringList、QList、QVector、QSet、QString 键的 QMap/QHash |
| 业务类型转换 | QDate、QTime、QDateTime、QUrl、QUuid |
| QVariant 与 QJson 桥接 | QVariant 固定白名单及 QJsonValue/Object/Array/Document |
| 严格读取 | `get_strict<T>()`、`get_to_strict()`、`read_required<T>()`、`value_strict<T>()` |
| 显式业务二进制 | `encode_base64()`、`decode_base64()` |
| Qt 转换错误 | `nlohmann::qt_error` 的错误类别和 JSON Pointer 路径 |

## 环境要求

- Qt 5.15 Core。
- C++11 或更新标准；启用 C++ 异常可获得可恢复的 Qt 转换错误处理。
- CMake：库配置最低 3.5，仓库测试最低 3.13；下方安装命令要求 3.15 或更新版本，建议使用近期版本。
- Windows 示例使用 Visual Studio 2022 的 MSVC v143、x64 和 PowerShell；Qt 路径示例为 `D:/Qt/Qt5.15/5.15.2/msvc2019_64`。

已验证的组合为 Windows x64、Qt 5.15.2、MSVC v143，覆盖 C++11 Debug 和 C++20 Release。Qt6、其他操作系统及编译器、C++ modules、Bazel 实际构建和其他包管理器尚未验证。

## 快速使用

```cpp
#include <nlohmann/json.hpp>

using Json = nlohmann::json;

Json sJson = Json::parse(QStringLiteral(R"({"数量":3})"));
sJson[QStringLiteral("名称")] = QStringLiteral("手机壳");

QString strName = sJson.at(QStringLiteral("名称")).get<QString>();
int nCount = sJson.read_required<int>(QStringLiteral("/数量"));
QByteArray bytesDocument = sJson.dump_utf8();
QString strDocument = sJson.dump_qstring(2);
```

Qt 容器可以直接构造 JSON 并按目标类型读回：

```cpp
QMap<QString, QList<QString>> mapValues{
    {QStringLiteral("名称"), {QStringLiteral("中文"), QStringLiteral("手机壳")}}
};
Json sJson = mapValues;
QMap<QString, QList<QString>> mapBack = sJson.get<QMap<QString, QList<QString>>>();
```

普通 `get<T>()` 保留上游数值转换语义，例如 `Json(1.5).get<int>()` 得到 1。需要拒绝类型、范围或精度损失时，显式使用 `get_strict<T>()` 等严格接口。

## 在应用中接入

### 作为源码子项目

把此仓库放入应用的 `third_party/nlohmann-json-qt`，在应用的 CMake 配置中使用：

```cmake
list(APPEND CMAKE_PREFIX_PATH "D:/Qt/Qt5.15/5.15.2/msvc2019_64")
add_subdirectory(third_party/nlohmann-json-qt)
target_link_libraries(MyApp PRIVATE nlohmann_json::nlohmann_json)
```

上述链接语句应位于 `MyApp` 目标创建之后。原 target 自动传播 QtCore 依赖，MSVC 下还会传播 `/utf-8`。

### 使用安装包

按后文命令安装后，在应用中设置安装前缀与 Qt 路径：

```cmake
list(APPEND CMAKE_PREFIX_PATH
    "E:/DemoProject/nlohmann-json-qt/build-install"
    "D:/Qt/Qt5.15/5.15.2/msvc2019_64")
find_package(nlohmann_json CONFIG REQUIRED)
target_link_libraries(MyApp PRIVATE nlohmann_json::nlohmann_json)
```

手工集成头文件时也需配置 QtCore 的头文件和链接参数。单头文件版同样依赖 QtCore，应用部署时还需提供匹配的 Qt 运行库。

## 配置、编译与测试

以下命令均在仓库根目录的 PowerShell 中执行。先按本机实际安装位置设置 Qt 路径；同一个构建目录不要混用生成器、架构或工具集。

```powershell
$qtRoot = 'D:/Qt/Qt5.15/5.15.2/msvc2019_64'
$env:PATH = "$qtRoot/bin;$env:PATH"
$env:CL = "$env:CL /FS"
```

构建前检查旧的 cmake、MSBuild、cl 和 mspdbsrv 进程；确认其他构建已结束，或确认是残留后再清理。若构建中断，不要立即叠加第二次构建。下面使用单进程 MSBuild 并关闭文件访问跟踪。

### 多头文件版

配置并注册 C++11、C++20 测试目标：

```powershell
cmake -S . -B build-qt-multi -G "Visual Studio 17 2022" -A x64 -T v143 `
  "-DCMAKE_PREFIX_PATH=$qtRoot" `
  '-DJSON_TestStandards=11;20' `
  -DJSON_BuildTests=ON -DJSON_FastTests=ON -DJSON_MultipleHeaders=ON
```

编译并运行 Qt 专项：

```powershell
cmake --build build-qt-multi --config Debug --target test-qt_cpp11 -- /m:1 /v:minimal /p:TrackFileAccess=false
ctest --test-dir build-qt-multi -C Debug -R '^test-qt_cpp11$' --output-on-failure

cmake --build build-qt-multi --config Release --target test-qt_cpp20 -- /m:1 /v:minimal /p:TrackFileAccess=false
ctest --test-dir build-qt-multi -C Release -R '^test-qt_cpp20$' --output-on-failure
```

项目本身是纯头文件库，没有需要单独编译的 JSON DLL 或静态库；以上编译的是使用该库的测试程序。

### 单头文件版

使用独立构建目录，复用上一步 CTest 准备的测试数据：

```powershell
cmake -S . -B build-qt-single -G "Visual Studio 17 2022" -A x64 -T v143 `
  "-DCMAKE_PREFIX_PATH=$qtRoot" `
  "-DJSON_TestDataDirectory=$PWD/build-qt-multi/test_files" `
  -DJSON_TestStandards=20 -DJSON_BuildTests=ON -DJSON_FastTests=ON -DJSON_MultipleHeaders=OFF

cmake --build build-qt-single --config Release --target test-qt_cpp20 -- /m:1 /v:minimal /p:TrackFileAccess=false
ctest --test-dir build-qt-single -C Release -R '^test-qt_cpp20$' --output-on-failure
```

如果没有执行多头文件版的测试，可省略 `JSON_TestDataDirectory`，由 CTest 准备数据，或将它设置为已解压的测试数据目录。

### 相关回归测试

修改构造、转换、解析、对象访问或迭代接口时，可在多头文件版的 C++20 Release 配置下运行以下相关回归：

```powershell
cmake --build build-qt-multi --config Release --target `
  test-qt_cpp20 test-constructor1_cpp20 test-constructor2_cpp20 test-conversions2_cpp20 `
  test-deserialization_cpp20 test-serialization_cpp20 test-element_access1_cpp20 `
  test-iterators1_cpp20 test-json_pointer_cpp20 test-ordered_json_cpp20 `
  -- /m:1 /v:minimal /p:TrackFileAccess=false

ctest --test-dir build-qt-multi -C Release --output-on-failure `
  -R '^test-(qt|constructor[12]|conversions2|deserialization|serialization|element_access1|iterators1|json_pointer|ordered_json)_cpp20$'
```

CTest 只运行已注册的测试，执行前必须编译对应目标；未加筛选条件的 CTest 可能包含尚未构建的测试。`JSON_FastTests=ON` 会跳过部分耗时场景，以上专项不代表完整测试全集。

上游测试数据使用 `nlohmann/json_test_data` 的 v3.2.0，默认在 CTest 的数据准备阶段下载。离线运行时需预先准备该版本数据，并在配置时设置 `JSON_TestDataDirectory`。可用 `ctest --test-dir build-qt-multi -C Release -N` 查看已注册测试。

## 安装与消费验证

安装多头文件版头文件和 CMake 包：

```powershell
cmake --install build-qt-multi --config Release --prefix "$PWD/build-install"
```

只需安装包时，可在配置阶段设置 `-DJSON_BuildTests=OFF -DJSON_Install=ON`，随后直接安装。纯头文件库的安装不需要先生成库二进制。

通过仓库内的消费程序检查安装后的 Qt 依赖传播、包含顺序、多翻译单元和编译期约束：

```powershell
cmake -S tests/qt_consumer -B build-consumer -G "Visual Studio 17 2022" -A x64 -T v143 `
  "-DCMAKE_PREFIX_PATH=$PWD/build-install;$qtRoot"
cmake --build build-consumer --config Release -- /m:1 /v:minimal /p:TrackFileAccess=false
ctest --test-dir build-consumer -C Release --output-on-failure
```

配置日志中 `QT_CONSUMER_POSITIVE` 应成功；`QT_STRICT_VECTOR_COMPILES` 和 `QT_THROWING_SWAP_COMPILES` 应失败，它们是预期的负向编译检查。单头文件安装可将安装源改为 `build-qt-single` 并使用独立安装前缀及消费构建目录。

## 使用约定

- 同一程序的各模块统一使用此 fork 的头文件和配置宏，并重新编译相关模块。名称相同不意味着与原版具备二进制兼容性；避免重复引入其他库对相同 Qt 类型的 `adl_serializer` 特化。
- QString 和对象键严格检查 UTF 编码。Qt 文档输入拒绝原始 NUL，JSON 字符串中的转义 NUL 合法。
- 裸 QByteArray 不提供值构造或 `get<QByteArray>()`。文档使用 `parse()`，业务字节使用显式 Base64 接口。
- QVariant 按固定白名单投影，不自动恢复原始元类型。QJson 桥接拒绝 Qt5 double 无法精确表示的整数。
- QDateTime 使用本项目约定的对象格式，并交叉校验时间和时区。外部协议要求其他格式时，应显式定义领域转换。
- 严格接口支持明确的目标类型及 Qt 容器递归，标准库容器和领域 ADL 不会自动获得严格递归支持。

## 文档

| 文档 | 内容 |
|---|---|
| [Qt 扩展点与使用说明](docs/Qt扩展点与使用说明.md) | 扩展总览、类型映射、错误模型、兼容边界与源码索引 |
| [QtCore 使用指南](docs/QtCore.md) | 接口示例、日期时间格式、普通与严格转换 |
| [验证记录](docs/validation/2026-11-24-qt-core-support.md) | 验证环境、场景、结果与证据位置 |
| [上游 API 文档](https://json.nlohmann.me/) | 原生 JSON 操作和通用接口参考 |

维护源码时修改 `include/nlohmann`，再通过 amalgamate 工具生成 `single_include`，不直接编辑生成头文件。Windows 调用该 Python 工具时使用 `python -X utf8`；格式化工具版本由 `tools/astyle/requirements.txt` 固定。

## 许可与上游来源

本项目基于 Niels Lohmann 的 [nlohmann/json](https://github.com/nlohmann/json)，采用 [MIT 许可](LICENSE.MIT)。分发时应保留相应版权及许可声明。
