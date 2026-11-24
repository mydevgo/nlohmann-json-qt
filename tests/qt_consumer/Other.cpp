/**
 * @file Other.cpp
 * @brief JSON 在前时的独立翻译单元与默认 Qt 宏验证。
 * @author 黄凯
 * @date 2026-11-24
 * SPDX-License-Identifier: MIT
 */
#include <nlohmann/json.hpp>
#include <QString>

/** @brief 跨翻译单元返回 Qt 容器内的文本值。 */
QString OtherQtValue()
{
    const nlohmann::ordered_json sJson = QStringList{QStringLiteral("installed")};
    return sJson.get<QStringList>().front();
}
