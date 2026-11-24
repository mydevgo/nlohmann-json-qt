/**
 * @file QtJsonStrict.hpp
 * @brief 明确命名的严格转换及其支持类型约束。
 * @author 黄凯
 * @date 2026-11-24
 * SPDX-License-Identifier: MIT
 */
#pragma once
#include <nlohmann/QtJsonVariant.hpp>
#include <utility>

NLOHMANN_JSON_NAMESPACE_BEGIN
namespace detail
{
template<typename T, typename Enable = void>
struct CQtStrictCodec
{
    static constexpr bool supported = false; ///< 禁止静默回退为普通转换
};

template<typename T>
struct CQtStrictCodec < T, enable_if_t < std::is_integral<T>::value && !std::is_same<T, bool>::value && !std::is_same<T, char>::value >>
{
    static constexpr bool supported = true; ///< 整型严格检查范围及类型
    template<typename J> static T Read(const J& sJson)
    {
        return QtInteger<T>(sJson);
    }
};

template<typename T>
struct CQtStrictCodec<T, enable_if_t<std::is_floating_point<T>::value>>
{
    static constexpr bool supported = true; ///< 浮点严格检查范围及精度
    template<typename J> static T Read(const J& sJson)
    {
        return QtFloat<T>(sJson);
    }
};

template<> struct CQtStrictCodec<bool>
{
    static constexpr bool supported = true; ///< bool 不接受整数
    template<typename J> static bool Read(const J& sJson)
    {
        if (!sJson.is_boolean())
        {
            JSON_THROW(qt_error(qt_error_category::type));
        }
        return sJson.template get<bool>();
    }
};

template<> struct CQtStrictCodec<std::nullptr_t>
{
    static constexpr bool supported = true; ///< 显式 null 目标
    template<typename J> static std::nullptr_t Read(const J& sJson)
    {
        if (!sJson.is_null())
        {
            JSON_THROW(qt_error(qt_error_category::type));
        }
        return nullptr;
    }
};

template<typename T> struct CQtStrictNative
{
    static constexpr bool supported = true; ///< Qt 值适配本身检查有效性
    template<typename J> static T Read(const J& sJson)
    {
        return sJson.template get<T>();
    }
};
template<> struct CQtStrictCodec<QString> : CQtStrictNative<QString> {};
template<> struct CQtStrictCodec<QChar> : CQtStrictNative<QChar> {};
template<> struct CQtStrictCodec<QDate> : CQtStrictNative<QDate> {};
template<> struct CQtStrictCodec<QTime> : CQtStrictNative<QTime> {};
template<> struct CQtStrictCodec<QDateTime> : CQtStrictNative<QDateTime> {};
template<> struct CQtStrictCodec<QUrl> : CQtStrictNative<QUrl> {};
template<> struct CQtStrictCodec<QUuid> : CQtStrictNative<QUuid> {};
template<> struct CQtStrictCodec<QVariant> : CQtStrictNative<QVariant> {};
template<> struct CQtStrictCodec<QJsonValue> : CQtStrictNative<QJsonValue> {};
template<> struct CQtStrictCodec<QJsonArray> : CQtStrictNative<QJsonArray> {};
template<> struct CQtStrictCodec<QJsonObject> : CQtStrictNative<QJsonObject> {};
template<> struct CQtStrictCodec<QJsonDocument> : CQtStrictNative<QJsonDocument> {};

/** @brief 严格入口补充 null 分类及字段/容器路径。 */
template<typename T, typename J>
T QtStrictRead(const J& sJson, const QString& strPath = QString())
{
    static_assert(CQtStrictCodec<T>::supported, "get_strict: unsupported target; strict recursion requires a supported Qt type");
    if (sJson.is_null() && !std::is_same<T, std::nullptr_t>::value && !std::is_same<T, QVariant>::value && !std::is_same<T, QJsonValue>::value)
    {
        JSON_THROW(qt_error(qt_error_category::null_value, strPath));
    }
    JSON_TRY { return CQtStrictCodec<T>::Read(sJson); }
    JSON_CATCH(const qt_error & sError)
    {
        JSON_THROW(QtPrepend(sError, strPath));
    }
    JSON_CATCH(const exception&)
    {
        JSON_THROW(qt_error(qt_error_category::value, strPath));
    }
}

template<typename C> struct CQtStrictSequence
{
    static constexpr bool supported = CQtStrictCodec<typename C::value_type>::supported; ///< 元素必须可严格读取
    /** @brief 严格递归读取数组，元素数值不使用普通 get。 */
    template<typename J> static C Read(const J& sJson)
    {
        if (!sJson.is_array())
        {
            JSON_THROW(qt_error(qt_error_category::type));
        }
        if (sJson.size() >= static_cast<std::size_t>((std::numeric_limits<int>::max)()))
        {
            JSON_THROW(qt_error(qt_error_category::range));
        }
        C sOutput;
        std::size_t nIndex = 0;
        for (const J& sValue : sJson)
        {
            QtAppend(sOutput, QtStrictRead<typename C::value_type>(sValue, QStringLiteral("/") + QString::number(static_cast<qulonglong>(nIndex++))));
        }
        return sOutput;
    }
};
template<typename T> struct CQtStrictCodec<QList<T>> : CQtStrictSequence<QList<T>> {};
template<typename T> struct CQtStrictCodec<QVector<T>> : CQtStrictSequence<QVector<T>> {};
template<typename T> struct CQtStrictCodec<QSet<T>> : CQtStrictSequence<QSet<T>> {};
template<> struct CQtStrictCodec<QStringList> : CQtStrictSequence<QStringList> {};

template<typename C> struct CQtStrictMap
{
    static constexpr bool supported = std::is_same<typename C::key_type, QString>::value && CQtStrictCodec<typename C::mapped_type>::supported; ///< 键和元素均受约束
    /** @brief 严格递归读取对象值。 */
    template<typename J> static C Read(const J& sJson)
    {
        if (!sJson.is_object())
        {
            JSON_THROW(qt_error(qt_error_category::type));
        }
        if (sJson.size() >= static_cast<std::size_t>((std::numeric_limits<int>::max)()))
        {
            JSON_THROW(qt_error(qt_error_category::range));
        }
        C sOutput;
        for (auto it = sJson.begin(); it != sJson.end(); ++it)
        {
            const QString strKey = QtDecode(it.key());
            sOutput.insert(strKey, QtStrictRead<typename C::mapped_type>(it.value(), QtToken(strKey)));
        }
        return sOutput;
    }
};
template<typename K, typename T> struct CQtStrictCodec<QMap<K, T>> : CQtStrictMap<QMap<K, T>> {};
template<typename K, typename T> struct CQtStrictCodec<QHash<K, T>> : CQtStrictMap<QHash<K, T>> {};

using std::swap;
/** @brief ADL swap 检查与实际提交使用相同表达式。 */
template<typename T>
void QtStrictSwap(T& rLeft, T& rRight) noexcept(noexcept(swap(rLeft, rRight)))
{
    swap(rLeft, rRight);
}

/** @brief 按 RFC 6901 遍历；缺失、父节点类型错误及 null 分开报告。 */
template<typename J>
const J& QtLocate(const J& sRoot, const QString& strPointer)
{
    const typename J::json_pointer sValidated(strPointer);
    (void)sValidated;
    if (strPointer.isEmpty())
    {
        return sRoot;
    }
    const QStringList lstTokens = strPointer.mid(1).split(QLatin1Char('/'), Qt::KeepEmptyParts);
    const J* pCurrent = &sRoot;
    QString strPath;
    for (QString strToken : lstTokens)
    {
        strToken.replace(QStringLiteral("~1"), QStringLiteral("/"));
        strToken.replace(QStringLiteral("~0"), QStringLiteral("~"));
        if (pCurrent->is_null())
        {
            JSON_THROW(qt_error(qt_error_category::null_value, strPath));
        }
        strPath += QtToken(strToken);
        if (pCurrent->is_object())
        {
            const auto it = pCurrent->find(QtKey<typename J::string_t>(strToken));
            if (it == pCurrent->end())
            {
                JSON_THROW(qt_error(qt_error_category::missing, strPath));
            }
            pCurrent = &it.value();
        }
        else if (pCurrent->is_array())
        {
            typename J::size_type nIndex = 0;
            const typename J::string_t strIndex = QtKey<typename J::string_t>(strToken);
            if (parse_array_index(strIndex, nIndex) != array_index_status::ok)
            {
                JSON_THROW(qt_error(qt_error_category::value, strPath));
            }
            if (nIndex >= pCurrent->size())
            {
                JSON_THROW(qt_error(qt_error_category::missing, strPath));
            }
            pCurrent = &pCurrent->at(nIndex);
        }
        else
        {
            JSON_THROW(qt_error(qt_error_category::type, strPath));
        }
    }
    return *pCurrent;
}
}
NLOHMANN_JSON_NAMESPACE_END
