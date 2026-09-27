#pragma once

#include <cstddef>
#include <functional>
#include <span>
#include <vector>

#include "Calc/Buffs.hpp"
#include "Calc/Formula.hpp"
#include "Model/Types.hpp"

namespace e33::opt
{
struct Request
{
    Stats base_stats{};
    std::vector<Picto> pictos{};
    std::vector<Lumina> luminas{};
    std::vector<calc::Buff> buffs{};
    Skill skill{};
    Target target{};
    calc::FormulaCoefficients coefficients{};
    int picto_slots{3};
    int lumina_budget{0};
    bool include_unowned{false};
};

struct Candidate
{
    std::vector<const Picto*> pictos{};
    std::vector<const Lumina*> luminas{};
    double expected_damage{0.0};
    int lumina_cost{0};
};

struct Options
{
    std::size_t top_n{5};
    // 0 = branch and bound exaustivo (resultado provadamente ótimo).
    // >0 = beam search com esta largura, para os casos que ainda explodem.
    std::size_t beam_width{0};
    // Teto de segurança: acima disso a busca exaustiva vira beam sozinha, em
    // vez de rodar por minutos e o usuário achar que travou.
    std::size_t auto_beam_threshold{400'000};
};

struct Result
{
    std::vector<Candidate> top{};
    std::size_t evaluated{0};
    std::size_t pruned_by_bound{0};
    std::size_t candidates_after_dominance{0};
    bool exhaustive{false};  // true = ótimo provado; false = beam, pode não ser
    bool cancelled{false};
};

// Chamado periodicamente com o progresso em 0..1. Devolver false cancela.
using ProgressFn = std::function<bool(double)>;

// Roda a busca de forma síncrona. Quem quiser em background usa Job.
[[nodiscard]] Result search(const Request& request, const Options& options = {},
                            const ProgressFn& progress = {});

// Dano de uma combinação específica, para comparar a build atual com a sugerida.
[[nodiscard]] double evaluate(const Request& request, std::span<const Picto* const> pictos,
                              std::span<const Lumina* const> luminas);
} // namespace e33::opt
