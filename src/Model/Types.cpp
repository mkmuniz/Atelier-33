#include "Model/Types.hpp"

#include <algorithm>
#include <array>

namespace e33
{
namespace
{
constexpr std::array<std::string_view, static_cast<std::size_t>(Element::Count)> kElementNames{
    "none", "fire", "ice", "lightning", "earth", "light", "dark",
};
} // namespace

std::string_view element_name(Element element)
{
    const auto index = static_cast<std::size_t>(element);
    return index < kElementNames.size() ? kElementNames[index] : kElementNames[0];
}

Element element_from_name(std::string_view name)
{
    for (std::size_t i = 0; i < kElementNames.size(); ++i)
    {
        if (kElementNames[i] == name)
        {
            return static_cast<Element>(i);
        }
    }
    return Element::None;
}

Stats& Stats::operator+=(const Stats& other)
{
    attack += other.attack;
    defense += other.defense;
    health += other.health;
    speed += other.speed;
    crit_rate += other.crit_rate;
    crit_damage += other.crit_damage;
    damage_bonus += other.damage_bonus;
    for (std::size_t i = 0; i < element_bonus.size(); ++i)
    {
        element_bonus[i] += other.element_bonus[i];
    }
    return *this;
}

Stats operator+(Stats lhs, const Stats& rhs)
{
    lhs += rhs;
    return lhs;
}

double Stats::bonus_for(Element element) const
{
    const auto index = static_cast<std::size_t>(element);
    return index < element_bonus.size() ? element_bonus[index] : 0.0;
}

bool Stats::dominates(const Stats& other) const
{
    // Domina se não é pior em nada. A comparação é campo a campo porque um
    // picto melhor em ataque e pior em crítico não domina coisa nenhuma — e é
    // exatamente esse par que a poda não pode descartar.
    const auto ge = [](double a, double b) { return a >= b; };
    if (!ge(attack, other.attack) || !ge(defense, other.defense) || !ge(health, other.health)
        || !ge(speed, other.speed) || !ge(crit_rate, other.crit_rate)
        || !ge(crit_damage, other.crit_damage) || !ge(damage_bonus, other.damage_bonus))
    {
        return false;
    }
    for (std::size_t i = 0; i < element_bonus.size(); ++i)
    {
        if (!ge(element_bonus[i], other.element_bonus[i]))
        {
            return false;
        }
    }
    return true;
}
} // namespace e33
