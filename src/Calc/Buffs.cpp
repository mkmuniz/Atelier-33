#include "Calc/Buffs.hpp"

#include <algorithm>

namespace e33::calc
{
Stats accumulate(std::span<const Buff> buffs)
{
    Stats total;
    for (const auto& buff : buffs)
    {
        const auto stacks = std::clamp(buff.stacks, 0, std::max(buff.max_stacks, 0));
        for (int i = 0; i < stacks; ++i)
        {
            total += buff.per_stack;
        }
    }
    return total;
}

Stats total_stats(const Stats& base, std::span<const Picto* const> pictos,
                  std::span<const Lumina* const> luminas, std::span<const Buff> buffs)
{
    Stats total = base;
    for (const auto* picto : pictos)
    {
        if (picto != nullptr)
        {
            total += picto->stats;
        }
    }
    for (const auto* lumina : luminas)
    {
        if (lumina != nullptr)
        {
            total += lumina->stats;
        }
    }
    total += accumulate(buffs);
    return total;
}
} // namespace e33::calc
