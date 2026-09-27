#pragma once

#include <filesystem>
#include <string>
#include <string_view>

#include "Model/Types.hpp"

namespace e33::calc
{
// Coeficientes da fórmula, em dados e não em código.
//
// A forma da fórmula (como os multiplicadores se compõem) é estrutura; os
// números são medição. Separar os dois é o que faz o M4 ser "ajustar
// coeficientes contra as fixtures" em vez de "reescrever e recompilar a cada
// tentativa" — e é o que permite corrigir um patch do jogo editando um JSON.
//
// TODO(M4, só mensurável em jogo): os valores abaixo são um ponto de partida
// plausível, NÃO medidos no E33. Ajustar contra tests/fixtures/ até o erro
// médio ficar abaixo de 2%, e publicar o número no README.
struct FormulaCoefficients
{
    // Dano = escala * poder * ataque^expoente * mitigação, com
    // mitigação = suavidade / (suavidade + peso * defesa).
    //
    // A mitigação depende só da defesa do alvo, então o ataque escala de forma
    // linear com expoente 1.0. Se as fixtures mostrarem que o jogo é
    // superlinear (build de ataque rendendo mais que o dobro ao dobrar), é o
    // expoente que sobe — sem tocar no código.
    double attack_exponent{1.0};
    double defense_weight{1.0};      // quanto a defesa do alvo pesa
    double defense_softness{300.0};  // defesa igual a isso corta o dano pela metade
    double crit_base{1.5};           // multiplicador de crítico sem bônus
    double global_scale{1.0};        // calibra a ordem de grandeza

    [[nodiscard]] std::string to_json_string() const;
    bool apply_json(std::string_view text);
    bool load(const std::filesystem::path& path);
};

// Dano decomposto por multiplicador.
//
// O breakdown importa mais que o número final: é o que faz o usuário confiar no
// resultado em vez de achar que é chute. Cada campo é um fator independente, e
// o produto deles é o dano.
struct DamageBreakdown
{
    double base{0.0};             // termo de ataque contra defesa, já com a skill
    double damage_bonus{1.0};     // bônus geral de dano
    double element_bonus{1.0};    // afinidade elemental do personagem
    double weakness{1.0};         // fraqueza elemental do alvo
    double brk{1.0};              // alvo em break
    double hits{1.0};             // golpes da skill

    [[nodiscard]] double without_crit() const;
    [[nodiscard]] double on_crit(double crit_multiplier) const;
    // Média ponderada pela taxa de crítico. É o número que o otimizador compara:
    // maximizar o dano crítico sozinho escolheria builds que quase nunca criticam.
    [[nodiscard]] double expected(double crit_rate, double crit_multiplier) const;
};

struct DamageResult
{
    DamageBreakdown breakdown{};
    double crit_rate{0.0};
    double crit_multiplier{1.5};

    [[nodiscard]] double expected() const
    {
        return breakdown.expected(crit_rate, crit_multiplier);
    }
    [[nodiscard]] double minimum() const { return breakdown.without_crit(); }
    [[nodiscard]] double maximum() const { return breakdown.on_crit(crit_multiplier); }
};

[[nodiscard]] DamageResult compute_damage(const Stats& total, const Skill& skill,
                                          const Target& target,
                                          const FormulaCoefficients& coefficients);
} // namespace e33::calc
