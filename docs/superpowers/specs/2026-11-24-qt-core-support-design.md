# nlohmann-json-qt QtCore 扩展设计

状态：用户已批准实施；每完成一项任务提交一次。

## 1. 目标与已确认决策

在 fork 的 `nlohmann::basic_json` 中直接提供 QtCore 使用能力。调用方只需包含 `<nlohmann/json.hpp>`，使用 `nlohmann::json` 完成 Qt 值构造、对象操作、转换和文档输入输出。

- 保留 `nlohmann::` 命名空间、`basic_json`、`json`、`ordered_json`、子节点和迭代器体系。
- 保留上游内部字符串、对象、数组存储及核心算法。
- 普通 `get<T>()`、`get_to()` 和原有标准类型接口保留上游语义，包括数值转换规则。
- 严格转换使用单独命名的接口。
- QtCore 是必需依赖，不增加关闭 Qt 支持的配置开关。
- fork 作为原版的替代品，同一程序统一使用 fork 的头文件并重新编译相关模块。
- 本项目独立开发，不修改 qt-utility 的 LibQtJson，不承担其调用方迁移。

开发基线为本地 `develop` 的 `322063822`，开发分支为 `codex/qt-core-support`。不将尚未检查的远端状态表述为最新状态。

## 2. 实现结构

继续提供 header-only 的 `nlohmann_json::nlohmann_json` INTERFACE target，不引入自建 DLL、运行时注册表或全局可变状态。

在 `include/nlohmann` 中按职责加入 Qt 文本、值、容器和严格转换实现。`basic_json` 仅增加成员接口及必要的分派；复用现有解析器、序列化器、ADL 和节点操作。

Qt 值适配对 `BasicJsonType` 模板化，同一实现用于 `json` 和 `ordered_json`。新增公开 API 沿用上游的 snake_case 风格。实现细节置于 `nlohmann::detail`，不引入另一套调用方命名空间。

以 `include/nlohmann` 为源码入口，使用上游 amalgamate 工具更新 `single_include`，不直接编辑生成文件；新增头文件同步更新 `BUILD.bazel` 的头文件列表。

## 3. 依赖与支持边界

本阶段面向 Qt 5.15.x，实际验证使用 Windows x64、Qt 5.15.2 msvc2019_64 和 MSVC v143。保留上游 C++11 语言下限，不将核心库统一提升为 C++20。Qt 6、其他平台和编译器的支持不在本阶段验收范围内。

CMake 必须查找 Qt5 Core，并通过 INTERFACE 链接要求传播头文件、定义和链接依赖。安装包的 config 同样查找依赖，使安装消费方只需链接原有 target。手动包含头文件的消费方自行提供 QtCore 编译与链接参数。

纯标准类型测试仍链接 QtCore。Qt5 的 `foreach` 等宏与上游标识符的共存需要验证，不能要求所有消费方额外定义 `QT_NO_KEYWORDS` 才能包含头文件。

已启用 C++ modules 的构建不纳入本阶段支持声明；不得宣称尚未验证的 modules、Bazel 或其他包管理器流程已经支持 Qt。Bazel 列表更新只保证生成的文件清单一致。

## 4. 文档输入与输出

拟议的公开接口：

```cpp
using json = nlohmann::json;

json sJson = json::parse(QStringLiteral(R"({"数量":3})"));
json sOther = json::parse(QByteArray("{\"count\":3}"));

QByteArray bytesDocument = sJson.dump_utf8();
QString strDocument = sJson.dump_qstring(2);
```

- 为 `parse` 和 `accept` 提供 QString、QByteArray 输入支持，覆盖 const、非 const 左值和右值，不让通用转发重载绕过 Qt 文本处理。
- 新增输入支持保留原有 callback、allow_exceptions、ignore_comments、ignore_trailing_commas 等参数的含义及默认值。
- QString 文档先严格验证 UTF-16，再转为 UTF-8 交给原解析器。中文、emoji、转义及内嵌 NUL 都按显式长度处理。
- QByteArray 在 `parse` 中始终表示 UTF-8 文档字节，不按 C 字符串长度截断。原始 NUL 不得造成尾部内容被忽略；转义的 `\u0000` 仍是合法字符串内容。
- `accept` 对非法输入返回 false；`parse(..., allow_exceptions=false)` 对非法输入返回 discarded，包括 Qt 文本编码错误。普通异常路径使用原生异常体系。
- `dump_utf8(indent=-1, indent_char=' ', ensure_ascii=false, error_handler=strict)` 返回 QByteArray，`dump_qstring` 使用相同参数返回 QString；原有 `dump()` 的返回类型不变。
- UTF-8 转 QString 必须严格校验；用户选择上游的 replace/ignore 输出策略时遵循该策略，转换后不得静默引入 Qt 替代字符。
- Qt 容器容量不足时报告范围错误，不把长度窄化为 int 后分配。内存分配异常正常传播。

本阶段不改变原有 std::string 文档解析的重复键、深度、输入大小或浮点解析策略，不默认复制 LibQtJson 的文档限额及重复键拒绝规则。新增 Qt 文档入口也不额外建立另一套文档策略，完整范围读取及编码校验除外。

## 5. QString 对象键与路径

为以下对象键入口增加 QString 支持：

```cpp
sJson[QStringLiteral("名称")] = QStringLiteral("手机壳");
QString strName = sJson.at(QStringLiteral("名称")).get<QString>();
bool bExists = sJson.contains(QStringLiteral("名称"));
auto it = sJson.find(QStringLiteral("名称"));
QString strFallback = sJson.value(QStringLiteral("备注"), QStringLiteral("无"));
sJson.count(QStringLiteral("名称"));
sJson.erase(QStringLiteral("名称"));
```

常量与非常量重载的返回类型及节点语义保持上游约定。键严格转为 UTF-8，保留内嵌 NUL；无效 UTF-16 报错。

JSON Pointer 可直接从 QString 构造；语法保持 RFC 6901。迭代器的现有 `key()` 继续返回原内部字符串类型，新增 `key_qstring()` 用于 Qt 文本输出；`items()` 的迭代代理同样提供 `key_qstring()`。

对象初始化列表中的 QString 键应正确识别为对象键；字符串常量、标准字符串、数组下标和 JSON Pointer 的原有重载不得发生歧义。

## 6. 普通 Qt 值适配

以下类型通过构造、赋值、`get<T>()` 和 `get_to()` 使用。日期等业务值格式为本 fork 的明确约定，不声称与所有外部系统具有通用互操作性。

| Qt 类型 | JSON 表示与约束 |
|---|---|
| QString | UTF-8 string；严格验证双向编码，保留内嵌 NUL；null QString 归一化为空字符串 |
| QChar | 单个非代理 BMP 字符的 string；emoji 使用 QString |
| QStringList、QList<T>、QVector<T> | array，保持顺序 |
| QSet<T> | array，反向按 Qt 相等/哈希语义去重；输出顺序不保证 |
| QMap<QString,T>、QHash<QString,T> | object，键按严格 UTF-8 转换；不自动字符串化其他键类型 |
| QDate | `yyyy-MM-dd`，年份 0001～9999，输入必须规范且有效 |
| QTime | `HH:mm:ss.zzz`，输入必须规范且有效 |
| QDateTime | object，保存并交叉校验日期、时间、epochMilliseconds、timeSpec、offsetSeconds，LocalTime/TimeZone 保存 timeZoneId；详见下文 |
| QUrl | FullyEncoded URI string，StrictMode 校验并规范回验；允许有效相对 URL，拒绝空 URL |
| QUuid | 小写无花括号的 36 字符 string；输入允许十六进制大小写，零 UUID 有效 |
| QVariant | 按下节明确白名单投影为 JSON 值，不自动保存原元类型 |
| QJsonValue、QJsonObject、QJsonArray、QJsonDocument | 递归 JSON 桥接；受 Qt5 double 数值模型限制 |

Qt 容器的普通读取逐元素使用普通 `get<T>()`。例如 `[1.5]` 读取为 `QList<int>` 时，元素的行为与直接 `get<int>()` 一致。转换先生成临时对象，再提交 `get_to` 的输出，避免失败时交付部分 Qt 容器结果。

不接管领域类型的 ADL 规则；领域类型仍在自身命名空间定义 `to_json/from_json`。

### QDateTime 对象格式

固定字段为 `date`、`time`、`epochMilliseconds`、`timeSpec`、`offsetSeconds`。`timeSpec` 为 `UTC`、`OffsetFromUTC`、`LocalTime` 或 `TimeZone`。LocalTime 和 TimeZone 另含 `timeZoneId`；使用 Qt 可恢复的时区 ID。

反向构造后同时校验 epoch、墙上日期时间、偏移和 timeSpec。时区不可用、LocalTime 与系统时区不一致、字段矛盾或无效日期时间均报错，不降级为 UTC 或系统默认值。只保证 Qt5 毫秒精度，不保证恢复任意自定义时区对象的额外元数据。

### QVariant 与 QJson 的具体边界

QVariant 内置白名单包括：无效 QVariant、bool、标准有符号/无符号整型元类型、float、double、QString、QChar、QStringList、QVariantList、QVariantMap、QVariantHash、上表日期/URL/UUID 类型，以及 QJson 类型。有效但不在白名单中的类型报错；不使用 `canConvert` 或 `toString` 兜底，不自动支持任意模板容器、用户元类型或指针。

JSON null 反向成为无效 QVariant；boolean、signed integer、unsigned integer、float、string、array、object 分别反向成为 bool、qlonglong、qulonglong、double、QString、QVariantList、QVariantMap。日期、URL、UUID 等字符串不猜测为原类型，QDateTime object 不猜测为日期时间。非有限浮点值报错。

QJson 桥接拒绝 Undefined、null document 和无法由 double 精确表示的整数；QJsonDocument 根仅接受 object/array。进入 QJson 前已经丢失的整数精度无法追溯诊断。

## 7. QByteArray 的两种语义

文档输入使用 `json::parse(QByteArray)`；业务二进制使用显式具名入口：

```cpp
json sBinary = json::encode_base64(bytesPayload);
QByteArray bytesBack = sBinary.decode_base64();
```

`encode_base64` 的参数是原始业务字节，输出为标准 Base64 JSON string；`decode_base64` 解码当前 JSON string。

裸 QByteArray 的值构造、赋值及 `get<QByteArray>()` 不提供隐式转换，防止被通用容器适配误识别成数字数组。QVariant 中的 QByteArray 同样拒绝。上游 `json::binary()` 保留原有含义，不把它自动等同于 Base64 string。

Base64 解码严格拒绝空白、非标准字母表、错误填充及非规范尾部位；空字节序列合法。

## 8. 严格接口与错误

新增接口：

```cpp
int nCount = sJson.at("count").get_strict<int>();
sJson.at("items").get_to_strict(lstItems);
quint64 nId = sJson.read_required<quint64>(QStringLiteral("/items/0/id"));
QString strNote = sJson.value_strict<QString>(QStringLiteral("note"), QStringLiteral("无"));
```

- `get_strict<T>()` 对整型目标拒绝 float（包括 1.0），检查符号和范围；浮点目标拒绝非有限值、范围错误和精度损失。
- 严格接口递归覆盖上表 Qt 容器。普通标准库容器、领域 ADL 类型及自定义序列化器不宣称自动获得递归严格检查；不支持严格递归的目标在编译期给出明确诊断，不能静默回退为普通 get。
- Qt 文本、日期、URL 等已有的值有效性检查在普通和严格接口中保持一致。
- `get_to_strict` 仅对可无异常交换的目标开放；转换成功后提交，失败保留旧值。
- `read_required<T>(QString pointer)` 按 RFC 6901 查找并严格读取，区分缺失、null、类型和范围错误。null 仅当目标自身能够表示 JSON null 时有效。
- `value_strict<T>(QString key, T default_value)` 仅对缺失键使用默认值，存在但为 null 或非法值时报错；该接口读取直接对象键，不解释为 JSON Pointer。

Qt 扩展的转换错误使用 `nlohmann::qt_error`，从上游 `detail::exception` 继承，提供错误类别与 QString JSON Pointer 路径，使 `catch (const nlohmann::json::exception&)` 能捕获。字段和容器层补充路径；不保存完整输入值。原有标准接口的异常类型与 ID 保持不变，Qt 文档解析继续遵循 parse_error/discarded 规则。

严格数值错误、编码、无效业务值、字段缺失、null、未支持的 QVariant、Base64 和 QJson 精度错误均有可区分的类别。内存分配失败正常传播。自定义 ADL 的任意外部副作用不提供回滚保证。

本阶段不新增 result 包装体系，也不移植 LibQtJson 的注册表、options 或全局资源策略。

## 9. 验证计划与通过条件

测试使用现有 doctest/CTest 机制，并保留首次失败、修正及最终结果。日志按构建目录和配置区分，不读取旧结果作为本轮证据。

1. 变更前构建并运行受影响上游专项作为基线：构造、转换、对象访问、解析、序列化、迭代、JSON Pointer 和 ordered_json。
2. 新增 Qt 专项覆盖：所有上表类型、嵌套容器、QString 对象初始化键、Qt 左值/右值输入、Unicode/emoji/NUL/非法编码、空值、负向输入及转换失败保留旧值。
3. 数值区分普通与严格路径：1.5 转 int、负值转无符号、整型上下界、2^53+1、2^53+2、double 转 float、NaN/Infinity；容器内使用同样的边界。
4. 检查 QVariant 白名单与无元类型恢复规则、QJson 精度、日期时间/时区恢复、Base64 非规范输入、错误类别和嵌套路径。
5. Qt 专项同时实例化 json 和 ordered_json，保留原有字符串/下标重载，覆盖 const 与非常量对象。
6. 多头文件版使用 C++11 Debug 与 C++20 Release 运行 Qt 专项及必要上游回归；生成的单头文件版使用 C++20 Release 运行同一 Qt 专项与受影响上游专项。
7. 检查 Qt 在前/JSON 在前、json_fwd 在前、多翻译单元的包含与链接；默认 Qt 宏设置下能够使用。
8. 隔离安装后，通过 `find_package(nlohmann_json CONFIG REQUIRED)` 编译并运行消费程序，确认 QtCore 依赖自动传播；多头文件和单头文件安装分别验证。
9. 单头文件重新生成后再次生成应无差异，BUILD.bazel 的头文件清单与源码一致；检查最终 diff 没有无关格式化修改。

通过条件：上述适用专项全部通过；普通标准接口回归与基线一致；生成文件同步；消费方只链接原 target 即可使用 Qt；未验证的平台和工具链明确列为限制。

## 10. 交付边界

公开使用指南描述当前接口、格式与限制；任务记录单独保存构建配置、日志、结果和未执行事项。保留上游 MIT 版权与许可，新增 Qt 能力的说明不伪装成上游官方发布。

未经用户明确要求不暂存、提交、推送或创建 PR。分支内的实现、测试、生成文件和文档作为可审阅工作区改动交付。
