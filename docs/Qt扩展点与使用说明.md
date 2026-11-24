# Qt 扩展点与使用说明

本文介绍 nlohmann-json-qt 相对于上游 nlohmann/json 增加的 QtCore 能力、使用约定及边界。接口用法可结合 [QtCore 使用指南](QtCore.md) 阅读。

## 1. 项目定位

nlohmann-json-qt 在上游源码中直接扩展 `nlohmann::basic_json` 和 Qt 类型适配，对外继续使用 `nlohmann::json`、`nlohmann::ordered_json` 以及 `nlohmann::` 下的相关接口。

项目保持 header-only 结构，默认内部字符串、对象和数组仍采用上游的标准库存储方式。QString 等 Qt 类型在接口边界进行转换，并未把 JSON DOM 的内部存储替换为 Qt 容器。因此，QtCore 是必需链接依赖，但本项目不提供额外的 JSON DLL，也不要求通过另一套包装节点访问数据。

```cpp
#include <nlohmann/json.hpp>

using Json = nlohmann::json;

Json sJson;
sJson[QStringLiteral("名称")] = QStringLiteral("手机壳");
sJson[QStringLiteral("数量")] = 3;
QString strName = sJson.at(QStringLiteral("名称")).get<QString>();
```

`json` 与 `ordered_json` 共享 Qt 扩展能力。前者采用默认对象排序方式，后者维护对象成员插入顺序；二者依旧是不同的模板实例类型，不应混淆。

## 2. 扩展点总览

| 扩展区域 | 提供的能力 | 主要入口 |
|---|---|---|
| Qt 文档输入 | 直接解析或校验 QString、QByteArray | `parse()`、`accept()` |
| Qt 文档输出 | 输出 UTF-8 字节或 QString | `dump_utf8()`、`dump_qstring()` |
| Qt 对象键 | 以 QString 读写、查找、删除对象成员 | `operator[]`、`at`、`find`、`contains`、`count`、`erase`、`value` |
| 路径与迭代 | QString JSON Pointer、Qt 文本键 | `json_pointer(QString)`、`key_qstring()` |
| Qt 文本值 | QString、QChar 与 JSON string 互转 | 构造、赋值、`get<T>()`、`get_to()` |
| Qt 容器 | 序列、集合、QString 键映射互转 | 同上，可嵌套 |
| Qt 业务值 | 日期、时间、URL、UUID 的固定格式 | 同上 |
| QVariant | 固定元类型白名单与 JSON 值投影 | `Json(sVariant)`、`get<QVariant>()` |
| QJson | 与 Qt 自带 JSON 类型桥接 | `Json(sQJson)`、`get<QJson…>()` |
| 显式二进制 | 原始业务字节与标准 Base64 string 互转 | `encode_base64()`、`decode_base64()` |
| 严格转换 | 检查类型、范围、精度并递归读取 Qt 容器 | `get_strict<T>()`、`get_to_strict()` |
| 严格字段读取 | 区分字段缺失、null 和非法值 | `read_required<T>()`、`value_strict<T>()` |
| Qt 转换错误 | 保留错误类别及 JSON Pointer 路径 | `qt_error`、`qt_error_category` |

## 3. Qt 文档与文本

### 3.1 文档输入输出

```cpp
Json sJson = Json::parse(QStringLiteral(R"({"名称":"手机壳","数量":3})"));
QByteArray bytesDocument = sJson.dump_utf8();
QString strDocument = sJson.dump_qstring(2);
bool bValid = Json::accept(bytesDocument);
```

QString 文档按 UTF-16 校验后转为 UTF-8；QByteArray 文档按完整字节范围处理。const、非 const 左值及右值均可使用。Qt 输入重载保留上游的解析 callback、异常开关、注释及尾逗号参数含义。

输出接口与原 `dump()` 使用相同的参数顺序：`indent`、`indent_char`、`ensure_ascii`、`error_handler`。原 `dump()` 仍返回内部 `string_t`，默认是 `std::string`；新增方法分别返回 QByteArray 和 QString。

Qt 文档入口拒绝无效编码和原始 NUL，不会把 NUL 后的内容静默忽略。JSON 字符串中通过 `\u0000` 表示的字符可以正常解析。普通字符串值和对象键也允许包含内嵌 NUL。

启用 C++ 异常时，`parse(bytes, nullptr, false)` 对无效 Qt 文档返回 discarded，`accept()` 返回 false。`JSON_NOEXCEPTION` 下不能依赖 Qt 转换预检查中的错误恢复；需要可恢复错误时应启用异常。

### 3.2 编码约定

- QString 与 JSON string 之间严格转换，保留中文、emoji、BOM 字符和内嵌 NUL。
- 孤立 UTF-16 代理项、非法或截断 UTF-8 会报错，不默认使用替代字符修复。
- QChar 只表示单个非代理 BMP 字符；emoji 等需要代理对的字符使用 QString。
- QString 的 null 状态不作为独立 JSON 类型保存，序列化为空字符串。
- 显式选择 dump 的 replace/ignore 策略时，遵循上游对应的替换或忽略行为。

Qt5 文本和容器的容量受 int 长度模型约束，相关转换会检查容量。项目没有增加统一的文档大小配额、深度配额或全局内存配额；应用需要的资源策略应在自身边界明确设置。

## 4. 对象键、路径与迭代

```cpp
const QString strKey = QStringLiteral("名称");
sJson[strKey] = QStringLiteral("手机壳");
bool bExists = sJson.contains(strKey);
QString strName = sJson.at(strKey).get<QString>();
QString strNote = sJson.value(QStringLiteral("备注"), QStringLiteral("无"));

Json::json_pointer sPointer(QStringLiteral("/名称"));
QString strByPointer = sJson.at(sPointer).get<QString>();

for (Json::const_iterator it = sJson.cbegin(); it != sJson.cend(); ++it)
{
    QString strMemberKey = it.key_qstring();
}
```

QString 键会严格编码为内部字符串类型。扩展没有改变原有对象操作的含义：非常量 `operator[]` 可以创建成员；检查必填字段通常应使用 `at()` 或 `read_required()`。`value()` 只对缺失成员使用默认值，不能当作所有转换错误的兜底。

JSON Pointer 沿用 RFC 6901：根路径为空字符串，`/` 分隔路径段，键中的 `~` 和 `/` 分别写成 `~0` 和 `~1`。单独的 QString 对象键不会被自动解释为 JSON Pointer。

迭代器及 `items()` 的迭代项提供 `key_qstring()`；原有 `key()` 的返回类型不变。`items()` 遍历数组时，Qt 文本键与原 key 一样表示数组索引。

## 5. Qt 值与容器映射

| Qt 类型 | JSON 表示 | 约束 |
|---|---|---|
| QString、QChar | string | 编码及字符数量规则见前文 |
| QStringList、QList<T>、QVector<T> | array | 保持顺序，元素需可转换 |
| QSet<T> | array | 按 Qt 语义去重，输出顺序不保证 |
| QMap<QString,T>、QHash<QString,T> | object | 不支持自动字符串化非 QString 键；QHash 顺序不保证 |
| QDate | `yyyy-MM-dd` string | 有效日期，年份 0001～9999 |
| QTime | `HH:mm:ss.zzz` string | 有效时间，保留毫秒 |
| QDateTime | 固定字段 object | 交叉校验 epoch、墙上时间与时区 |
| QUrl | FullyEncoded URI string | StrictMode 规范回验，允许有效相对 URL，拒绝空 URL |
| QUuid | 36 字符、无括号的小写 string | 输入允许十六进制大小写，零 UUID 合法 |

```cpp
QMap<QString, QList<QString>> mapValues{
    {QStringLiteral("名称"), {QStringLiteral("中文"), QStringLiteral("手机壳")}}
};
Json sValues = mapValues;
QMap<QString, QList<QString>> mapBack = sValues.get<QMap<QString, QList<QString>>>();
```

容器可以组合嵌套。普通容器转换逐元素使用普通 `get<T>()`，不会因为外层容器是 Qt 类型就自动改为严格数值转换。Qt 值和容器采用临时结果提交，失败时不交付部分转换结果；这一保证不涵盖用户领域 ADL 自行产生的外部副作用。

### QDateTime 的格式与可移植性

QDateTime object 包含 `date`、`time`、`epochMilliseconds`、`timeSpec`、`offsetSeconds`。`timeSpec` 为 `UTC`、`OffsetFromUTC`、`LocalTime` 或 `TimeZone`；后两者另含 `timeZoneId`。

反向转换不会只信任其中一个字段，而会恢复时间后交叉校验。时区不存在、LocalTime 的时区与当前系统不符、偏移或日期时间矛盾都会失败。它只保存 Qt5 毫秒级信息，不保证保存自定义时区对象的额外元数据。

这是本项目约定的格式。对接要求 ISO 8601 string 等其他格式的服务时，应在领域类型的 ADL 中显式定义协议，不应把默认 QDateTime object 直接当作任意外部协议的日期格式。

## 6. QVariant 和 QJson 的边界

### 6.1 QVariant 不保存完整类型信息

支持的 QVariant 元类型白名单包括：无效 QVariant、bool、标准整型元类型、float/double、QString/QChar、QStringList、QVariantList/Map/Hash、QDate/QTime/QDateTime、QUrl/QUuid，以及 QJson 类型。

JSON 反向投影采用以下规则：

| JSON 类型 | QVariant 保存的类型 |
|---|---|
| null | 无效 QVariant |
| boolean | bool |
| signed integer / unsigned integer | qlonglong / qulonglong |
| float | double |
| string | QString |
| array | QVariantList |
| object | QVariantMap |

因此，QVariant 中的 QDate 写入 JSON 后成为 string，再读为 QVariant 时得到 QString；QDateTime object 再读为 QVariant 时得到 QVariantMap。需要恢复特定类型时应使用 `get<QDate>()` 等明确目标类型。

任意模板容器的 QVariant、用户自定义元类型、指针和 QByteArray 不在自动支持范围内。即使 `QList<int>` 本身可以转换，把它包装成 QVariant 也不会自动加入白名单。实现不使用 `canConvert()` 或 `toString()` 兜底，也没有全局元类型注册表。

### 6.2 QJson 桥接会检查整数精度

QJsonValue、QJsonObject、QJsonArray、QJsonDocument 可以通过构造及 `get<T>()` 桥接。Qt5 QJson 以 double 模型表示数值，因此从 JSON 转入 QJson 时会拒绝不能精确表示的整数。

例如 `9007199254740993` 不能精确表示为 double，会失败；`9007199254740994` 可以精确表示。进入 QJson 之前已经发生的精度损失无法通过桥接追溯恢复。

QJson Undefined、null document 被拒绝；QJsonDocument 的根只接受 object 或 array。

## 7. 普通转换和严格转换如何选择

普通 `get<T>()` 保留上游数值规则；新增严格接口提供显式选择：

| 场景 | 普通接口 | 严格接口 |
|---|---|---|
| float `1.5` 读为 int | 得到 1 | 拒绝 |
| float `1.0` 读为 int | 得到 1 | 拒绝，输入仍是 float |
| 整数符号、范围 | 沿用上游转换规则 | 检查目标范围及符号 |
| 整数转浮点、double 转 float | 沿用上游规则 | 拒绝精度损失 |
| Qt 容器元素 | 普通 `get<T>()` | 递归严格读取 |
| NaN、Infinity 读取为浮点 | 沿用上游规则 | 拒绝 |

```cpp
Json sResponse = Json::parse(QByteArray(R"({"count":3,"items":[1,2]})"));
int nCount = sResponse.read_required<int>(QStringLiteral("/count"));
QString strNote = sResponse.value_strict<QString>(QStringLiteral("note"), QStringLiteral("无"));
QList<int> lstItems;
sResponse.at("items").get_to_strict(lstItems);
```

- `get_strict<T>()`：严格读取当前节点。
- `get_to_strict(T&)`：先转换再无异常交换，失败保留旧值；目标必须满足可无异常交换的约束。
- `read_required<T>(QString pointer)`：按 JSON Pointer 查找并严格读取，区分缺失、null、父节点类型和非法值。
- `value_strict<T>(QString key, default_value)`：读取直接对象键，仅缺失时使用默认值；存在但为 null 或非法值会失败。

严格目标包括整型（不含 char）、浮点、bool、std::nullptr_t、支持的 Qt 值及其可严格递归的 Qt 容器。标准库容器、领域 ADL 类型及自定义序列化器不会自动获得严格递归支持，不支持的目标在编译期被拒绝。

严格读取 null 时，std::nullptr_t、QVariant、QJsonValue 可以表达相应空值，普通数值和 QString 不会自动变成零或空字符串。严格读取也不会改变解析阶段已经采用的 double 舍入模型；项目不提供任意精度小数。

## 8. QByteArray 的两种含义

QByteArray 文档字节通过 `parse()` 解析为 JSON；业务字节通过显式接口转换为 Base64 string：

```cpp
QByteArray bytesDocument("{\"count\":3}");
Json sDocument = Json::parse(bytesDocument);

QByteArray bytesPayload("\0\xff", 2);
Json sBinary = Json::encode_base64(bytesPayload);
QByteArray bytesBack = sBinary.decode_base64();
```

`encode_base64()` 接收原始字节并产生标准 Base64 JSON string；`decode_base64()` 解码当前 JSON string。解码拒绝空白、错误填充、非标准字母表和非规范尾部位。

裸 QByteArray 不提供 JSON 值构造、赋值或 `get<QByteArray>()`，防止把文档、二进制和字符数组混为一谈。上游 `json::binary()` 的原生二进制节点仍保留，和 Base64 string 是不同表示。

## 9. 错误处理与路径

Qt 转换错误使用 `nlohmann::qt_error`，继承 `Json::exception`，包含 `category`、QString `path` 及继承的异常 ID。`what()` 为通用错误消息，判断业务分支时应读取类别，而不是解析消息文本。

类别包括 `encoding`、`type`、`range`、`precision`、`value`、`missing`、`null_value`、`unsupported`、`base64`。

```cpp
try
{
    int nId = sResponse.read_required<int>(QStringLiteral("/items/0/id"));
}
catch (const nlohmann::qt_error& rError)
{
    qWarning() << static_cast<int>(rError.category) << rError.path;
}
catch (const Json::exception& rError)
{
    qWarning() << rError.what();
}
```

上例日志需要包含 `<QDebug>`。先捕获 qt_error，才能读取 Qt 扩展字段；后面的原生异常分支用于其他 JSON 操作。

容器和字段入口会补充相对路径。例如从 `/items` 严格读取嵌套容器时，可报告 `/items/1/id`。先通过 `at()` 单独取得子节点再调用 `get_strict()` 时，不会自动恢复该节点相对于原文档的完整路径；需要完整上下文时使用 `read_required()`。

原有标准接口继续使用原生异常。Qt 文档解析错误采用 parse_error 或 discarded；内存分配失败正常传播，不统一包装成 qt_error。错误对象不保存完整输入值，调用方仍需自行决定日志及脱敏策略。

## 10. 接入、兼容及维护

### 10.1 CMake 与头文件

要求 Qt 5.15 Core 与 C++11 或更新标准。通过 CMake 链接 `nlohmann_json::nlohmann_json` 会传播 QtCore 依赖；MSVC 还会传播 `/utf-8`。手工集成头文件时需自行配置 QtCore 的包含和链接参数。

```cmake
find_package(nlohmann_json CONFIG REQUIRED)
target_link_libraries(MyApp PRIVATE nlohmann_json::nlohmann_json)
```

源码作为子项目时可以使用 `add_subdirectory()`。`JSON_MultipleHeaders=ON` 选择多头文件，OFF 选择生成的单头文件；两种形式都依赖 QtCore，并不意味着单头文件可以脱离 Qt 使用。应用部署时还需安排 Qt 运行库。

### 10.2 与原版及其他适配层共存

fork 保留原版命名空间、包含路径和类型名称。同一程序内应统一使用此 fork 的头文件及配置宏，并重新编译相关模块；不能依赖它与原版具备二进制兼容性，也不应混用同名但定义不同的头文件。

Qt 类型已在 fork 内提供 `adl_serializer` 特化，不应再同时引入其他库对相同 Qt 类型的重复特化。领域类型仍在自身命名空间定义 `to_json/from_json`，并使用 `nlohmann::json` 或所选 JSON 类型作为参数；`get_strict<领域类型>()` 不会替代领域适配自身的校验逻辑。

### 10.3 支持范围与源码索引

目前验证基线为 Windows x64、Qt 5.15.2、MSVC v143，覆盖 C++11 Debug、C++20 Release、多头文件及单头文件、安装消费与包含顺序。Qt6、其他平台和编译器、C++ modules、Bazel 实际构建及其他包管理器尚未验证。详细证据见 [验证记录](validation/2026-11-24-qt-core-support.md)。

| 源码 | 职责 |
|---|---|
| [json.hpp](../include/nlohmann/json.hpp) | Qt 文档、对象键、输出、严格读取及 Base64 成员入口 |
| [QtJsonText.hpp](../include/nlohmann/QtJsonText.hpp) | 严格 UTF 编码、错误类别和错误路径辅助 |
| [QtJsonAdapters.hpp](../include/nlohmann/QtJsonAdapters.hpp) | QString/QChar 适配，阻止 QByteArray 隐式值转换 |
| [QtJsonContainers.hpp](../include/nlohmann/QtJsonContainers.hpp) | Qt 序列、集合及映射适配 |
| [QtJsonNumber.hpp](../include/nlohmann/QtJsonNumber.hpp) | 整数范围和浮点精度检查 |
| [QtJsonValues.hpp](../include/nlohmann/QtJsonValues.hpp) | 日期时间、URL、UUID 格式 |
| [QtJsonVariant.hpp](../include/nlohmann/QtJsonVariant.hpp) | QVariant 白名单及投影 |
| [QtJsonQJson.hpp](../include/nlohmann/QtJsonQJson.hpp) | QJson 桥接与精度检查 |
| [QtJsonStrict.hpp](../include/nlohmann/QtJsonStrict.hpp) | 严格目标白名单、容器递归及字段查找 |
| [QtJsonBinary.hpp](../include/nlohmann/QtJsonBinary.hpp) | 规范 Base64 编解码 |
| [unit-qt.cpp](../tests/src/unit-qt.cpp) | Qt 扩展契约测试 |
| [qt_consumer](../tests/qt_consumer/CMakeLists.txt) | 安装消费、包含顺序、多翻译单元及负向编译检查 |

维护时以 `include/nlohmann` 为源码入口，使用仓库 amalgamate 工具生成 `single_include`，不直接修改生成头文件。新增头文件还需同步 Bazel 文件清单；生成和格式化方法见 [上游贡献指南](../.github/CONTRIBUTING.md)。
