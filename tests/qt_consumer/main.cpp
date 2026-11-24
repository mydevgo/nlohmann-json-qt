/**
 * @file main.cpp
 * @brief 安装后 Qt 在前、json_fwd 在前及跨翻译单元消费验证。
 * @author 黄凯
 * @date 2026-11-24
 * SPDX-License-Identifier: MIT
 */
#include <QString>
#include <nlohmann/json_fwd.hpp>
#include <nlohmann/json.hpp>
#include <type_traits>

QString OtherQtValue();

namespace Domain
{
struct SValue
{
    int nCount = 0; ///< 领域数量
};
/** @brief 领域 ADL 保持原生签名。 */
void to_json(nlohmann::json& rJson, const SValue& sValue)
{
    rJson = {{"count", sValue.nCount}};
}
/** @brief 领域类型自主选择普通读取。 */
void from_json(const nlohmann::json& sJson, SValue& rValue)
{
    rValue.nCount = sJson.at("count").get<int>();
}
}

/** @brief 包依赖与接口验证，失败返回非零。 */
int main()
{
    // 准备。
    typedef nlohmann::json J;
    static_assert(!std::is_constructible<J, QByteArray>::value, "QByteArray requires explicit semantics");
    Domain::SValue sValue;
    sValue.nCount = 3;
    // 执行。
    const J sJson = J::parse(QStringLiteral("{\"name\":\"installed\",\"count\":3}"));
    const QString strName = sJson.at(QStringLiteral("name")).get<QString>();
    const QByteArray bytes = sJson.dump_utf8();
    // 断言：仅链接原 target 即可使用 Qt，无额外 Qt include/link 设置。
    if (strName != OtherQtValue() || J::parse(bytes) != sJson)
    {
        return 1;
    }
    if (sJson.read_required<int>(QStringLiteral("/count")) != 3)
    {
        return 2;
    }
    if (J(sValue).get<Domain::SValue>().nCount != 3)
    {
        return 3;
    }
    return 0;
}
