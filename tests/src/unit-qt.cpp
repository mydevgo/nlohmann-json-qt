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
