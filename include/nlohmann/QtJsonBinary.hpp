/**
 * @file QtJsonBinary.hpp
 * @brief 规范 Base64 编解码，业务字节与文档字节显式区分。
 * @author 黄凯
 * @date 2026-11-24
 * SPDX-License-Identifier: MIT
 */
#pragma once
#include <nlohmann/QtJsonText.hpp>

NLOHMANN_JSON_NAMESPACE_BEGIN
namespace detail
{
/** @brief 编码前计算容量上限，避免 Qt5 int 溢出。 */
inline QByteArray QtBase64Encode(const QByteArray& bytes)
{
    if (4 * ((static_cast<quint64>(bytes.size()) + 2) / 3) >= static_cast<quint64>((std::numeric_limits<int>::max)()))
    {
        JSON_THROW(qt_error(qt_error_category::range));
    }
    return bytes.toBase64();
}

/** @brief 解码后规范回验，拒绝空白、错误填充、非标准字母及非零尾部位。 */
inline QByteArray QtBase64Decode(const QString& strText)
{
    const QByteArray bytesEncoded = QtEncode(strText);
    const QByteArray::FromBase64Result sResult = QByteArray::fromBase64Encoding(bytesEncoded, QByteArray::AbortOnBase64DecodingErrors);
    if (!sResult || sResult.decoded.toBase64() != bytesEncoded)
    {
        JSON_THROW(qt_error(qt_error_category::base64));
    }
    return sResult.decoded;
}
}
NLOHMANN_JSON_NAMESPACE_END
