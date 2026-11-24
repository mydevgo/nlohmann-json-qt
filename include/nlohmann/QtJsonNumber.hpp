/**
 * @file QtJsonNumber.hpp
 * @brief 不依赖 long double 扩展精度的数值范围及精度检查。
 * @author 黄凯
 * @date 2026-11-24
 * SPDX-License-Identifier: MIT
 */
#pragma once
#include <nlohmann/QtJsonText.hpp>
#include <cmath>
#include <cstdint>
#include <type_traits>

NLOHMANN_JSON_NAMESPACE_BEGIN
namespace detail
{
/** @brief 严格读取整型；先比较边界，再执行不会越界的转换。 */
template<typename T, typename J>
T QtInteger(const J& sJson)
{
    if (!sJson.is_number_integer())
    {
        JSON_THROW(qt_error(qt_error_category::type));
    }
    if (sJson.is_number_unsigned())
    {
        const typename J::number_unsigned_t nValue = sJson.template get<typename J::number_unsigned_t>();
        if (nValue > static_cast<typename J::number_unsigned_t>((std::numeric_limits<T>::max)()))
        {
            JSON_THROW(qt_error(qt_error_category::range));
        }
        return static_cast<T>(nValue);
    }
    const typename J::number_integer_t nValue = sJson.template get<typename J::number_integer_t>();
    if (nValue < 0)
    {
        if (nValue < static_cast<typename J::number_integer_t>((std::numeric_limits<T>::lowest)()))
        {
            JSON_THROW(qt_error(qt_error_category::range));
        }
    }
    else if (static_cast<typename J::number_unsigned_t>(nValue) > static_cast<typename J::number_unsigned_t>((std::numeric_limits<T>::max)()))
    {
        JSON_THROW(qt_error(qt_error_category::range));
    }
    return static_cast<T>(nValue);
}

/** @brief 判断整数的有效二进制位能否由目标浮点尾数精确保存。 */
template<typename T>
bool QtIntegerExact(std::uint64_t nMagnitude)
{
    if (nMagnitude == 0)
    {
        return true;
    }
    while ((nMagnitude & 1u) == 0)
    {
        nMagnitude >>= 1;
    }
    int nBits = 0;
    while (nMagnitude != 0)
    {
        ++nBits;
        nMagnitude >>= 1;
    }
    return nBits <= std::numeric_limits<T>::digits;
}

/** @brief 严格读取浮点，拒绝非有限值、溢出和精度损失。 */
template<typename T, typename J>
T QtFloat(const J& sJson)
{
    if (!sJson.is_number())
    {
        JSON_THROW(qt_error(qt_error_category::type));
    }
    if (sJson.is_number_integer())
    {
        std::uint64_t nMagnitude;
        if (sJson.is_number_unsigned())
        {
            nMagnitude = sJson.template get<std::uint64_t>();
        }
        else
        {
            const std::int64_t nValue = sJson.template get<std::int64_t>();
            nMagnitude = nValue < 0 ? static_cast<std::uint64_t>(-(nValue + 1)) + 1 : static_cast<std::uint64_t>(nValue);
        }
        if (!QtIntegerExact<T>(nMagnitude))
        {
            JSON_THROW(qt_error(qt_error_category::precision));
        }
        return sJson.template get<T>();
    }
    const typename J::number_float_t dValue = sJson.template get<typename J::number_float_t>();
    if (!std::isfinite(dValue) || dValue > (std::numeric_limits<T>::max)() || dValue < -(std::numeric_limits<T>::max)())
    {
        JSON_THROW(qt_error(qt_error_category::range));
    }
    const T dResult = static_cast<T>(dValue);
    if (static_cast<typename J::number_float_t>(dResult) != dValue)
    {
        JSON_THROW(qt_error(qt_error_category::precision));
    }
    return dResult;
}
}
NLOHMANN_JSON_NAMESPACE_END
