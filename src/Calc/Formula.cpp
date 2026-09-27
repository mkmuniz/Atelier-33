#include "Calc/Formula.hpp"

#include <algorithm>
#include <cmath>

#include "Support/Json.hpp"

namespace e33::calc
{
std::string FormulaCoefficients::to_json_string() const
{
    Json root = Json::object();
    root["attack_exponent"] = attack_exponent;
    root["defense_weight"] = defense_weight;
    root["defense_softness"] = defense_softness;
    root["crit_base"] = crit_base;
    root["global_scale"] = global_scale;
    return root.dump(2) + "\n";
}

bool FormulaCoefficients::apply_json(std::string_view text)
{
    const auto parsed = parse_json(text);
    if (!parsed || !parsed->is_object())
    {
        return false;
    }
    const auto number = [&parsed](std::string_view key, double& out) {
        if (const auto it = parsed->find(key); it != parsed->end() && it->is_number())
        {
            out = it->get<double>();
        }
    };
    number("attack_exponent", attack_exponent);
    number("defense_weight", defense_weight);
    number("defense_softness", defense_softness);
    number("crit_base", crit_base);
    number("global_scale", global_scale);

    // Suavidade zerada ou negativa zeraria todo o dano silenciosamente.
    defense_softness = std::max(defense_softness, 1e-6);
    return true;
}

bool FormulaCoefficients::load(const std::filesystem::path& path)
{
    const auto text = read_text_file(path);
    return text.has_value() && apply_json(*text);
}

double DamageBreakdown::without_crit() const
{
    return base * damage_bonus * element_bonus * weakness * brk * hits;
}

double DamageBreakdown::on_crit(double crit_multiplier) const
{
    return without_crit() * crit_multiplier;
}

double DamageBreakdown::expected(double crit_rate, double crit_multiplier) const
{
    const auto rate = std::clamp(crit_rate, 0.0, 1.0);
    return without_crit() * (1.0 + rate * (crit_multiplier - 1.0));
}

DamageResult compute_damage(const Stats& total, const Skill& skill, const Target& target,
                            const FormulaCoefficients& coefficients)
{
    DamageResult result;

    const auto attack = std::max(total.attack, 0.0);
    const auto scaled_attack = std::pow(attack, coefficients.attack_exponent);
    const auto defense = std::max(target.defense, 0.0) * coefficients.defense_weight;

    // A mitigação depende só da defesa do alvo. Manter o ataque fora dela é o
    // que garante que o expoente seja o único controle da curva de ataque —
    // misturar os dois torna impossível ajustar um sem mexer no outro quando as
    // fixtures chegarem.
    const auto mitigation = coefficients.defense_softness
                            / (coefficients.defense_softness + defense);

    result.breakdown.base = coefficients.global_scale * skill.power * scaled_attack * mitigation;

    result.breakdown.damage_bonus = 1.0 + total.damage_bonus;
    result.breakdown.element_bonus = 1.0 + total.bonus_for(skill.element);
    result.breakdown.hits = static_cast<double>(std::max(skill.hits, 1));

    if (skill.element != Element::None && skill.element == target.weakness)
    {
        result.breakdown.weakness = target.weakness_multiplier;
    }
    if (target.broken)
    {
        result.breakdown.brk = target.break_multiplier;
    }

    result.crit_rate = skill.can_crit ? std::clamp(total.crit_rate, 0.0, 1.0) : 0.0;
    result.crit_multiplier = coefficients.crit_base + total.crit_damage;
    return result;
}
} // namespace e33::calc
