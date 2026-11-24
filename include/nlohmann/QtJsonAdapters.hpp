/**
 * @file QtJsonAdapters.hpp
 * @brief Qt 文本值的原生 ADL 适配。
 * @author 黄凯
 * @date 2026-11-24
 * SPDX-License-Identifier: MIT
 */
#pragma once

#include <nlohmann/QtJsonText.hpp>
#include <nlohmann/adl_serializer.hpp>

NLOHMANN_JSON_NAMESPACE_BEGIN

template<>
struct adl_serializer<QString>
{
    /** @brief 严格编码 QString；失败不修改输出节点。 */
    template<typename J>
    static void to_json(J& rJson, const QString& strText)
    {
        rJson = detail::QtKey<typename J::string_t>(strText);
    }
    /** @brief 严格解码 JSON string；失败不修改输出文本。 */
    template<typename J>
    static void from_json(const J& sJson, QString& rText)
    {
        if (!sJson.is_string())
        {
            JSON_THROW(qt_error(qt_error_category::type));
        }
        QString strTemporary = detail::QtDecode(sJson.template get_ref<const typename J::string_t&>());
        rText.swap(strTemporary);
    }
};

template<>
struct adl_serializer<QChar>
{
    /** @brief 写入单个非代理 BMP 字符。 */
    template<typename J>
    static void to_json(J& rJson, QChar sChar)
    {
        if (sChar.isSurrogate())
        {
            JSON_THROW(qt_error(qt_error_category::encoding));
        }
        rJson = QString(sChar);
    }
    /** @brief 读取单个非代理 BMP 字符。 */
    template<typename J>
    static void from_json(const J& sJson, QChar& rChar)
    {
        const QString strText = sJson.template get<QString>();
        if (strText.size() != 1 || strText.at(0).isSurrogate())
        {
            JSON_THROW(qt_error(qt_error_category::value));
        }
        rChar = strText.at(0);
    }
};

// 字节必须由文档入口或显式 Base64 入口解释，不允许误识别成数字数组。
template<>
struct adl_serializer<QByteArray>
{};

NLOHMANN_JSON_NAMESPACE_END
