#include <doctest/doctest.h>

#include "Calc/Buffs.hpp"
#include "Calc/Formula.hpp"

using namespace e33;
using namespace e33::calc;

namespace
{
Stats attacker(double attack, double crit_rate = 0.0, double crit_damage = 0.0)
{
    Stats s;
    s.attack = attack;
    s.crit_rate = crit_rate;
    s.crit_damage = crit_damage;
    return s;
}

Skill basic_skill(Element element = Element::None)
{
    Skill s;
    s.id = "skill";
    s.power = 1.0;
    s.element = element;
    return s;
}

Target dummy(double defense = 100.0)
{
    Target t;
    t.defense = defense;
    return t;
}
} // namespace

TEST_CASE("com o expoente padrao o ataque escala linearmente")
{
    const FormulaCoefficients k;
    const auto low = compute_damage(attacker(200.0), basic_skill(), dummy(), k).expected();
    const auto high = compute_damage(attacker(800.0), basic_skill(), dummy(), k).expected();

    CHECK(high == doctest::Approx(4.0 * low));
}

TEST_CASE("o expoente e o unico controle da curva de ataque")
{
    // E este knob que as fixtures do M4 vao ajustar se o jogo se revelar
    // superlinear; o teste existe para garantir que ele de fato controla isso.
    FormulaCoefficients k;
    k.attack_exponent = 1.2;

    const auto low = compute_damage(attacker(200.0), basic_skill(), dummy(), k).expected();
    const auto high = compute_damage(attacker(800.0), basic_skill(), dummy(), k).expected();
    CHECK(high > 4.0 * low);
}

TEST_CASE("mitigacao nao depende do ataque, so da defesa do alvo")
{
    const FormulaCoefficients k;
    const auto soft_low = compute_damage(attacker(200.0), basic_skill(), dummy(50.0), k).expected();
    const auto soft_high = compute_damage(attacker(400.0), basic_skill(), dummy(50.0), k).expected();
    const auto hard_low = compute_damage(attacker(200.0), basic_skill(), dummy(600.0), k).expected();
    const auto hard_high = compute_damage(attacker(400.0), basic_skill(), dummy(600.0), k).expected();

    CHECK(soft_high / soft_low == doctest::Approx(hard_high / hard_low));
}

TEST_CASE("defesa igual a suavidade corta o dano pela metade")
{
    FormulaCoefficients k;
    k.defense_softness = 300.0;

    const auto unmitigated = compute_damage(attacker(400.0), basic_skill(), dummy(0.0), k).expected();
    const auto halved = compute_damage(attacker(400.0), basic_skill(), dummy(300.0), k).expected();
    CHECK(halved == doctest::Approx(unmitigated * 0.5));
}

TEST_CASE("alvo mais blindado recebe menos dano")
{
    const FormulaCoefficients k;
    const auto soft = compute_damage(attacker(400.0), basic_skill(), dummy(50.0), k).expected();
    const auto hard = compute_damage(attacker(400.0), basic_skill(), dummy(500.0), k).expected();
    CHECK(hard < soft);
}

TEST_CASE("alvo sem defesa nao causa divisao por zero")
{
    const FormulaCoefficients k;
    const auto result = compute_damage(attacker(400.0), basic_skill(), dummy(0.0), k);
    CHECK(std::isfinite(result.expected()));
    CHECK(result.expected() > 0.0);
}

TEST_CASE("fraqueza elemental so vale quando o elemento bate")
{
    const FormulaCoefficients k;
    Target target = dummy();
    target.weakness = Element::Fire;
    target.weakness_multiplier = 1.5;

    const auto neutral = compute_damage(attacker(400.0), basic_skill(Element::Ice), target, k);
    const auto exploited = compute_damage(attacker(400.0), basic_skill(Element::Fire), target, k);

    CHECK(neutral.breakdown.weakness == doctest::Approx(1.0));
    CHECK(exploited.breakdown.weakness == doctest::Approx(1.5));
    CHECK(exploited.expected() == doctest::Approx(neutral.expected() * 1.5));
}

TEST_CASE("skill fisica nunca conta como fraqueza, mesmo contra alvo sem fraqueza")
{
    const FormulaCoefficients k;
    Target target = dummy();
    target.weakness = Element::None;

    const auto result = compute_damage(attacker(400.0), basic_skill(Element::None), target, k);
    CHECK(result.breakdown.weakness == doctest::Approx(1.0));
}

TEST_CASE("break multiplica, e compoe com fraqueza")
{
    const FormulaCoefficients k;
    Target target = dummy();
    target.weakness = Element::Fire;
    target.weakness_multiplier = 1.5;
    target.break_multiplier = 2.0;

    const auto normal = compute_damage(attacker(400.0), basic_skill(Element::Fire), target, k);
    target.broken = true;
    const auto broken = compute_damage(attacker(400.0), basic_skill(Element::Fire), target, k);

    CHECK(broken.expected() == doctest::Approx(normal.expected() * 2.0));
    CHECK(broken.breakdown.brk == doctest::Approx(2.0));
    CHECK(broken.breakdown.weakness == doctest::Approx(1.5));
}

TEST_CASE("o produto do breakdown reproduz o dano sem critico")
{
    const FormulaCoefficients k;
    Target target = dummy();
    target.weakness = Element::Fire;
    target.broken = true;

    Stats stats = attacker(400.0);
    stats.damage_bonus = 0.25;
    stats.element_bonus[static_cast<std::size_t>(Element::Fire)] = 0.4;

    auto skill = basic_skill(Element::Fire);
    skill.hits = 3;

    const auto result = compute_damage(stats, skill, target, k);
    const auto& b = result.breakdown;
    const auto product = b.base * b.damage_bonus * b.element_bonus * b.weakness * b.brk * b.hits;

    // O painel mostra os fatores; se eles nao reproduzirem o total, o usuario
    // perde a confianca no numero e o breakdown deixa de servir.
    CHECK(result.minimum() == doctest::Approx(product));
    CHECK(b.damage_bonus == doctest::Approx(1.25));
    CHECK(b.element_bonus == doctest::Approx(1.4));
    CHECK(b.hits == doctest::Approx(3.0));
}

TEST_CASE("dano esperado fica entre o sem critico e o critico")
{
    const FormulaCoefficients k;
    const auto result = compute_damage(attacker(400.0, 0.3, 0.5), basic_skill(), dummy(), k);

    CHECK(result.minimum() < result.expected());
    CHECK(result.expected() < result.maximum());
    CHECK(result.crit_multiplier == doctest::Approx(k.crit_base + 0.5));
}

TEST_CASE("critico em 100% faz o esperado encostar no maximo")
{
    const FormulaCoefficients k;
    const auto result = compute_damage(attacker(400.0, 1.0, 0.5), basic_skill(), dummy(), k);
    CHECK(result.expected() == doctest::Approx(result.maximum()));
}

TEST_CASE("taxa de critico acima de 100% nao vira bonus extra")
{
    const FormulaCoefficients k;
    const auto capped = compute_damage(attacker(400.0, 3.0, 0.5), basic_skill(), dummy(), k);
    const auto exact = compute_damage(attacker(400.0, 1.0, 0.5), basic_skill(), dummy(), k);
    CHECK(capped.expected() == doctest::Approx(exact.expected()));
}

TEST_CASE("skill que nao critica ignora a taxa de critico da build")
{
    const FormulaCoefficients k;
    auto skill = basic_skill();
    skill.can_crit = false;

    const auto result = compute_damage(attacker(400.0, 0.9, 1.0), skill, dummy(), k);
    CHECK(result.expected() == doctest::Approx(result.minimum()));
}

TEST_CASE("coeficientes fazem round trip em JSON")
{
    FormulaCoefficients written;
    written.attack_exponent = 1.2;
    written.crit_base = 1.75;

    FormulaCoefficients read;
    REQUIRE(read.apply_json(written.to_json_string()));
    CHECK(read.attack_exponent == doctest::Approx(1.2));
    CHECK(read.crit_base == doctest::Approx(1.75));
}

TEST_CASE("coeficiente ausente mantem o padrao, e JSON invalido e recusado")
{
    FormulaCoefficients k;
    REQUIRE(k.apply_json(R"({"crit_base": 2.0})"));
    CHECK(k.crit_base == doctest::Approx(2.0));
    CHECK(k.attack_exponent == doctest::Approx(1.0));
    CHECK_FALSE(k.apply_json("{ quebrado"));
}

TEST_CASE("buff empilha ate o teto e nao alem")
{
    Buff rage;
    rage.per_stack.attack = 50.0;
    rage.stacks = 10;
    rage.max_stacks = 3;

    const std::vector<Buff> buffs{rage};
    CHECK(accumulate(buffs).attack == doctest::Approx(150.0));
}

TEST_CASE("buff com zero stacks nao soma nada")
{
    Buff rage;
    rage.per_stack.attack = 50.0;
    rage.stacks = 0;
    rage.max_stacks = 3;

    const std::vector<Buff> buffs{rage};
    CHECK(accumulate(buffs).attack == doctest::Approx(0.0));
}

TEST_CASE("total_stats soma base, pictos, luminas e buffs")
{
    Stats base;
    base.attack = 100.0;

    Picto picto;
    picto.stats.attack = 30.0;
    picto.stats.crit_rate = 0.1;

    Lumina lumina;
    lumina.stats.damage_bonus = 0.2;

    Buff buff;
    buff.per_stack.attack = 10.0;
    buff.stacks = 2;
    buff.max_stacks = 2;

    const std::vector<const Picto*> pictos{&picto};
    const std::vector<const Lumina*> luminas{&lumina};
    const std::vector<Buff> buffs{buff};

    const auto total = total_stats(base, pictos, luminas, buffs);
    CHECK(total.attack == doctest::Approx(150.0));
    CHECK(total.crit_rate == doctest::Approx(0.1));
    CHECK(total.damage_bonus == doctest::Approx(0.2));
}

TEST_CASE("slot vazio nao quebra a soma")
{
    Stats base;
    base.attack = 100.0;
    const std::vector<const Picto*> pictos{nullptr, nullptr};
    const std::vector<const Lumina*> luminas{};
    const std::vector<Buff> buffs{};

    CHECK(total_stats(base, pictos, luminas, buffs).attack == doctest::Approx(100.0));
}
