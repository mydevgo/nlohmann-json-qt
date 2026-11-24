/**
 * @file QtJsonText.hpp
 * @brief Qt 文本的严格编码转换及扩展错误。
 * @author 黄凯
 * @date 2026-11-24
 * SPDX-License-Identifier: MIT
 */
#pragma once

#include <nlohmann/detail/exceptions.hpp>
#include <QByteArray>
#include <QString>
#include <QTextCodec>
#include <limits>
#include <string>

NLOHMANN_JSON_NAMESPACE_BEGIN

/// Qt 扩展错误类别，不改变上游异常 ID。
enum class qt_error_category
{
    encoding,          ///< 非法 UTF 编码
    type,              ///< 节点类型错误
    range,             ///< 数值或容器容量越界
    precision,         ///< 数值不能精确表示
    value,             ///< 业务值无效
    missing,           ///< 字段缺失
    null_value,        ///< 不允许 null
    unsupported,       ///< QVariant 或目标类型不受支持
    base64             ///< Base64 编码不规范
};

/// 可由 json::exception 捕获的 Qt 转换错误；消息不包含完整输入值。
class qt_error : public detail::exception
{
  public:
    const qt_error_category category; ///< 稳定错误类别
    const QString path;              ///< RFC 6901 路径

    /** @brief 构造脱敏错误，保留类别与路径。 */
    explicit qt_error(qt_error_category eCategory, const QString& strPath = QString())
        : detail::exception(600 + static_cast<int>(eCategory), "[json.exception.qt_error] Qt conversion failed"),
          category(eCategory), path(strPath)
    {}
};

namespace detail
{
/** @brief 校验 UTF-16；不替换孤立代理项。 */
inline bool QtValidUtf16(const QString& strText)
{
    for (int i = 0; i < strText.size(); ++i)
    {
        const QChar sChar = strText.at(i);
        if (sChar.isHighSurrogate())
        {
            if (++i >= strText.size() || !strText.at(i).isLowSurrogate())
            {
                return false;
            }
        }
        else if (sChar.isLowSurrogate())
        {
            return false;
        }
    }
    return true;
}

/** @brief 转为 UTF-8；编码错误抛出 qt_error。 */
inline QByteArray QtEncode(const QString& strText)
{
    if (!QtValidUtf16(strText))
    {
        JSON_THROW(qt_error(qt_error_category::encoding));
    }
    // 在 Qt 的 int 容量边界前计算实际 UTF-8 长度。
    quint64 nBytes = 0;
    for (int i = 0; i < strText.size(); ++i)
    {
        const ushort nCode = strText.at(i).unicode();
        if (QChar::isHighSurrogate(nCode))
        {
            nBytes += 4;
            ++i;
        }
        else
        {
            nBytes += nCode < 0x80 ? 1 : nCode < 0x800 ? 2 : 3;
        }
    }
    if (nBytes >= static_cast<quint64>((std::numeric_limits<int>::max)()))
    {
        JSON_THROW(qt_error(qt_error_category::range));
    }
    return strText.toUtf8();
}

/** @brief 按显式长度严格解码 UTF-8，保留 BOM 和内嵌 NUL。 */
template<typename String>
QString QtDecode(const String& strText)
{
    if (strText.size() >= static_cast<std::size_t>((std::numeric_limits<int>::max)()))
    {
        JSON_THROW(qt_error(qt_error_category::range));
    }
    QTextCodec::ConverterState sState(QTextCodec::IgnoreHeader);
    const QString strResult = QTextCodec::codecForName("UTF-8")->toUnicode(
                                  strText.data(), static_cast<int>(strText.size()), &sState);
    if (sState.invalidChars != 0 || sState.remainingChars != 0)
    {
        JSON_THROW(qt_error(qt_error_category::encoding));
    }
    return strResult;
}

/** @brief 转为内部字符串类型，保留字节长度。 */
template<typename String>
String QtKey(const QString& strText)
{
    const QByteArray bytes = QtEncode(strText);
    return String(bytes.constData(), static_cast<std::size_t>(bytes.size()));
}

/** @brief 转换异常加上父节点路径。 */
inline qt_error QtPrepend(const qt_error& sError, const QString& strPrefix)
{
    return qt_error(sError.category, strPrefix + sError.path);
}

/** @brief 对象键转换为 RFC 6901 路径片段。 */
inline QString QtToken(QString strKey)
{
    strKey.replace(QStringLiteral("~"), QStringLiteral("~0"));
    strKey.replace(QStringLiteral("/"), QStringLiteral("~1"));
    return QStringLiteral("/") + strKey;
}
}
NLOHMANN_JSON_NAMESPACE_END
