#pragma once

#include <span>
#include <string>
#include <vector>

#include "Model/Types.hpp"

namespace e33::calc
{
// Buff ativo no momento do golpe. `stacks` é quantas vezes está aplicado e
// `max_stacks` o teto — empilhamento sem teto é a causa mais comum de um
// otimizador sugerir uma build que na prática rende menos.
struct Buff
{
    std::string id{};
    std::string name{};
    Stats per_stack{};
    int stacks{1};
    int max_stacks{1};
};

// Soma os buffs respeitando o teto de cada um.
[[nodiscard]] Stats accumulate(std::span<const Buff> buffs);

// Stats totais do personagem: base + arma + pictos + luminas + buffs.
[[nodiscard]] Stats total_stats(const Stats& base, std::span<const Picto* const> pictos,
                                std::span<const Lumina* const> luminas,
                                std::span<const Buff> buffs);
} // namespace e33::calc
