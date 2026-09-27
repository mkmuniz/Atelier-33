#include <doctest/doctest.h>

#include <random>

#include "Optimizer/Prune.hpp"
#include "Optimizer/Search.hpp"

using namespace e33;
using namespace e33::opt;

namespace
{
Picto make_picto(std::string id, double attack, double crit_rate = 0.0, int cost = 0)
{
    Picto p;
    p.id = std::move(id);
    p.name = p.id;
    p.lumina_cost = cost;
    p.stats.attack = attack;
    p.stats.crit_rate = crit_rate;
    return p;
}

Lumina make_lumina(std::string id, double damage_bonus, int cost)
{
    Lumina l;
    l.id = std::move(id);
    l.name = l.id;
    l.cost = cost;
    l.stats.damage_bonus = damage_bonus;
    return l;
}

Request base_request()
{
    Request r;
    r.base_stats.attack = 200.0;
    r.skill.power = 1.0;
    r.skill.id = "skill";
    r.target.defense = 150.0;
    r.picto_slots = 3;
    r.lumina_budget = 0;
    return r;
}

// Referência: enumera tudo, sem poda e sem bound. Lenta de proposito — existe
// para provar que a busca esperta chega no mesmo numero.
double brute_force(const Request& request)
{
    std::vector<const Picto*> pictos;
    for (const auto& p : request.pictos)
    {
        if (p.owned || request.include_unowned)
        {
            pictos.push_back(&p);
        }
    }
    std::vector<const Lumina*> luminas;
    for (const auto& l : request.luminas)
    {
        if (l.owned || request.include_unowned)
        {
            luminas.push_back(&l);
        }
    }

    double best = 0.0;
    const auto slots = static_cast<std::size_t>(std::max(request.picto_slots, 0));
    const auto picto_combos = std::size_t{1} << pictos.size();
    const auto lumina_combos = std::size_t{1} << luminas.size();

    for (std::size_t pm = 0; pm < picto_combos; ++pm)
    {
        std::vector<const Picto*> chosen_p;
        for (std::size_t i = 0; i < pictos.size(); ++i)
        {
            if ((pm & (std::size_t{1} << i)) != 0)
            {
                chosen_p.push_back(pictos[i]);
            }
        }
        if (chosen_p.size() > slots)
        {
            continue;
        }
        for (std::size_t lm = 0; lm < lumina_combos; ++lm)
        {
            std::vector<const Lumina*> chosen_l;
            int cost = 0;
            for (std::size_t i = 0; i < luminas.size(); ++i)
            {
                if ((lm & (std::size_t{1} << i)) != 0)
                {
                    chosen_l.push_back(luminas[i]);
                    cost += luminas[i]->cost;
                }
            }
            if (cost > request.lumina_budget)
            {
                continue;
            }
            best = std::max(best, evaluate(request, chosen_p, chosen_l));
        }
    }
    return best;
}
} // namespace

TEST_CASE("poda descarta o estritamente pior, mantendo o trade-off")
{
    const std::vector<Picto> pictos{
        make_picto("forte", 100.0, 0.10),
        make_picto("fraco", 50.0, 0.05),   // dominado por "forte"
        make_picto("critico", 20.0, 0.30), // troca ataque por critico: fica
    };

    const auto kept = prune_dominated(pictos, false);
    REQUIRE(kept.size() == 2);
    CHECK(kept[0]->id == "forte");
    CHECK(kept[1]->id == "critico");
}

TEST_CASE("custo maior so sobrevive se compensar em stat")
{
    const std::vector<Picto> pictos{
        make_picto("barato", 100.0, 0.0, 2),
        make_picto("caro_igual", 100.0, 0.0, 5), // mesmo stat, custa mais: sai
        make_picto("caro_melhor", 140.0, 0.0, 5),
    };

    const auto kept = prune_dominated(pictos, false);
    REQUIRE(kept.size() == 2);
    CHECK(kept[0]->id == "barato");
    CHECK(kept[1]->id == "caro_melhor");
}

TEST_CASE("itens identicos: sobra exatamente um, nao zero")
{
    const std::vector<Picto> pictos{
        make_picto("copia_a", 100.0),
        make_picto("copia_b", 100.0),
        make_picto("copia_c", 100.0),
    };

    const auto kept = prune_dominated(pictos, false);
    CHECK(kept.size() == 1);
    CHECK(kept.front()->id == "copia_a");
}

TEST_CASE("picto nao obtido fica de fora por padrao")
{
    std::vector<Picto> pictos{make_picto("tenho", 50.0), make_picto("nao_tenho", 500.0)};
    pictos[1].owned = false;

    CHECK(prune_dominated(pictos, false).size() == 1);
    CHECK(prune_dominated(pictos, true).size() == 1); // "nao_tenho" domina "tenho"
    CHECK(prune_dominated(pictos, true).front()->id == "nao_tenho");
}

TEST_CASE("branch and bound chega no mesmo otimo que a forca bruta")
{
    auto request = base_request();
    request.pictos = {
        make_picto("a", 120.0, 0.05), make_picto("b", 90.0, 0.20),
        make_picto("c", 60.0, 0.35),  make_picto("d", 140.0, 0.0),
        make_picto("e", 30.0, 0.45),  make_picto("f", 100.0, 0.12),
    };

    const auto result = search(request, Options{.top_n = 3});
    REQUIRE_FALSE(result.top.empty());
    CHECK(result.exhaustive);
    CHECK(result.top.front().expected_damage == doctest::Approx(brute_force(request)));
}

TEST_CASE("com luminas e orcamento, o otimo continua sendo o otimo")
{
    auto request = base_request();
    request.picto_slots = 2;
    request.lumina_budget = 6;
    request.pictos = {
        make_picto("a", 120.0, 0.05), make_picto("b", 90.0, 0.20),
        make_picto("c", 60.0, 0.35),  make_picto("d", 140.0, 0.0),
    };
    request.luminas = {
        make_lumina("l1", 0.15, 3),
        make_lumina("l2", 0.30, 5),
        make_lumina("l3", 0.10, 2),
        make_lumina("l4", 0.05, 1),
    };

    const auto result = search(request, Options{.top_n = 5});
    REQUIRE_FALSE(result.top.empty());
    CHECK(result.exhaustive);
    CHECK(result.top.front().expected_damage == doctest::Approx(brute_force(request)));
    CHECK(result.top.front().lumina_cost <= request.lumina_budget);
}

TEST_CASE("bound realmente poda: menos avaliacoes que a enumeracao completa")
{
    auto request = base_request();
    for (int i = 0; i < 12; ++i)
    {
        // Trade-off real: mais ataque, menos critico. Sem isso a poda por
        // dominancia resolve tudo e nao sobra ramo para o bound cortar.
        request.pictos.push_back(make_picto("p" + std::to_string(i), 40.0 + i * 11.0,
                                            0.40 - 0.03 * static_cast<double>(i)));
    }

    const auto result = search(request, Options{.top_n = 1});
    CHECK(result.exhaustive);
    CHECK(result.pruned_by_bound > 0);
    // C(12,3) = 220 combinacoes completas; com bound tem de ficar bem abaixo.
    CHECK(result.evaluated < 220);
}

TEST_CASE("resultado respeita o numero de slots")
{
    auto request = base_request();
    request.picto_slots = 2;
    request.pictos = {make_picto("a", 100.0), make_picto("b", 90.0), make_picto("c", 80.0)};

    const auto result = search(request, Options{.top_n = 5});
    for (const auto& candidate : result.top)
    {
        CHECK(candidate.pictos.size() <= 2);
    }
}

TEST_CASE("orcamento zero nao deixa passar nenhuma lumina com custo")
{
    auto request = base_request();
    request.lumina_budget = 0;
    request.pictos = {make_picto("a", 100.0)};
    request.luminas = {make_lumina("cara", 0.5, 3), make_lumina("gratis", 0.1, 0)};

    const auto result = search(request, Options{.top_n = 3});
    for (const auto& candidate : result.top)
    {
        CHECK(candidate.lumina_cost == 0);
        for (const auto* lumina : candidate.luminas)
        {
            CHECK(lumina->cost == 0);
        }
    }
}

TEST_CASE("inventario vazio devolve a build vazia, nao lixo")
{
    const auto request = base_request();
    const auto result = search(request, Options{.top_n = 3});

    REQUIRE(result.top.size() == 1);
    CHECK(result.top.front().pictos.empty());
    CHECK(result.top.front().expected_damage > 0.0);
}

TEST_CASE("top_n vem ordenado do melhor para o pior")
{
    auto request = base_request();
    for (int i = 0; i < 8; ++i)
    {
        request.pictos.push_back(make_picto("p" + std::to_string(i), 30.0 + i * 17.0,
                                            0.35 - 0.04 * static_cast<double>(i)));
    }

    const auto result = search(request, Options{.top_n = 5});
    REQUIRE(result.top.size() == 5);
    for (std::size_t i = 1; i < result.top.size(); ++i)
    {
        CHECK(result.top[i - 1].expected_damage >= result.top[i].expected_damage);
    }
}

TEST_CASE("beam search nunca supera o otimo, e chega perto")
{
    std::mt19937 rng{1234};
    std::uniform_real_distribution<double> attack{20.0, 160.0};
    std::uniform_real_distribution<double> crit{0.0, 0.4};
    // Ataque e critico sorteados de forma independente: gera trade-offs, que e
    // o caso em que beam e exaustivo podem divergir.

    auto request = base_request();
    for (int i = 0; i < 14; ++i)
    {
        request.pictos.push_back(make_picto("p" + std::to_string(i), attack(rng), crit(rng)));
    }

    const auto exact = search(request, Options{.top_n = 1});
    const auto beam = search(request, Options{.top_n = 1, .beam_width = 8});

    REQUIRE(exact.exhaustive);
    REQUIRE_FALSE(beam.exhaustive);
    CHECK(beam.top.front().expected_damage <= exact.top.front().expected_damage
                                                  * (1.0 + 1e-9));
    CHECK(beam.top.front().expected_damage >= exact.top.front().expected_damage * 0.9);
}

TEST_CASE("cancelamento interrompe e marca o resultado como parcial")
{
    auto request = base_request();
    for (int i = 0; i < 20; ++i)
    {
        request.pictos.push_back(
            make_picto("p" + std::to_string(i), 30.0 + i * 7.0, 0.45 - 0.02 * i));
    }

    int calls = 0;
    const auto result = search(request, Options{.top_n = 3}, [&calls](double) {
        ++calls;
        return calls < 2; // cancela no segundo progresso
    });

    CHECK(result.cancelled);
    CHECK_FALSE(result.exhaustive);
}

TEST_CASE("progresso e reportado entre 0 e 1, sem retroceder")
{
    auto request = base_request();
    for (int i = 0; i < 10; ++i)
    {
        request.pictos.push_back(
            make_picto("p" + std::to_string(i), 30.0 + i * 9.0, 0.30 - 0.02 * i));
    }

    double last = -1.0;
    bool monotonic = true;
    const auto result = search(request, Options{.top_n = 1}, [&](double fraction) {
        monotonic = monotonic && fraction >= last && fraction >= 0.0 && fraction <= 1.0;
        last = fraction;
        return true;
    });

    CHECK(monotonic);
    CHECK_FALSE(result.cancelled);
}
