# QtCore 支持

项目扩展总览、接口选择和源码职责见 [Qt 扩展点与使用说明](Qt扩展点与使用说明.md)。

nlohmann-json-qt 是 nlohmann/json 的 QtCore 扩展 fork，保持 `nlohmann::json` 的操作方式、STL 内部存储和 header-only 结构。它是原版的替代品，同一程序内各模块必须统一使用此 fork 的头文件并重新编译。

## 依赖与接入

要求 Qt 5.15.x 和 C++11 或更新标准。已验证环境为 Windows x64、Qt 5.15.2、MSVC v143。Qt 6、其他平台、C++ modules、Bazel 和其他包管理器尚未验证；仅同步 Bazel 的源码清单不代表已接入其 Qt 依赖。

通过 `add_subdirectory` 或安装包链接同一个 target：

```cmake
list(APPEND CMAKE_PREFIX_PATH "D:/Qt/Qt5.15/5.15.2/msvc2019_64")
find_package(nlohmann_json CONFIG REQUIRED)
target_link_libraries(MyApp PRIVATE nlohmann_json::nlohmann_json)
```

QtCore 是必需依赖，target 自动传播 Qt 头文件和链接要求。MSVC 使用 `/utf-8`。手动包含头文件时也需提供 QtCore 的编译与链接参数。`JSON_MultipleHeaders=ON/OFF` 分别选择多头文件和生成的单头文件版。

## 文本、对象与文档

```cpp
#include <nlohmann/json.hpp>

using Json = nlohmann::json;

Json sJson = Json::parse(QStringLiteral(R"({"数量":3})"));
sJson[QStringLiteral("名称")] = QStringLiteral("手机壳");
QString strName = sJson.at(QStringLiteral("名称")).get<QString>();
QByteArray bytesDocument = sJson.dump_utf8();
QString strDocument = sJson.dump_qstring(2);
```

`parse` 和 `accept` 接受 QString 或 QByteArray，覆盖 const、非 const 左值和右值；callback、异常开关、注释和尾逗号参数沿用上游语义。Qt 文档输入严格检查编码并按完整长度读取，原始 NUL 被拒绝；JSON 中的 `\u0000` 合法。`parse(..., nullptr, false)` 在非法输入时返回 discarded，`accept` 返回 false。

`dump_utf8` 返回 QByteArray，`dump_qstring` 返回 QString，参数与原 `dump` 一致：indent、indent_char、ensure_ascii、error_handler。原 `dump` 继续返回 std::string。选择 replace/ignore 错误策略会沿用上游的替换/忽略规则。

QString 值和键保留中文、emoji、BOM 及内嵌 NUL；无效 UTF-8 或孤立 UTF-16 代理项报错。QString 的 null 状态归一化为空字符串。QString 键适用于 `operator[]/at/find/contains/count/erase/value`；标准字符串、字符串常量和数组下标继续可用。

JSON Pointer 可以从 QString 构造，仍使用 RFC 6901。迭代器与 `items()` 的 `key_qstring()` 返回 Qt 文本；原 `key()` 保持内部字符串类型。

## Qt 类型映射

| 类型 | JSON 表示 |
|---|---|
| QString、QChar | string；QChar 只接受单个非代理 BMP 字符 |
| QStringList、QList、QVector | array，保持顺序 |
| QSet | array，按 Qt 语义去重，输出顺序不保证 |
| QMap<QString,T>、QHash<QString,T> | object；非 QString 键不提供适配 |
| QDate | `yyyy-MM-dd`，有效年份 0001～9999 |
| QTime | `HH:mm:ss.zzz` |
| QDateTime | 下面定义的对象格式 |
| QUrl | FullyEncoded URI string，StrictMode 规范回验，允许相对 URL，拒绝空 URL |
| QUuid | 小写无括号 string，零 UUID 合法，输入允许大小写 |
| QVariant | 白名单值投影，不保存元类型标记 |
| QJsonValue/Object/Array/Document | JSON 桥接，检查 Qt5 double 的精度限制 |

```cpp
QMap<QString, QList<QString>> mapValues{
    {QStringLiteral("名称"), {QStringLiteral("中文"), QStringLiteral("emoji 😀")}}
};
Json sJson = mapValues;
QMap<QString, QList<QString>> mapBack = sJson.get<QMap<QString, QList<QString>>>();
```

构造、赋值、`get<T>()` 和 `get_to()` 均可使用这些类型。普通 Qt 容器逐元素使用普通 `get<T>()`，因此 `[1.5]` 普通读取为 QList<int> 时得到 1。Qt 值与容器使用临时结果提交，转换失败不交付部分输出；领域 ADL 的外部副作用不提供回滚保证。

### QDateTime 格式

对象字段为 `date`、`time`、`epochMilliseconds`、`timeSpec`、`offsetSeconds`。timeSpec 为 UTC、OffsetFromUTC、LocalTime 或 TimeZone；后两者另含 `timeZoneId`。

读取时交叉校验 epoch、墙上日期时间、偏移和时区；时区不可用、LocalTime 与系统时区不一致或字段矛盾时报错。仅保证毫秒精度，不保证保存任意自定义时区的额外元数据。需要通用 ISO 文本格式的协议应在领域 ADL 中显式定义。

### QVariant 和 QJson

QVariant 白名单包含无效 QVariant、bool、标准整型元类型、float/double、QString/QChar、QStringList、QVariantList/Map/Hash、日期时间、URL/UUID 和 QJson 类型。其他模板容器、用户元类型、指针及 QByteArray 被拒绝，不使用 toString 或 canConvert 兜底。

反向投影：null 为无效 QVariant；有符号/无符号整数为 qlonglong/qulonglong，float 为 double，string 为 QString，array/object 为 QVariantList/QVariantMap。日期等字符串不恢复原元类型，QDateTime 对象不猜测为日期时间。非有限浮点值被拒绝。

QJson 拒绝 Undefined、null document 和 double 无法精确表示的整数；QJsonDocument 根只接受 object/array。进入 QJson 前已经发生的精度损失无法追溯诊断。

## 业务字节

QByteArray 作为文档使用 `parse`。业务二进制显式使用 Base64：

```cpp
Json sBinary = Json::encode_base64(bytesPayload);
QByteArray bytesBack = sBinary.decode_base64();
```

编码输出标准 Base64 JSON string；解码拒绝空白、错误填充、非标准字母表和非规范尾部位。空字节序列合法。裸 QByteArray 不提供值构造、赋值或 `get<QByteArray>()`；`json::binary()` 保留上游的原生二进制含义。

## 严格读取与错误

```cpp
int nCount = sJson.at("count").get_strict<int>();
quint64 nId = sJson.read_required<quint64>(QStringLiteral("/items/0/id"));
QString strNote = sJson.value_strict<QString>(QStringLiteral("note"), QStringLiteral("无"));
QList<int> lstItems;
sJson.at("items").get_to_strict(lstItems);
```

普通 `get<T>()` 保留上游数值语义。严格接口拒绝整型目标的 float 输入（包括 1.0）、符号/范围错误、浮点精度损失和非有限值；递归覆盖受支持的 Qt 容器。标准库容器、领域 ADL、自定义序列化器和 char 不在严格目标白名单内，编译期拒绝。

`read_required` 读取 RFC 6901 Pointer，区分缺失、null、父节点类型和非法值。`value_strict` 读取直接对象键，仅缺失时使用默认值。null 可以读为 std::nullptr_t、无效 QVariant 或 QJsonValue Null，不自动变成零或空字符串。

`get_to_strict` 仅对可无异常交换的目标提供接口，失败保留旧值。`nlohmann::qt_error` 提供 `category` 和 QString `path`，继承原 `json::exception`；容器/字段路径对 `/`、`~` 分别转义为 `~1`、`~0`。原有标准接口仍抛出原生异常。Qt 文档解析使用 parse_error/discarded。内存分配失败正常传播。

`JSON_NOEXCEPTION` 下沿用上游终止进程的错误处理方式，不保证 Qt 非法编码等预检查能通过 allow_exceptions=false 恢复。需要可恢复 Qt 错误时启用 C++ 异常。

## 构建与测试

```powershell
cmake -S . -B build-qt -G "Visual Studio 17 2022" -A x64 -T v143 `
  -DCMAKE_PREFIX_PATH=D:/Qt/Qt5.15/5.15.2/msvc2019_64 `
  -DJSON_TestStandards=11 -DJSON_FastTests=ON
cmake --build build-qt --config Debug --target test-qt_cpp11 -- /m:1 /p:TrackFileAccess=false
ctest --test-dir build-qt -C Debug -R '^test-qt_cpp11$' --output-on-failure
```

运行前将对应 Qt bin 加入 PATH。生成单头文件时使用仓库锁定的 AStyle 版本；Windows 上调用 Python amalgamate 时加 `-X utf8`。不手动编辑 single_include。
