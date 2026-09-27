#include "Optimizer/Prune.hpp"

namespace e33::opt
{
namespace
{
// `a` domina `b` se não custa mais e não é pior em stat nenhum.
template <typename T>
bool dominates(const T& a, const T& b, int cost_a, int cost_b)
{
    return cost_a <= cost_b && a.stats.dominates(b.stats);
}

template <typename T, typename CostFn>
std::vector<const T*> prune_impl(std::span<const T> items, bool include_unowned, CostFn cost)
{
    std::vector<const T*> candidates;
    candidates.reserve(items.size());
    for (const auto& item : items)
    {
        // O toggle "ignorar pictos ainda não obtidos" (M6) entra aqui, e não na
        // busca: um item fora do filtro nem devia gastar comparação.
        if (item.owned || include_unowned)
        {
            candidates.push_back(&item);
        }
    }

    std::vector<const T*> kept;
    kept.reserve(candidates.size());
    for (std::size_t i = 0; i < candidates.size(); ++i)
    {
        const auto* self = candidates[i];
        bool dominated = false;
        for (std::size_t j = 0; j < candidates.size() && !dominated; ++j)
        {
            if (i == j)
            {
                continue;
            }
            const auto* other = candidates[j];
            if (!dominates(*other, *self, cost(*other), cost(*self)))
            {
                continue;
            }
            // Empate exato: quem domina é só o de índice menor, senão duas
            // cópias iguais se descartam mutuamente.
            const bool mutual = dominates(*self, *other, cost(*self), cost(*other));
            dominated = !mutual || j < i;
        }
        if (!dominated)
        {
            kept.push_back(self);
        }
    }
    return kept;
}
} // namespace

std::vector<const Picto*> prune_dominated(std::span<const Picto> pictos, bool include_unowned)
{
    return prune_impl<Picto>(pictos, include_unowned,
                             [](const Picto& p) { return p.lumina_cost; });
}

std::vector<const Lumina*> prune_dominated(std::span<const Lumina> luminas, bool include_unowned)
{
    return prune_impl<Lumina>(luminas, include_unowned, [](const Lumina& l) { return l.cost; });
}
} // namespace e33::opt
