/**
 * @file QtJsonContainers.hpp
 * @brief Qt 序列、集合和 QString 键映射的普通转换。
 * @author 黄凯
 * @date 2026-11-24
 * SPDX-License-Identifier: MIT
 */
#pragma once
#include <nlohmann/QtJsonAdapters.hpp>
#include <QHash>
#include <QList>
#include <QMap>
#include <QSet>
#include <QStringList>
#include <QVector>

NLOHMANN_JSON_NAMESPACE_BEGIN
namespace detail
{
/** @brief 序列追加元素，集合按自身去重语义插入。 */
template<typename C>
void QtAppend(C& rValues, const typename C::value_type& sValue)
{
    rValues.push_back(sValue);
}
template<typename T>
void QtAppend(QSet<T>& rValues, const T& sValue)
{
    rValues.insert(sValue);
}

/** @brief 将容器元素错误加上路径，普通数值规则仍由 get 决定。 */
template<typename T, typename J>
T QtReadAt(const J& sJson, const QString& strPath)
{
    JSON_TRY { return sJson.template get<T>(); }
    JSON_CATCH(const qt_error & sError)
    {
        JSON_THROW(QtPrepend(sError, strPath));
    }
    JSON_CATCH(const exception&)
    {
        JSON_THROW(qt_error(qt_error_category::type, strPath));
    }
}

/** @brief 先生成元素节点再提交，附加容器路径。 */
template<typename J, typename T>
J QtWriteAt(const T& sValue, const QString& strPath)
{
    JSON_TRY { return J(sValue); }
    JSON_CATCH(const qt_error & sError)
    {
        JSON_THROW(QtPrepend(sError, strPath));
    }
    JSON_CATCH(const exception&)
    {
        JSON_THROW(qt_error(qt_error_category::type, strPath));
    }
}

template<typename C>
struct CQtSequenceAdapter
{
    /** @brief 数组读取完成后提交，失败保留旧容器。 */
    template<typename J>
    static void from_json(const J& sJson, C& rOutput)
    {
        if (!sJson.is_array())
        {
            JSON_THROW(qt_error(qt_error_category::type));
        }
        if (sJson.size() >= static_cast<std::size_t>((std::numeric_limits<int>::max)()))
        {
            JSON_THROW(qt_error(qt_error_category::range));
        }
        C sTemporary;
        std::size_t nIndex = 0;
        for (const J& sValue : sJson)
        {
            QtAppend(sTemporary, QtReadAt<typename C::value_type>(
                         sValue, QStringLiteral("/") + QString::number(static_cast<qulonglong>(nIndex++))));
        }
        rOutput.swap(sTemporary);
    }
    /** @brief 按迭代顺序生成数组，成功后提交输出节点。 */
    template<typename J>
    static void to_json(J& rJson, const C& sValues)
    {
        J sTemporary = J::array();
        std::size_t nIndex = 0;
        for (const typename C::value_type& sValue : sValues)
        {
            sTemporary.push_back(QtWriteAt<J>(sValue, QStringLiteral("/") + QString::number(static_cast<qulonglong>(nIndex++))));
        }
        rJson.swap(sTemporary);
    }
};

template<typename C, bool = std::is_same<typename C::key_type, QString>::value>
struct CQtMapAdapter {};

template<typename C>
struct CQtMapAdapter<C, true>
{
    /** @brief 读取 QString 键映射，失败保留旧对象。 */
    template<typename J>
    static void from_json(const J& sJson, C& rOutput)
    {
        if (!sJson.is_object())
        {
            JSON_THROW(qt_error(qt_error_category::type));
        }
        if (sJson.size() >= static_cast<std::size_t>((std::numeric_limits<int>::max)()))
        {
            JSON_THROW(qt_error(qt_error_category::range));
        }
        C sTemporary;
        for (auto it = sJson.begin(); it != sJson.end(); ++it)
        {
            const QString strKey = QtDecode(it.key());
            sTemporary.insert(strKey, QtReadAt<typename C::mapped_type>(it.value(), QtToken(strKey)));
        }
        rOutput.swap(sTemporary);
    }
    /** @brief 生成对象；不字符串化非 QString 键。 */
    template<typename J>
    static void to_json(J& rJson, const C& sValues)
    {
        J sTemporary = J::object();
        for (auto it = sValues.cbegin(); it != sValues.cend(); ++it)
        {
            sTemporary[QtKey<typename J::string_t>(it.key())] = QtWriteAt<J>(it.value(), QtToken(it.key()));
        }
        rJson.swap(sTemporary);
    }
};
}

template<typename T> struct adl_serializer<QList<T>> : detail::CQtSequenceAdapter<QList<T>> {};
template<typename T> struct adl_serializer<QVector<T>> : detail::CQtSequenceAdapter<QVector<T>> {};
template<typename T> struct adl_serializer<QSet<T>> : detail::CQtSequenceAdapter<QSet<T>> {};
template<> struct adl_serializer<QStringList> : detail::CQtSequenceAdapter<QStringList> {};
template<typename K, typename V> struct adl_serializer<QMap<K, V>> : detail::CQtMapAdapter<QMap<K, V>> {};
template<typename K, typename V> struct adl_serializer<QHash<K, V>> : detail::CQtMapAdapter<QHash<K, V>> {};
NLOHMANN_JSON_NAMESPACE_END
