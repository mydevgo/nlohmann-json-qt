/**
 * @file QtJsonValues.hpp
 * @brief Qt 日期、时间、URL 和 UUID 值格式。
 * @author 黄凯
 * @date 2026-11-24
 * SPDX-License-Identifier: MIT
 */
#pragma once
#include <nlohmann/QtJsonContainers.hpp>
#include <nlohmann/QtJsonNumber.hpp>
#include <QDateTime>
#include <QTimeZone>
#include <QUrl>
#include <QUuid>

NLOHMANN_JSON_NAMESPACE_BEGIN
namespace detail
{
/** @brief 业务值无效时报告脱敏转换错误。 */
inline void QtRequireValue(bool bValid)
{
    if (!bValid)
    {
        JSON_THROW(qt_error(qt_error_category::value));
    }
}

/** @brief 日期时间格式字段必须存在，错误携带字段路径。 */
template<typename J>
const J& QtValueField(const J& sJson, const char* szName)
{
    const auto it = sJson.find(szName);
    if (it == sJson.end())
    {
        JSON_THROW(qt_error(qt_error_category::missing, QtToken(QString::fromLatin1(szName))));
    }
    return it.value();
}

/** @brief 元数据整数严格检查后加上字段路径。 */
template<typename T, typename J>
T QtValueRead(const J& sJson, const char* szName, std::true_type)
{
    const J& sField = QtValueField(sJson, szName);
    JSON_TRY { return QtInteger<T>(sField); }
    JSON_CATCH(const qt_error & sError)
    {
        JSON_THROW(QtPrepend(sError, QtToken(QString::fromLatin1(szName))));
    }
}

/** @brief 非数值格式字段复用普通有效性检查。 */
template<typename T, typename J>
T QtValueRead(const J& sJson, const char* szName, std::false_type)
{
    return QtReadAt<T>(QtValueField(sJson, szName), QtToken(QString::fromLatin1(szName)));
}

/** @brief 日期时间协议字段的统一读取。 */
template<typename T, typename J>
T QtValueRead(const J& sJson, const char* szName)
{
    return QtValueRead<T>(sJson, szName, std::is_integral<T>());
}
}

template<> struct adl_serializer<QDate>
{
    /** @brief 写入规范 ISO 日期，拒绝无效值和非四位年份。 */
    template<typename J> static void to_json(J& rJson, const QDate& sDate)
    {
        detail::QtRequireValue(sDate.isValid() && sDate.year() >= 1 && sDate.year() <= 9999);
        rJson = sDate.toString(QStringLiteral("yyyy-MM-dd"));
    }
    /** @brief 严格解析并回验日期文本。 */
    template<typename J> static void from_json(const J& sJson, QDate& rDate)
    {
        const QString strText = sJson.template get<QString>();
        const QDate sDate = QDate::fromString(strText, QStringLiteral("yyyy-MM-dd"));
        detail::QtRequireValue(sDate.isValid() && sDate.year() >= 1 && sDate.year() <= 9999 && sDate.toString(QStringLiteral("yyyy-MM-dd")) == strText);
        rDate = sDate;
    }
};

template<> struct adl_serializer<QTime>
{
    /** @brief 写入毫秒时间。 */
    template<typename J> static void to_json(J& rJson, const QTime& sTime)
    {
        detail::QtRequireValue(sTime.isValid());
        rJson = sTime.toString(QStringLiteral("HH:mm:ss.zzz"));
    }
    /** @brief 严格解析规范毫秒时间。 */
    template<typename J> static void from_json(const J& sJson, QTime& rTime)
    {
        const QString strText = sJson.template get<QString>();
        const QTime sTime = QTime::fromString(strText, QStringLiteral("HH:mm:ss.zzz"));
        detail::QtRequireValue(sTime.isValid() && sTime.toString(QStringLiteral("HH:mm:ss.zzz")) == strText);
        rTime = sTime;
    }
};

template<> struct adl_serializer<QUrl>
{
    /** @brief 输出规范完整编码 URI，拒绝空 URL。 */
    template<typename J> static void to_json(J& rJson, const QUrl& sUrl)
    {
        detail::QtRequireValue(sUrl.isValid() && !sUrl.isEmpty());
        rJson = sUrl.toString(QUrl::FullyEncoded);
    }
    /** @brief StrictMode 解析并规范回验。 */
    template<typename J> static void from_json(const J& sJson, QUrl& rUrl)
    {
        const QString strText = sJson.template get<QString>();
        const QUrl sUrl(strText, QUrl::StrictMode);
        detail::QtRequireValue(sUrl.isValid() && !sUrl.isEmpty() && sUrl.toString(QUrl::FullyEncoded) == strText);
        rUrl = sUrl;
    }
};

template<> struct adl_serializer<QUuid>
{
    /** @brief 写入小写无括号 UUID。 */
    template<typename J> static void to_json(J& rJson, const QUuid& sUuid)
    {
        rJson = sUuid.toString(QUuid::WithoutBraces).toLower();
    }
    /** @brief 接受规范长度及大小写十六进制，零 UUID 合法。 */
    template<typename J> static void from_json(const J& sJson, QUuid& rUuid)
    {
        const QString strText = sJson.template get<QString>();
        const QUuid sUuid(strText);
        detail::QtRequireValue(strText.size() == 36 && sUuid.toString(QUuid::WithoutBraces) == strText.toLower());
        rUuid = sUuid;
    }
};

template<> struct adl_serializer<QDateTime>
{
    /** @brief 保存可交叉校验的 Qt 日期时间对象。 */
    template<typename J> static void to_json(J& rJson, const QDateTime& sDateTime)
    {
        detail::QtRequireValue(sDateTime.isValid());
        const char* szSpec = sDateTime.timeSpec() == Qt::UTC ? "UTC" :
                             sDateTime.timeSpec() == Qt::OffsetFromUTC ? "OffsetFromUTC" :
                             sDateTime.timeSpec() == Qt::LocalTime ? "LocalTime" : "TimeZone";
        J sTemporary{{"date", sDateTime.date()}, {"time", sDateTime.time()},
            {"epochMilliseconds", sDateTime.toMSecsSinceEpoch()}, {"timeSpec", szSpec},
            {"offsetSeconds", sDateTime.offsetFromUtc()}};
        if (sDateTime.timeSpec() == Qt::LocalTime || sDateTime.timeSpec() == Qt::TimeZone)
        {
            const QByteArray bytesId = sDateTime.timeZone().id();
            detail::QtRequireValue(!bytesId.isEmpty() && QTimeZone(bytesId).isValid());
            sTemporary["timeZoneId"] = QString::fromUtf8(bytesId);
        }
        rJson.swap(sTemporary);
    }
    /** @brief 根据 epoch 恢复并校验所有墙上时间及时区字段。 */
    template<typename J> static void from_json(const J& sJson, QDateTime& rDateTime)
    {
        if (!sJson.is_object())
        {
            JSON_THROW(qt_error(qt_error_category::type));
        }
        const qint64 nEpoch = detail::QtValueRead<qint64>(sJson, "epochMilliseconds");
        const int nOffset = detail::QtValueRead<int>(sJson, "offsetSeconds");
        const QString strSpec = detail::QtValueRead<QString>(sJson, "timeSpec");
        QDateTime sResult;
        if (strSpec == QStringLiteral("UTC"))
        {
            sResult = QDateTime::fromMSecsSinceEpoch(nEpoch, Qt::UTC);
        }
        else if (strSpec == QStringLiteral("OffsetFromUTC"))
        {
            detail::QtRequireValue(nOffset != 0 && nOffset > -86400 && nOffset < 86400);
            sResult = QDateTime::fromMSecsSinceEpoch(nEpoch, Qt::OffsetFromUTC, nOffset);
        }
        else
        {
            detail::QtRequireValue(strSpec == QStringLiteral("LocalTime") || strSpec == QStringLiteral("TimeZone"));
            const QByteArray bytesId = detail::QtEncode(detail::QtValueRead<QString>(sJson, "timeZoneId"));
            const QTimeZone sZone(bytesId);
            detail::QtRequireValue(sZone.isValid());
            if (strSpec == QStringLiteral("LocalTime"))
            {
                detail::QtRequireValue(bytesId == QTimeZone::systemTimeZoneId());
                sResult = QDateTime::fromMSecsSinceEpoch(nEpoch, Qt::LocalTime);
            }
            else
            {
                sResult = QDateTime::fromMSecsSinceEpoch(nEpoch, sZone);
            }
        }
        detail::QtRequireValue(sResult.isValid() && sResult.toMSecsSinceEpoch() == nEpoch &&
                               sResult.offsetFromUtc() == nOffset && sResult.date() == detail::QtValueRead<QDate>(sJson, "date") &&
                               sResult.time() == detail::QtValueRead<QTime>(sJson, "time"));
        rDateTime.swap(sResult);
    }
};
NLOHMANN_JSON_NAMESPACE_END
