// Mede o tempo da busca. O plano fixa o alvo: uma busca típica tem de terminar
// em menos de 3 segundos. Este número é medido no macOS; o que vale para o
// usuário é o do PC, e é lá que ele deve ser refeito antes de publicar.
//
//   xmake build bench && xmake run bench

#include <chrono>
#include <cstdio>
#include <random>
#include <string>

#include "Optimizer/Search.hpp"

using namespace e33;
using namespace e33::opt;

namespace
{
Request make_request(int picto_count, int lumina_count, int budget, unsigned seed)
{
    std::mt19937 rng{seed};
    std::uniform_real_distribution<double> attack{10.0, 180.0};
    std::uniform_real_distribution<double> crit{0.0, 0.35};
    std::uniform_real_distribution<double> bonus{0.02, 0.30};
    std::uniform_int_distribution<int> cost{1, 8};

    Request r;
    r.base_stats.attack = 400.0;
    r.skill.power = 1.6;
    r.skill.element = Element::Fire;
    r.target.defense = 320.0;
    r.target.weakness = Element::Fire;
    r.picto_slots = 3;
    r.lumina_budget = budget;

    // Pictos com stats em muitas dimensões de propósito. Com só três campos
    // aleatórios a poda por dominância derruba 120 itens para ~14 e o bench
    // mede uma busca que não existe: a fronteira de Pareto cresce rápido com o
    // número de stats, e é essa fronteira que a busca enfrenta de verdade.
    std::uniform_int_distribution<int> element{1, static_cast<int>(Element::Count) - 1};
    for (int i = 0; i < picto_count; ++i)
    {
        Picto p;
        p.id = "picto_" + std::to_string(i);
        p.name = p.id;
        p.stats.attack = attack(rng);
        p.stats.crit_rate = crit(rng);
        p.stats.crit_damage = crit(rng);
        p.stats.damage_bonus = bonus(rng);
        p.stats.element_bonus[static_cast<std::size_t>(element(rng))] = bonus(rng);
        p.stats.element_bonus[static_cast<std::size_t>(element(rng))] = bonus(rng);
        r.pictos.push_back(std::move(p));
    }
    for (int i = 0; i < lumina_count; ++i)
    {
        Lumina l;
        l.id = "lumina_" + std::to_string(i);
        l.name = l.id;
        l.cost = cost(rng);
        l.stats.damage_bonus = bonus(rng);
        l.stats.crit_rate = crit(rng) * 0.5;
        l.stats.element_bonus[static_cast<std::size_t>(element(rng))] = bonus(rng);
        r.luminas.push_back(std::move(l));
    }
    return r;
}

void run(const char* label, const Request& request, const Options& options)
{
    const auto started = std::chrono::steady_clock::now();
    const auto result = search(request, options);
    const std::chrono::duration<double> elapsed = std::chrono::steady_clock::now() - started;

    // Distinguir "provado ótimo" de "melhor achado dentro do teto" importa: o
    // painel diz ao usuário qual dos dois ele está lendo.
    const char* status = "[otimo provado]";
    if (result.node_limit_hit)
    {
        status = "[parou no teto de nos]";
    }
    else if (!result.exhaustive)
    {
        status = "[beam]";
    }

    std::printf("%-34s %7.3fs  avaliadas=%-9zu podadas=%-9zu pool=%-4zu %s\n", label,
                elapsed.count(), result.evaluated, result.pruned_by_bound,
                result.candidates_after_dominance, status);
}
} // namespace

int main()
{
    std::printf("Alvo do plano: busca tipica < 3s\n\n");

    run("inventario pequeno (20p/10l)", make_request(20, 10, 12, 1), Options{.top_n = 5});
    run("inventario tipico (60p/30l)", make_request(60, 30, 20, 2), Options{.top_n = 5});
    run("inventario grande (120p/60l)", make_request(120, 60, 30, 3), Options{.top_n = 5});
    run("grande, beam forcada", make_request(120, 60, 30, 3),
        Options{.top_n = 5, .beam_width = 64});

    return 0;
}
