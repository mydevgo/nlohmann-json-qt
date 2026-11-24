/**
 * @file unit-qt.cpp
 * @brief Qt 文本、键和文档接口的契约测试。
 * @author 黄凯
 * @date 2026-11-24
 * SPDX-License-Identifier: MIT
 */
#include "doctest_compatibility.h"
#include <QString>
#include <nlohmann/json.hpp>

/** @brief 对 json 和 ordered_json 使用相同的 Qt 契约。 */
template<typename J>
void CheckQtText()
{
    // 准备：包含代理对与内嵌 NUL 的文本。
    QString strText = QString::fromUtf8("\xe4\xb8\xad\xe6\x96\x87\xf0\x9f\x98\x80");
    strText.append(QChar(0));
    strText.append(QLatin1Char('x'));
    const QString strKey = QString::fromUtf8("\xe5\x90\x8d\xe7\xa7\xb0");

    // 执行：Qt 值、键、初始化列表、文档输入输出。
    J sJson{{strKey, strText}};
    sJson[strKey] = strText;
    const J& rConst = sJson;
    const QByteArray bytes = sJson.dump_utf8();
    const QString strDocument = sJson.dump_qstring(2);
    QString strMutable = strDocument;
    QByteArray bytesMutable = bytes;

    // 断言：长度、编码、重载与标准入口保持一致。
    CHECK(sJson.at(strKey).template get<QString>() == strText);
    CHECK(rConst[strKey].template get<QString>() == strText);
    CHECK(rConst.at(strKey).template get<QString>() == strText);
    CHECK(rConst.contains(strKey));
    CHECK(rConst.count(strKey) == 1);
    CHECK(rConst.find(strKey).key_qstring() == strKey);
    CHECK(sJson.find(strKey).key_qstring() == strKey);
    CHECK(sJson.items().begin().key_qstring() == strKey);
    CHECK(sJson.value(strKey, QString()) == strText);
    CHECK(sJson.value(QStringLiteral("missing"), QStringLiteral("fallback")) == QStringLiteral("fallback"));
    CHECK(J::parse(bytes) == sJson);
    CHECK(J::parse(bytesMutable) == sJson);
    CHECK(J::parse(QByteArray(bytes)) == sJson);
    CHECK(J::parse(strDocument) == sJson);
    CHECK(J::parse(strMutable) == sJson);
    CHECK(J::parse(QString(strDocument)) == sJson);
    CHECK(J::accept(bytes));
    CHECK(J::accept(bytesMutable));
    CHECK(J::accept(QByteArray(bytes)));
    CHECK(J::accept(strDocument));
    CHECK(J::accept(strMutable));
    CHECK(J::accept(QString(strDocument)));
    const typename J::json_pointer sPointer(QStringLiteral("/") + strKey);
    CHECK(sJson.at(sPointer).template get<QString>() == strText);
    CHECK(sJson.erase(strKey) == 1);
    CHECK_FALSE(sJson.contains(strKey));
    CHECK(J(1.5).template get<int>() == 1);
    CHECK(J::array({1, 2}).at(0) == 1);
}

TEST_CASE("Qt text and keys")
{
    CheckQtText<nlohmann::json>();
    CheckQtText<nlohmann::ordered_json>();
}

TEST_CASE("Qt invalid document encoding and explicit byte length")
{
    // 准备：孤立代理项、截断 UTF-8、原始 NUL 后附尾部内容。
    typedef nlohmann::json J;
    const QString strInvalid(QChar(0xd800));
    const QByteArray bytesInvalid("\"\xc3\"", 3);
    const QByteArray bytesTail("{}\0garbage", 10);

    // 执行与断言：异常开关不得被 Qt 转换绕过。
    CHECK_THROWS_AS(J::parse(strInvalid), J::parse_error);
    CHECK(J::parse(strInvalid, nullptr, false).is_discarded());
    CHECK_FALSE(J::accept(strInvalid));
    CHECK(J::parse(bytesInvalid, nullptr, false).is_discarded());
    CHECK_FALSE(J::accept(bytesInvalid));
    CHECK(J::parse(bytesTail, nullptr, false).is_discarded());
    CHECK_FALSE(J::accept(bytesTail));
    CHECK_THROWS_AS(J(strInvalid), nlohmann::qt_error);
    CHECK(J::parse(QByteArray("\"\\u0000\"")).get<QString>().size() == 1);
    CHECK(J::parse(QStringLiteral("/*x*/[1,]"), nullptr, true, true, true).is_array());
    CHECK_FALSE((std::is_constructible<J, QByteArray>::value));
}

/** @brief Qt 容器普通转换与失败时输出保护。 */
template<typename J>
void CheckQtContainers()
{
    // 准备。
    const QMap<QString, QList<QString>> mapValues{{QStringLiteral("a/b~"), {QStringLiteral("x"), QStringLiteral("y")}}};
    const QSet<int> setValues{1, 2};
    const QVector<int> vecValues{3, 4};
    const QHash<QString, int> mapHash{{QStringLiteral("k"), 7}};
    QList<QString> lstOutput{QStringLiteral("old")};
    // 执行与断言。
    CHECK((J(mapValues).template get<QMap<QString, QList<QString>>>() == mapValues));
    CHECK(J(setValues).template get<QSet<int>>() == setValues);
    CHECK(J(vecValues).template get<QVector<int>>() == vecValues);
    CHECK((J(mapHash).template get<QHash<QString, int>>() == mapHash));
    CHECK(J(QStringList{QStringLiteral("x")}).template get<QStringList>().size() == 1);
    CHECK(J::array({1.5}).template get<QList<int>>().front() == 1);
    CHECK_THROWS_AS(J::array({"ok", 5}).get_to(lstOutput), nlohmann::qt_error);
    CHECK(lstOutput == QList<QString> {QStringLiteral("old")});
    CHECK_FALSE((std::is_constructible<J, QMap<int, int>>::value));
}

/** @brief 日期、URI、UUID 与时区对象往返。 */
template<typename J>
void CheckQtValues()
{
    // 准备。
    const QDate sDate(2024, 2, 29);
    const QTime sTime(12, 34, 56, 789);
    const QUrl sUrl(QStringLiteral("https://example.com/%E4%B8%AD?q=1"), QUrl::StrictMode);
    const QUuid sUuid(QStringLiteral("550e8400-e29b-41d4-a716-446655440000"));
    // 执行与断言。
    CHECK(J(sDate).template get<QDate>() == sDate);
    CHECK(J(sTime).template get<QTime>() == sTime);
    CHECK(J(sUrl).template get<QUrl>() == sUrl);
    CHECK(J(sUuid).template get<QUuid>() == sUuid);
    CHECK(J(QUuid()).template get<QUuid>().isNull());
    CHECK_THROWS_AS(J(QDate()), nlohmann::qt_error);
    CHECK_THROWS_AS(J("2023-02-29").template get<QDate>(), nlohmann::qt_error);
    CHECK_THROWS_AS(J("25:00:00.000").template get<QTime>(), nlohmann::qt_error);
    CHECK_THROWS_AS(J("invalid").template get<QUuid>(), nlohmann::qt_error);
    CHECK_THROWS_AS(J(QUrl()), nlohmann::qt_error);
    const QDateTime arrTimes[] =
    {
        QDateTime(sDate, sTime, Qt::UTC), QDateTime(sDate, sTime, Qt::OffsetFromUTC, 19800),
        QDateTime(sDate, sTime, Qt::LocalTime), QDateTime(sDate, sTime, QTimeZone("UTC"))
    };
    for (const QDateTime& sValue : arrTimes)
    {
        const QDateTime sBack = J(sValue).template get<QDateTime>();
        CHECK(sBack == sValue);
        CHECK(sBack.timeSpec() == sValue.timeSpec());
    }
    J sInvalid = arrTimes[0];
    sInvalid["offsetSeconds"] = 1;
    CHECK_THROWS_AS(sInvalid.template get<QDateTime>(), nlohmann::qt_error);
}

/** @brief QVariant 白名单、QJson 精度和显式二进制。 */
template<typename J>
void CheckQtBridges()
{
    // 准备。
    const QVariantMap mapValues{{QStringLiteral("items"), QVariantList{true, 7, QStringLiteral("x")}}};
    const QByteArray bytes("\0\xff\x01", 3);
    const QJsonObject sQJson{{QStringLiteral("a"), QJsonArray{1, QStringLiteral("x")}}};
    // 执行与断言。
    CHECK(J(QVariant(mapValues)).template get<QVariant>().toMap().value(QStringLiteral("items")).toList().size() == 3);
    CHECK_FALSE(J(nullptr).template get<QVariant>().isValid());
    CHECK(J(QVariant(QDate(2024, 1, 2))).template get<QVariant>().userType() == QMetaType::QString);
    CHECK_THROWS_AS(J(QVariant(bytes)), nlohmann::qt_error);
    CHECK_THROWS_AS(J(QVariant::fromValue(QList<int> {1, 2})), nlohmann::qt_error);
    CHECK_THROWS_AS(J(QVariant(std::numeric_limits<double>::infinity())), nlohmann::qt_error);
    CHECK(J(sQJson).template get<QJsonObject>() == sQJson);
    CHECK(J(QJsonDocument(sQJson)).template get<QJsonDocument>().object() == sQJson);
    CHECK_THROWS_AS(J(QJsonValue(QJsonValue::Undefined)), nlohmann::qt_error);
    CHECK_THROWS_AS(J(QJsonDocument()), nlohmann::qt_error);
    CHECK_THROWS_AS(J(std::uint64_t(9007199254740993ULL)).template get<QJsonValue>(), nlohmann::qt_error);
    CHECK(J(std::uint64_t(9007199254740994ULL)).template get<QJsonValue>().isDouble());
    CHECK(J::encode_base64(bytes).decode_base64() == bytes);
    CHECK(J::encode_base64(QByteArray()).decode_base64().isEmpty());
    CHECK_THROWS_AS(J("Zh==").decode_base64(), nlohmann::qt_error);
    CHECK_THROWS_AS(J("Zg").decode_base64(), nlohmann::qt_error);
    CHECK_THROWS_AS(J("Zg==\n").decode_base64(), nlohmann::qt_error);
}

TEST_CASE("Qt containers values and bridges")
{
    CheckQtContainers<nlohmann::json>();
    CheckQtContainers<nlohmann::ordered_json>();
    CheckQtValues<nlohmann::json>();
    CheckQtValues<nlohmann::ordered_json>();
    CheckQtBridges<nlohmann::json>();
    CheckQtBridges<nlohmann::ordered_json>();
}

TEST_CASE("Qt scalar and metadata boundaries")
{
    // 准备：白名单的各类整数和复合值。
    typedef nlohmann::json J;
    const QVariant arrValues[] =
    {
        QVariant(), QVariant(true), QVariant::fromValue(char(1)), QVariant::fromValue(signed char(2)),
        QVariant::fromValue(static_cast<unsigned char>(3)), QVariant::fromValue(short(4)),
        QVariant::fromValue(static_cast<unsigned short>(5)), QVariant(6), QVariant(7u),
        QVariant::fromValue(long(8)), QVariant::fromValue(9ul), QVariant::fromValue(qlonglong(-10)),
        QVariant::fromValue(qulonglong(11)), QVariant::fromValue(1.5f), QVariant(2.5),
        QVariant(QChar('x')), QVariant(QStringLiteral("text")), QVariant(QStringList{QStringLiteral("a")}),
        QVariant(QVariantList{1, 2}), QVariant(QVariantMap{{QStringLiteral("a"), 1}}),
        QVariant(QVariantHash{{QStringLiteral("b"), true}}), QVariant(QDate(2024, 1, 2)),
        QVariant(QTime(1, 2, 3, 4)), QVariant(QDateTime(QDate(2024, 1, 2), QTime(1, 2), Qt::UTC)),
        QVariant(QUrl(QStringLiteral("relative/path"))), QVariant(QUuid()),
        QVariant::fromValue(QJsonValue(3)), QVariant::fromValue(QJsonArray{1}),
        QVariant::fromValue(QJsonObject{{QStringLiteral("x"), 1}}),
        QVariant::fromValue(QJsonDocument(QJsonArray{1}))
    };
    // 执行与断言：投影内容稳定，不要求恢复原元类型。
    for (const QVariant& sValue : arrValues)
    {
        const J sJson = sValue;
        CHECK(J(sJson.get<QVariant>()) == sJson);
    }
    CHECK(J(QChar('x')).get<QChar>() == QChar('x'));
    CHECK_THROWS_AS(J(QChar(0xd800)), nlohmann::qt_error);
    CHECK_THROWS_AS(J("ab").get<QChar>(), nlohmann::qt_error);
    CHECK(J(QString()).get<QString>().isEmpty());
    CHECK(J(std::string("\xef\xbb\xbf", 3)).get<QString>() == QString(QChar(0xfeff)));
    CHECK_THROWS_AS(J(std::string("\xed\xa0\x80", 3)).get<QString>(), nlohmann::qt_error);
    CHECK_THROWS_AS(J(std::string("\xf0\x9f", 2)).get<QString>(), nlohmann::qt_error);
    CHECK_THROWS_AS(J::array({1}).get<QJsonObject>(), nlohmann::qt_error);
    CHECK_THROWS_AS(J(1).get<QJsonDocument>(), nlohmann::qt_error);
    CHECK(J("550E8400-E29B-41D4-A716-446655440000").get<QUuid>() == QUuid(QStringLiteral("550e8400-e29b-41d4-a716-446655440000")));
    J sBadTime = QDateTime(QDate(2024, 1, 2), QTime(1, 2), QTimeZone("UTC"));
    sBadTime["timeZoneId"] = "Invalid/TimeZone";
    CHECK_THROWS_AS(sBadTime.get<QDateTime>(), nlohmann::qt_error);
}

/** @brief 普通与严格数值路径的契约不同，严格读取精确保留边界。 */
template<typename J>
void CheckQtStrictNumbers()
{
    // 准备：每个边界均使用独立的 JSON 数值。
    // 执行与断言。
    CHECK(J(1.5).template get<int>() == 1);
    CHECK_THROWS_AS(J(1.5).template get_strict<int>(), nlohmann::qt_error);
    CHECK_THROWS_AS(J(1.0).template get_strict<int>(), nlohmann::qt_error);
    CHECK_THROWS_AS(J(-1).template get_strict<unsigned>(), nlohmann::qt_error);
    CHECK_THROWS_AS(J(256).template get_strict<unsigned char>(), nlohmann::qt_error);
    CHECK_THROWS_AS(J(128).template get_strict<signed char>(), nlohmann::qt_error);
    CHECK_THROWS_AS(J(-129).template get_strict<signed char>(), nlohmann::qt_error);
    CHECK(J(255).template get_strict<unsigned char>() == 255);
    CHECK(J(-128).template get_strict<signed char>() == -128);
    CHECK(J((std::numeric_limits<qint64>::min)()).template get_strict<qint64>() == (std::numeric_limits<qint64>::min)());
    CHECK(J((std::numeric_limits<quint64>::max)()).template get_strict<quint64>() == (std::numeric_limits<quint64>::max)());
    CHECK_THROWS_AS(J((std::numeric_limits<quint64>::max)()).template get_strict<qint64>(), nlohmann::qt_error);
    CHECK_THROWS_AS(J(std::uint64_t(9007199254740993ULL)).template get_strict<double>(), nlohmann::qt_error);
    CHECK(J(std::uint64_t(9007199254740994ULL)).template get_strict<double>() == 9007199254740994.0);
    CHECK(J(std::uint64_t(1) << 63).template get_strict<double>() == 9223372036854775808.0);
    CHECK_THROWS_AS(J(0.1).template get_strict<float>(), nlohmann::qt_error);
    CHECK(J(0.5).template get_strict<float>() == 0.5f);
    CHECK_THROWS_AS(J(std::numeric_limits<double>::infinity()).template get_strict<double>(), nlohmann::qt_error);
    CHECK_THROWS_AS(J(std::numeric_limits<double>::quiet_NaN()).template get_strict<double>(), nlohmann::qt_error);
    CHECK_THROWS_AS(J((std::numeric_limits<double>::max)()).template get_strict<float>(), nlohmann::qt_error);
}

/** @brief 严格容器和字段读取补充路径，失败保留旧值。 */
template<typename J>
void CheckQtStrictFields()
{
    // 准备。
    const J sJson{{"items", J::array({J{{"a/b~", J::array({1, 1.5})}}})}, {"null", nullptr}};
    QList<int> lstOutput{99};
    // 执行与断言。
    CHECK_THROWS_AS(J::array({1, 1.5}).get_to_strict(lstOutput), nlohmann::qt_error);
    CHECK(lstOutput == QList<int> {99});
    J::array({1, 2}).get_to_strict(lstOutput);
    CHECK(lstOutput == QList<int> {1, 2});
    CHECK(sJson.template value_strict<int>(QStringLiteral("missing"), 7) == 7);
    CHECK_THROWS_AS(sJson.template value_strict<int>(QStringLiteral("null"), 7), nlohmann::qt_error);
    CHECK_THROWS_AS(sJson.template read_required<int>(QStringLiteral("/missing")), nlohmann::qt_error);
    CHECK(sJson.template read_required<std::nullptr_t>(QStringLiteral("/null")) == nullptr);
    CHECK_FALSE(sJson.template read_required<QVariant>(QStringLiteral("/null")).isValid());
    CHECK_THROWS_AS(sJson.template read_required<int>(QStringLiteral("/items/00")), nlohmann::qt_error);
    CHECK_THROWS_AS(sJson.template read_required<int>(QStringLiteral("/items/-")), nlohmann::qt_error);
    CHECK_THROWS_AS(J(1).template get_strict<bool>(), nlohmann::qt_error);
    CHECK(J(true).template get_strict<bool>());
    const J sMap{{"a/b~", J::array({1, 2})}};
    CHECK((sMap.template get_strict<QMap<QString, QVector<int>>>().value(QStringLiteral("a/b~")) == QVector<int> {1, 2}));
    try
    {
        sJson.template read_required<QMap<QString, QList<int>>>(QStringLiteral("/items/0"));
        FAIL("strict read must fail");
    }
    catch (const nlohmann::qt_error& sError)
    {
        CHECK(sError.category == nlohmann::qt_error_category::type);
        CHECK(sError.path == QStringLiteral("/items/0/a~1b~0/1"));
    }
    catch (...)
    {
        FAIL("wrong exception type");
    }
    CHECK_THROWS_AS(J("bad").template get_strict<int>(), typename J::exception);
}

TEST_CASE("Qt strict conversions and fields")
{
    CheckQtStrictNumbers<nlohmann::json>();
    CheckQtStrictNumbers<nlohmann::ordered_json>();
    CheckQtStrictFields<nlohmann::json>();
    CheckQtStrictFields<nlohmann::ordered_json>();
}
