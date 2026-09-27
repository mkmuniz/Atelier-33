#include "Optimizer/Search.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

#include "Optimizer/Prune.hpp"

namespace e33::opt
{
namespace
{
// Coleta os N melhores sem ordenar o espaço inteiro.
class TopN
{
public:
    explicit TopN(std::size_t limit) : m_limit{std::max<std::size_t>(limit, 1)} {}

    void offer(Candidate candidate)
    {
        if (m_items.size() < m_limit)
        {
            m_items.push_back(std::move(candidate));
            std::ranges::sort(m_items, std::greater{}, &Candidate::expected_damage);
            return;
        }
        if (candidate.expected_damage <= m_items.back().expected_damage)
        {
            return;
        }
        m_items.back() = std::move(candidate);
        std::ranges::sort(m_items, std::greater{}, &Candidate::expected_damage);
    }

    [[nodiscard]] double worst() const
    {
        return m_items.size() < m_limit ? -std::numeric_limits<double>::infinity()
                                        : m_items.back().expected_damage;
    }
    [[nodiscard]] std::vector<Candidate> take() { return std::move(m_items); }

private:
    std::size_t m_limit;
    std::vector<Candidate> m_items{};
};

// Stats otimistas: o melhor valor de cada campo, independentemente, entre os
// itens que ainda podem ser escolhidos, multiplicado pelas escolhas restantes.
//
// Isso é um limite superior legítimo porque o dano é monotônico crescente em
// todo stat que ele usa — ataque, bônus de dano, afinidade, taxa e dano de
// crítico. Se algum dia um stat passar a poder reduzir dano, este bound deixa
// de valer e o branch and bound para de ser exato.
template <typename T>
Stats optimistic_remainder(std::span<const T* const> pool, std::size_t from, int picks)
{
    Stats best;
    if (picks <= 0)
    {
        return best;
    }
    for (std::size_t i = from; i < pool.size(); ++i)
    {
        const auto& s = pool[i]->stats;
        best.attack = std::max(best.attack, s.attack);
        best.crit_rate = std::max(best.crit_rate, s.crit_rate);
        best.crit_damage = std::max(best.crit_damage, s.crit_damage);
        best.damage_bonus = std::max(best.damage_bonus, s.damage_bonus);
        for (std::size_t e = 0; e < best.element_bonus.size(); ++e)
        {
            best.element_bonus[e] = std::max(best.element_bonus[e], s.element_bonus[e]);
        }
    }
    Stats total;
    for (int i = 0; i < picks; ++i)
    {
        total += best;
    }
    return total;
}

// Teto de quantos itens ainda cabem no orçamento a partir de `from`. Um item de
// custo zero torna o orçamento irrelevante, e aí o teto é o tamanho do resto.
int max_affordable_picks(std::span<const Lumina* const> pool, std::size_t from, int budget)
{
    if (budget < 0 || from >= pool.size())
    {
        return 0;
    }
    int cheapest = std::numeric_limits<int>::max();
    for (std::size_t i = from; i < pool.size(); ++i)
    {
        cheapest = std::min(cheapest, std::max(pool[i]->cost, 0));
    }
    const auto remaining = static_cast<int>(pool.size() - from);
    if (cheapest <= 0)
    {
        return remaining;
    }
    return std::min(remaining, budget / cheapest);
}

struct Context
{
    const Request* request{};
    Stats base{};  // base + buffs, já somados uma vez só
    std::vector<const Picto*> picto_pool{};
    std::vector<const Lumina*> lumina_pool{};
    TopN top;
    // Melhor contribuição que as luminas ainda podem dar, com o orçamento
    // inteiro. A fase dos pictos precisa somar isto ao seu bound: os valores já
    // encontrados incluem luminas, e comparar um bound sem elas contra eles
    // poda ramos que ainda venceriam.
    Stats lumina_ceiling{};
    Result result{};
    const ProgressFn* progress{};
    bool cancelled{false};

    [[nodiscard]] double damage_of(const Stats& stats) const
    {
        return calc::compute_damage(stats, request->skill, request->target,
                                    request->coefficients)
            .expected();
    }
};

// Fase 2: escolhe luminas dentro do orçamento, sobre um conjunto de pictos já
// fixado.
void search_luminas(Context& ctx, const std::vector<const Picto*>& pictos, const Stats& stats,
                    std::vector<const Lumina*>& chosen, int spent, std::size_t start)
{
    if (ctx.cancelled)
    {
        return;
    }

    ++ctx.result.evaluated;
    const auto damage = ctx.damage_of(stats);
    ctx.top.offer(Candidate{.pictos = pictos,
                            .luminas = chosen,
                            .expected_damage = damage,
                            .lumina_cost = spent});

    for (std::size_t i = start; i < ctx.lumina_pool.size(); ++i)
    {
        const auto* lumina = ctx.lumina_pool[i];
        if (spent + lumina->cost > ctx.request->lumina_budget)
        {
            continue;
        }
        // Quantas luminas ainda cabem no orçamento, no melhor caso: o teto é
        // o que sobra dividido pela mais barata que resta. Supor uma só aqui
        // deixa de ser limite superior e a poda passa a descartar o ótimo —
        // foi exatamente o que aconteceu antes deste cálculo existir.
        const auto picks = max_affordable_picks(ctx.lumina_pool, i,
                                                ctx.request->lumina_budget - spent);
        const auto bound_stats = stats
                                 + optimistic_remainder<Lumina>(ctx.lumina_pool, i, picks);
        if (ctx.damage_of(bound_stats) <= ctx.top.worst())
        {
            ++ctx.result.pruned_by_bound;
            continue;
        }

        chosen.push_back(lumina);
        search_luminas(ctx, pictos, stats + lumina->stats, chosen, spent + lumina->cost, i + 1);
        chosen.pop_back();
    }
}

// Fase 1: escolhe os pictos dos slots.
void search_pictos(Context& ctx, std::vector<const Picto*>& chosen, const Stats& stats,
                   std::size_t start, int remaining_slots)
{
    if (ctx.cancelled)
    {
        return;
    }

    // Todo nó é uma build válida, não só as folhas: deixar um slot vazio pode
    // ser melhor que qualquer picto restante, e uma busca que só avalia
    // conjuntos cheios simplesmente não enxerga essa resposta.
    {
        std::vector<const Lumina*> luminas;
        search_luminas(ctx, chosen, stats, luminas, 0, 0);
    }

    if (remaining_slots == 0 || start >= ctx.picto_pool.size())
    {
        return;
    }

    for (std::size_t i = start; i < ctx.picto_pool.size(); ++i)
    {
        if (ctx.cancelled)
        {
            return;
        }
        if (chosen.empty() && ctx.progress && *ctx.progress)
        {
            const auto fraction = static_cast<double>(i)
                                  / static_cast<double>(std::max<std::size_t>(
                                      ctx.picto_pool.size(), 1));
            if (!(*ctx.progress)(fraction))
            {
                ctx.cancelled = true;
                return;
            }
        }

        const auto bound_stats = stats
                                 + optimistic_remainder<Picto>(ctx.picto_pool, i,
                                                               remaining_slots)
                                 + ctx.lumina_ceiling;
        if (ctx.damage_of(bound_stats) <= ctx.top.worst())
        {
            // Nenhuma escolha a partir daqui alcança o pior dos melhores já
            // encontrados: o ramo inteiro morre sem ser enumerado.
            ++ctx.result.pruned_by_bound;
            continue;
        }

        chosen.push_back(ctx.picto_pool[i]);
        search_pictos(ctx, chosen, stats + ctx.picto_pool[i]->stats, i + 1, remaining_slots - 1);
        chosen.pop_back();
    }
}

// Beam search: mantém as melhores `width` combinações parciais a cada nível,
// em vez de descer em todas. Não prova otimalidade, mas termina.
void search_beam(Context& ctx, std::size_t width)
{
    struct Partial
    {
        std::vector<const Picto*> pictos{};
        Stats stats{};
        std::size_t next_index{0};
        double score{0.0};
    };

    std::vector<Partial> beam{Partial{.stats = ctx.base}};
    const auto slots = static_cast<std::size_t>(std::max(ctx.request->picto_slots, 0));

    for (std::size_t depth = 0; depth < slots; ++depth)
    {
        if (ctx.cancelled)
        {
            return;
        }
        if (ctx.progress && *ctx.progress)
        {
            const auto fraction = static_cast<double>(depth) / static_cast<double>(slots);
            if (!(*ctx.progress)(fraction))
            {
                ctx.cancelled = true;
                return;
            }
        }

        std::vector<Partial> next;
        for (const auto& partial : beam)
        {
            for (std::size_t i = partial.next_index; i < ctx.picto_pool.size(); ++i)
            {
                Partial child{.pictos = partial.pictos,
                              .stats = partial.stats + ctx.picto_pool[i]->stats,
                              .next_index = i + 1,
                              .score = 0.0};
                child.pictos.push_back(ctx.picto_pool[i]);
                child.score = ctx.damage_of(child.stats);
                next.push_back(std::move(child));
            }
        }
        if (next.empty())
        {
            break;
        }
        std::ranges::sort(next, std::greater{}, &Partial::score);
        if (next.size() > width)
        {
            next.resize(width);
        }
        beam = std::move(next);
    }

    for (const auto& partial : beam)
    {
        std::vector<const Lumina*> luminas;
        search_luminas(ctx, partial.pictos, partial.stats, luminas, 0, 0);
    }
}

std::size_t estimate_nodes(std::size_t pictos, int slots, std::size_t luminas)
{
    // Combinações de pictos vezes subconjuntos de lumina. Só a ordem de
    // grandeza importa: serve para decidir entre exaustivo e beam.
    double combinations = 1.0;
    for (int i = 0; i < slots && static_cast<std::size_t>(i) < pictos; ++i)
    {
        combinations *= static_cast<double>(pictos - static_cast<std::size_t>(i))
                        / static_cast<double>(i + 1);
    }
    const auto subsets = std::pow(2.0, static_cast<double>(std::min<std::size_t>(luminas, 20)));
    const auto total = combinations * subsets;
    return total >= static_cast<double>(std::numeric_limits<std::size_t>::max())
               ? std::numeric_limits<std::size_t>::max()
               : static_cast<std::size_t>(total);
}
} // namespace

double evaluate(const Request& request, std::span<const Picto* const> pictos,
                std::span<const Lumina* const> luminas)
{
    const auto stats = calc::total_stats(request.base_stats, pictos, luminas, request.buffs);
    return calc::compute_damage(stats, request.skill, request.target, request.coefficients)
        .expected();
}

Result search(const Request& request, const Options& options, const ProgressFn& progress)
{
    Context ctx{.request = &request, .top = TopN{options.top_n}};
    ctx.progress = &progress;

    ctx.base = request.base_stats;
    ctx.base += calc::accumulate(request.buffs);

    ctx.picto_pool = prune_dominated(request.pictos, request.include_unowned);
    ctx.lumina_pool = prune_dominated(request.luminas, request.include_unowned);
    ctx.result.candidates_after_dominance = ctx.picto_pool.size() + ctx.lumina_pool.size();
    ctx.lumina_ceiling = optimistic_remainder<Lumina>(
        ctx.lumina_pool, 0,
        max_affordable_picks(ctx.lumina_pool, 0, request.lumina_budget));

    auto width = options.beam_width;
    if (width == 0
        && estimate_nodes(ctx.picto_pool.size(), request.picto_slots, ctx.lumina_pool.size())
               > options.auto_beam_threshold)
    {
        // Vira beam sozinha em vez de rodar por minutos: o plano exige que uma
        // busca típica termine em menos de 3 segundos.
        width = 64;
    }

    if (width > 0)
    {
        search_beam(ctx, width);
        ctx.result.exhaustive = false;
    }
    else
    {
        std::vector<const Picto*> chosen;
        search_pictos(ctx, chosen, ctx.base, 0, request.picto_slots);
        ctx.result.exhaustive = !ctx.cancelled;
    }

    ctx.result.cancelled = ctx.cancelled;
    ctx.result.top = ctx.top.take();
    return ctx.result;
}
} // namespace e33::opt
