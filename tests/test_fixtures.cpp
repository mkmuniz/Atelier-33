#include <doctest/doctest.h>

#include "Calc/Fixtures.hpp"
#include "Support/Json.hpp"
#include "TempDir.hpp"

using namespace e33;
using namespace e33::calc;

namespace
{
GameData tiny_data(const test::TempDir& dir)
{
    const std::string base = "data/1.5.0/";
    REQUIRE(write_text_file_atomic(dir.file(base + "pictos.json"),
                                   R"({"pictos": [{"id": "p1", "name": "P1", "stats": {"attack": 100}}]})"));
    REQUIRE(write_text_file_atomic(dir.file(base + "luminas.json"), R"({"luminas": []})"));
    REQUIRE(write_text_file_atomic(dir.file(base + "weapons.json"), R"({"weapons": []})"));
    REQUIRE(write_text_file_atomic(dir.file(base + "skills.json"),
                                   R"({"skills": [{"id": "sk", "name": "Sk", "power": 1.0}]})"));
    REQUIRE(write_text_file_atomic(dir.file(base + "enemies.json"),
                                   R"({"enemies": [{"id": "en", "name": "En", "defense": 300}]})"));

    GameData data;
    data.load(dir.file(base));
    return data;
}
} // namespace

TEST_CASE("diretorio sem fixtures nao e erro: o M4 ainda nao aconteceu")
{
    const test::TempDir dir;
    CHECK(load_fixtures(dir.file("fixtures")).empty());
}

TEST_CASE("fixture sem dano observado e descartada em vez de contar como acerto")
{
    const test::TempDir dir;
    REQUIRE(write_text_file_atomic(dir.file("fx/a.json"),
                                   R"({"id": "a", "skill": "sk", "observed_damage": 0})"));
    CHECK(load_fixtures(dir.file("fx")).empty());
}

TEST_CASE("erro relativo e calculado contra o dano medido")
{
    const test::TempDir dir;
    const auto data = tiny_data(dir);

    // 400 de ataque, sem picto: base = 400 * 300/(300+300) = 200.
    REQUIRE(write_text_file_atomic(dir.file("fx/exato.json"), R"({
        "id": "exato", "skill": "sk", "enemy": "en",
        "base_stats": {"attack": 400}, "observed_damage": 200
    })"));

    const auto report = evaluate_fixtures(load_fixtures(dir.file("fx")), data,
                                          FormulaCoefficients{});
    REQUIRE(report.count() == 1);
    CHECK(report.mean_absolute_relative_error() == doctest::Approx(0.0).epsilon(1e-6));
}

TEST_CASE("erro aparece quando a previsao diverge da medicao")
{
    const test::TempDir dir;
    const auto data = tiny_data(dir);
    REQUIRE(write_text_file_atomic(dir.file("fx/off.json"), R"({
        "id": "off", "skill": "sk", "enemy": "en",
        "base_stats": {"attack": 400}, "observed_damage": 250
    })"));

    const auto report = evaluate_fixtures(load_fixtures(dir.file("fx")), data,
                                          FormulaCoefficients{});
    REQUIRE(report.count() == 1);
    // previsto 200 contra 250 medido => 20% de erro
    CHECK(report.mean_absolute_relative_error() == doctest::Approx(0.2));
    CHECK(report.worst_absolute_relative_error() == doctest::Approx(0.2));
}

TEST_CASE("fixture que cita id inexistente e pulada, nao medida contra outra build")
{
    const test::TempDir dir;
    const auto data = tiny_data(dir);
    REQUIRE(write_text_file_atomic(dir.file("fx/ghost.json"), R"({
        "id": "ghost", "skill": "sk", "pictos": ["nao_existe"],
        "base_stats": {"attack": 400}, "observed_damage": 200
    })"));

    const auto report = evaluate_fixtures(load_fixtures(dir.file("fx")), data,
                                          FormulaCoefficients{});
    CHECK(report.count() == 0);
    REQUIRE(report.skipped.size() == 1);
    CHECK(report.skipped.front() == "ghost");
}

TEST_CASE("o tipo da medicao escolhe qual numero comparar")
{
    const test::TempDir dir;
    const auto data = tiny_data(dir);
    REQUIRE(write_text_file_atomic(dir.file("fx/c.json"), R"({
        "id": "c", "skill": "sk", "enemy": "en", "kind": "crit",
        "base_stats": {"attack": 400, "crit_rate": 0.5}, "observed_damage": 300
    })"));

    const auto report = evaluate_fixtures(load_fixtures(dir.file("fx")), data,
                                          FormulaCoefficients{});
    REQUIRE(report.count() == 1);
    // crit = 200 * 1.5 = 300 => erro zero
    CHECK(report.mean_absolute_relative_error() == doctest::Approx(0.0).epsilon(1e-6));
}

TEST_CASE("as fixtures reais do repositorio, quando existirem, ficam abaixo de 2%")
{
    // Este e o teste que o M4 fecha. Enquanto tests/fixtures/ so tiver o README,
    // ele passa sem medir nada — e o README do projeto diz isso em vez de
    // anunciar uma precisao que nao foi verificada.
    const auto fixtures = load_fixtures("tests/fixtures");
    if (fixtures.empty())
    {
        MESSAGE("sem fixtures medidas ainda: M4 pendente");
        return;
    }

    GameData data;
    data.load("data/1.5.0");
    FormulaCoefficients coefficients;
    coefficients.load("data/1.5.0/formula.json");

    const auto report = evaluate_fixtures(fixtures, data, coefficients);
    MESSAGE("fixtures: ", report.count(), " medidas, erro medio ",
            report.mean_absolute_relative_error() * 100.0, "%");
    CHECK(report.mean_absolute_relative_error() < 0.02);
}
