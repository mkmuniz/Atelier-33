#pragma once

namespace e33::calc
{
// Breakdown por multiplicador. Importa mais que o numero final: é o que faz o
// usuario confiar no resultado em vez de achar que é chute.
struct DamageBreakdown
{
    double base{0.0};
    double buffs{1.0};
    double brk{1.0};            // break
    double elemental_weakness{1.0};
    double critical{1.0};

    [[nodiscard]] double total() const
    {
        return base * buffs * brk * elemental_weakness * critical;
    }
};
} // namespace e33::calc
