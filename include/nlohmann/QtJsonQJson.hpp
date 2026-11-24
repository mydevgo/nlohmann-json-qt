/**
 * @file QtJsonQJson.hpp
 * @brief Qt5 QJson 与精确整数 DOM 的显式精度检查桥接。
 * @author 黄凯
 * @date 2026-11-24
 * SPDX-License-Identifier: MIT
 */
#pragma once
#include <nlohmann/QtJsonValues.hpp>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>

NLOHMANN_JSON_NAMESPACE_BEGIN
namespace detail
{
/** @brief 从 QJson 递归生成 DOM；Undefined 和非有限数不降级。 */
template<typename J>
J QtFromQJson(const QJsonValue& sValue)
{
    switch (sValue.type())
    {
        case QJsonValue::Null:
            return nullptr;
        case QJsonValue::Bool:
            return sValue.toBool();
        case QJsonValue::Double:
            if (!std::isfinite(sValue.toDouble()))
            {
                JSON_THROW(qt_error(qt_error_category::range));
            }
            return sValue.toDouble();
        case QJsonValue::String:
            return sValue.toString();
        case QJsonValue::Array:
        {
            J sArray = J::array();
            const QJsonArray sValues = sValue.toArray();
            int nIndex = 0;
            for (const QJsonValue& sItem : sValues)
            {
                JSON_TRY { sArray.push_back(QtFromQJson<J>(sItem)); }
                JSON_CATCH(const qt_error & sError)
                {
                    JSON_THROW(QtPrepend(sError, QStringLiteral("/") + QString::number(nIndex)));
                }
                ++nIndex;
            }
            return sArray;
        }
        case QJsonValue::Object:
        {
            J sObject = J::object();
            const QJsonObject sValues = sValue.toObject();
            for (auto it = sValues.begin(); it != sValues.end(); ++it)
            {
                sObject[QtKey<typename J::string_t>(it.key())] = QtWriteAt<J>(it.value(), QtToken(it.key()));
            }
            return sObject;
        }
        default:
            JSON_THROW(qt_error(qt_error_category::value));
    }
}

/** @brief 递归投影 QJson，拒绝 double 无法精确保存的整数。 */
template<typename J>
QJsonValue QtToQJson(const J& sJson)
{
    if (sJson.is_structured() && sJson.size() >= static_cast<std::size_t>((std::numeric_limits<int>::max)()))
    {
        JSON_THROW(qt_error(qt_error_category::range));
    }
    if (sJson.is_null())
    {
        return QJsonValue(QJsonValue::Null);
    }
    if (sJson.is_boolean())
    {
        return QJsonValue(sJson.template get<bool>());
    }
    if (sJson.is_number())
    {
        return QJsonValue(QtFloat<double>(sJson));
    }
    if (sJson.is_string())
    {
        return QJsonValue(sJson.template get<QString>());
    }
    if (sJson.is_array())
    {
        QJsonArray sArray;
        std::size_t nIndex = 0;
        for (const J& sItem : sJson)
        {
            JSON_TRY { sArray.append(QtToQJson(sItem)); }
            JSON_CATCH(const qt_error & sError)
            {
                JSON_THROW(QtPrepend(sError, QStringLiteral("/") + QString::number(static_cast<qulonglong>(nIndex))));
            }
            ++nIndex;
        }
        return sArray;
    }
    if (sJson.is_object())
    {
        QJsonObject sObject;
        for (auto it = sJson.begin(); it != sJson.end(); ++it)
        {
            const QString strKey = QtDecode(it.key());
            JSON_TRY { sObject.insert(strKey, QtToQJson(it.value())); }
            JSON_CATCH(const qt_error & sError)
            {
                JSON_THROW(QtPrepend(sError, QtToken(strKey)));
            }
        }
        return sObject;
    }
    JSON_THROW(qt_error(qt_error_category::type));
}
}

template<> struct adl_serializer<QJsonValue>
{
    /** @brief 转入 QJson 值，失败不提交。 */
    template<typename J> static void from_json(const J& sJson, QJsonValue& rValue)
    {
        const QJsonValue sTemporary = detail::QtToQJson(sJson);
        rValue = sTemporary;
    }
    /** @brief 转出 QJson 值，拒绝 Undefined。 */
    template<typename J> static void to_json(J& rJson, const QJsonValue& sValue)
    {
        J sTemporary = detail::QtFromQJson<J>(sValue);
        rJson.swap(sTemporary);
    }
};

template<> struct adl_serializer<QJsonArray>
{
    /** @brief 只接受数组根节点。 */
    template<typename J> static void from_json(const J& sJson, QJsonArray& rValue)
    {
        if (!sJson.is_array())
        {
            JSON_THROW(qt_error(qt_error_category::type));
        }
        QJsonArray sTemporary = detail::QtToQJson(sJson).toArray();
        rValue.swap(sTemporary);
    }
    /** @brief QJson 数组递归转为 DOM。 */
    template<typename J> static void to_json(J& rJson, const QJsonArray& sValue)
    {
        adl_serializer<QJsonValue>::to_json(rJson, QJsonValue(sValue));
    }
};

template<> struct adl_serializer<QJsonObject>
{
    /** @brief 只接受对象根节点。 */
    template<typename J> static void from_json(const J& sJson, QJsonObject& rValue)
    {
        if (!sJson.is_object())
        {
            JSON_THROW(qt_error(qt_error_category::type));
        }
        QJsonObject sTemporary = detail::QtToQJson(sJson).toObject();
        rValue.swap(sTemporary);
    }
    /** @brief QJson 对象递归转为 DOM。 */
    template<typename J> static void to_json(J& rJson, const QJsonObject& sValue)
    {
        adl_serializer<QJsonValue>::to_json(rJson, QJsonValue(sValue));
    }
};

template<> struct adl_serializer<QJsonDocument>
{
    /** @brief 文档根只允许数组或对象。 */
    template<typename J> static void from_json(const J& sJson, QJsonDocument& rValue)
    {
        QJsonDocument sTemporary;
        if (sJson.is_array())
        {
            sTemporary = QJsonDocument(sJson.template get<QJsonArray>());
        }
        else if (sJson.is_object())
        {
            sTemporary = QJsonDocument(sJson.template get<QJsonObject>());
        }
        else
        {
            JSON_THROW(qt_error(qt_error_category::type));
        }
        rValue.swap(sTemporary);
    }
    /** @brief 拒绝 null document，不猜测为 JSON null。 */
    template<typename J> static void to_json(J& rJson, const QJsonDocument& sValue)
    {
        detail::QtRequireValue(!sValue.isNull());
        if (sValue.isArray())
        {
            rJson = sValue.array();
        }
        else
        {
            rJson = sValue.object();
        }
    }
};
NLOHMANN_JSON_NAMESPACE_END
