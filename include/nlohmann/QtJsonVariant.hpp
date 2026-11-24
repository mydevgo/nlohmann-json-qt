/**
 * @file QtJsonVariant.hpp
 * @brief QVariant 白名单投影，不自动恢复元类型。
 * @author 黄凯
 * @date 2026-11-24
 * SPDX-License-Identifier: MIT
 */
#pragma once
#include <nlohmann/QtJsonQJson.hpp>
#include <QVariant>

NLOHMANN_JSON_NAMESPACE_BEGIN
template<> struct adl_serializer<QVariant>
{
    /**
     * @brief 按固定元类型白名单序列化。
     * @details 协议分派集中保留，各类型复用已有值适配；拒绝隐式 canConvert 兜底。
     */
    template<typename J> static void to_json(J& rJson, const QVariant& sValue)
    {
        J sTemporary = _writeScalar<J>(sValue);
        rJson.swap(sTemporary);
    }

    /** @brief 按 JSON 节点类型投影，拒绝 binary/discarded，不猜测领域元类型。 */
    template<typename J> static void from_json(const J& sJson, QVariant& rValue)
    {
        QVariant sTemporary;
        if (sJson.is_null()) {}
        else if (sJson.is_boolean())
        {
            sTemporary = sJson.template get<bool>();
        }
        else if (sJson.is_number_unsigned())
        {
            sTemporary = QVariant::fromValue(sJson.template get<qulonglong>());
        }
        else if (sJson.is_number_integer())
        {
            sTemporary = QVariant::fromValue(sJson.template get<qlonglong>());
        }
        else if (sJson.is_number_float())
        {
            const double dValue = sJson.template get<double>();
            if (!std::isfinite(dValue))
            {
                JSON_THROW(qt_error(qt_error_category::range));
            }
            sTemporary = dValue;
        }
        else if (sJson.is_string())
        {
            sTemporary = sJson.template get<QString>();
        }
        else if (sJson.is_array())
        {
            sTemporary = QVariant::fromValue(sJson.template get<QVariantList>());
        }
        else if (sJson.is_object())
        {
            sTemporary = QVariant::fromValue(sJson.template get<QVariantMap>());
        }
        else
        {
            JSON_THROW(qt_error(qt_error_category::type));
        }
        rValue.swap(sTemporary);
    }
  private:
    /** @brief 标量元类型分派，复用普通 JSON 数值规则。 */
    template<typename J> static J _writeScalar(const QVariant& sValue)
    {
        J sTemporary;
        switch (sValue.userType())
        {
            case QMetaType::UnknownType:
                sTemporary = nullptr;
                break;
            case QMetaType::Bool:
                sTemporary = sValue.value<bool>();
                break;
            case QMetaType::Char:
                sTemporary = sValue.value<char>();
                break;
            case QMetaType::SChar:
                sTemporary = sValue.value<signed char>();
                break;
            case QMetaType::UChar:
                sTemporary = sValue.value<unsigned char>();
                break;
            case QMetaType::Short:
                sTemporary = sValue.value<short>();
                break;
            case QMetaType::UShort:
                sTemporary = sValue.value<unsigned short>();
                break;
            case QMetaType::Int:
                sTemporary = sValue.value<int>();
                break;
            case QMetaType::UInt:
                sTemporary = sValue.value<unsigned int>();
                break;
            case QMetaType::Long:
                sTemporary = sValue.value<long>();
                break;
            case QMetaType::ULong:
                sTemporary = sValue.value<unsigned long>();
                break;
            case QMetaType::LongLong:
                sTemporary = sValue.value<qlonglong>();
                break;
            case QMetaType::ULongLong:
                sTemporary = sValue.value<qulonglong>();
                break;
            case QMetaType::Float:
            case QMetaType::Double:
                if (!std::isfinite(sValue.toDouble()))
                {
                    JSON_THROW(qt_error(qt_error_category::range));
                }
                sTemporary = sValue.toDouble();
                break;
            default:
                return _writeQtValue<J>(sValue);
        }
        return sTemporary;
    }

    /** @brief Qt 业务元类型白名单分派，拒绝隐式字符串兜底。 */
    template<typename J> static J _writeQtValue(const QVariant& sValue)
    {
        J sTemporary;
        switch (sValue.userType())
        {
            case QMetaType::QString:
                sTemporary = sValue.value<QString>();
                break;
            case QMetaType::QChar:
                sTemporary = sValue.value<QChar>();
                break;
            case QMetaType::QStringList:
                sTemporary = sValue.value<QStringList>();
                break;
            case QMetaType::QVariantList:
                sTemporary = sValue.value<QVariantList>();
                break;
            case QMetaType::QVariantMap:
                sTemporary = sValue.value<QVariantMap>();
                break;
            case QMetaType::QVariantHash:
                sTemporary = sValue.value<QVariantHash>();
                break;
            case QMetaType::QDate:
                sTemporary = sValue.value<QDate>();
                break;
            case QMetaType::QTime:
                sTemporary = sValue.value<QTime>();
                break;
            case QMetaType::QDateTime:
                sTemporary = sValue.value<QDateTime>();
                break;
            case QMetaType::QUrl:
                sTemporary = sValue.value<QUrl>();
                break;
            case QMetaType::QUuid:
                sTemporary = sValue.value<QUuid>();
                break;
            case QMetaType::QJsonValue:
                sTemporary = sValue.value<QJsonValue>();
                break;
            case QMetaType::QJsonArray:
                sTemporary = sValue.value<QJsonArray>();
                break;
            case QMetaType::QJsonObject:
                sTemporary = sValue.value<QJsonObject>();
                break;
            case QMetaType::QJsonDocument:
                sTemporary = sValue.value<QJsonDocument>();
                break;
            default:
                JSON_THROW(qt_error(qt_error_category::unsupported));
        }
        return sTemporary;
    }

};
NLOHMANN_JSON_NAMESPACE_END
