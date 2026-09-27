#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace e33
{
// Elementos do jogo. `None` existe para skills físicas e para "sem fraqueza".
enum class Element : std::uint8_t
{
    None,
    Fire,
    Ice,
    Lightning,
    Earth,
    Light,
    Dark,
    Count,
};

[[nodiscard]] std::string_view element_name(Element element);
[[nodiscard]] Element element_from_name(std::string_view name);

// Bloco de stats aditivo. Um picto, uma arma e um buff somam aqui; a fórmula
// lê o total. Percentuais são frações (0.25 = +25%), nunca 25.0, para não
// existirem dois significados do mesmo campo em lugares diferentes.
struct Stats
{
    double attack{0.0};
    double defense{0.0};
    double health{0.0};
    double speed{0.0};
    double crit_rate{0.0};      // fração, 0..1
    double crit_damage{0.0};    // fração somada ao multiplicador base de crítico
    double damage_bonus{0.0};   // fração, multiplicador geral de dano
    std::array<double, static_cast<std::size_t>(Element::Count)> element_bonus{};

    Stats& operator+=(const Stats& other);
    [[nodiscard]] double bonus_for(Element element) const;
    [[nodiscard]] bool dominates(const Stats& other) const;
    [[nodiscard]] bool operator==(const Stats& other) const = default;
};

[[nodiscard]] Stats operator+(Stats lhs, const Stats& rhs);

struct Picto
{
    std::string id{};
    std::string name{};
    int lumina_cost{0};   // custo em pontos de lumina, se equipado como lumina
    Stats stats{};
    bool owned{true};     // toggle "ignorar pictos ainda nao obtidos" (M6)
};

struct Lumina
{
    std::string id{};
    std::string name{};
    int cost{0};
    Stats stats{};
    bool owned{true};
};

struct Weapon
{
    std::string id{};
    std::string name{};
    Stats stats{};
    Element element{Element::None};
};

struct Skill
{
    std::string id{};
    std::string name{};
    double power{1.0};          // multiplicador base da skill
    Element element{Element::None};
    int hits{1};
    bool can_crit{true};
};

// Alvo do cálculo: é o inimigo mais o estado dele no momento do golpe.
struct Target
{
    std::string id{};
    std::string name{};
    double defense{0.0};
    Element weakness{Element::None};
    double weakness_multiplier{1.5};
    bool broken{false};                  // estado de break
    double break_multiplier{1.5};
};

struct Character
{
    std::string id{};
    std::string name{};
    Stats base_stats{};
    int picto_slots{3};
    int lumina_budget{0};
};
} // namespace e33
