#include <doctest/doctest.h>

#include <memory>
#include <thread>

#include "Core/AppController.hpp"
#include "Support/Json.hpp"
#include "TempDir.hpp"

using namespace e33;

namespace
{
void write_data(const test::TempDir& dir, std::string_view version = "1.5.0")
{
    const auto base = std::string{"data/"} + std::string{version} + "/";
    REQUIRE(write_text_file_atomic(dir.file(base + "pictos.json"), R"({"pictos": [
        {"id": "p_atk", "name": "Lame", "lumina_cost": 3, "stats": {"attack": 100}},
        {"id": "p_crit", "name": "Oeil", "lumina_cost": 4, "stats": {"crit_rate": 0.3}},
        {"id": "p_mix", "name": "Coeur", "lumina_cost": 5,
         "stats": {"attack": 60, "crit_rate": 0.15}},
        {"id": "p_fogo", "name": "Flamme", "lumina_cost": 2,
         "stats": {"element_bonus": {"fire": 0.5}}},
        {"id": "p_lixo", "stats": {}}
    ]})"));
    REQUIRE(write_text_file_atomic(dir.file(base + "luminas.json"), R"({"luminas": [
        {"id": "l_dmg", "name": "Force", "cost": 4, "stats": {"damage_bonus": 0.25}},
        {"id": "l_crit", "name": "Precision", "cost": 3, "stats": {"crit_damage": 0.4}}
    ]})"));
    REQUIRE(write_text_file_atomic(dir.file(base + "weapons.json"), R"({"weapons": [
        {"id": "w1", "name": "Epee", "stats": {"attack": 250}, "element": "fire"}
    ]})"));
    REQUIRE(write_text_file_atomic(dir.file(base + "skills.json"), R"({"skills": [
        {"id": "sk_fire", "name": "Brasier", "power": 1.8, "element": "fire", "hits": 2},
        {"id": "sk_hit", "name": "Coup", "power": 1.0}
    ]})"));
    REQUIRE(write_text_file_atomic(dir.file(base + "enemies.json"), R"({"enemies": [
        {"id": "e_sirene", "name": "Sirène", "defense": 300, "weakness": "fire",
         "weakness_multiplier": 1.6, "break_multiplier": 2.0}
    ]})"));
}

PartySnapshot sample_party()
{
    CharacterSnapshot maelle;
    maelle.id = "maelle";
    maelle.name = "Maelle";
    maelle.base_stats.attack = 300.0;
    maelle.weapon_id = "w1";
    maelle.equipped_picto_ids = {"p_atk"};
    maelle.active_lumina_ids = {"l_dmg"};
    maelle.picto_slots = 3;
    maelle.lumina_budget = 8;

    PartySnapshot party;
    party.characters = {maelle};
    party.owned_picto_ids = {"p_atk", "p_crit", "p_mix", "p_fogo"};
    party.owned_lumina_ids = {"l_dmg", "l_crit"};
    party.valid = true;
    return party;
}

struct Fixture
{
    explicit Fixture(const test::TempDir& dir, PartySnapshot party = sample_party())
        : source{new MockPartySource{std::move(party)}}
        , app{std::unique_ptr<IPartySource>{source}}
    {
        app.initialize(dir.path());
        app.tick(0.0);
    }

    MockPartySource* source;
    AppController app;
};

void wait_for_job(AppController& app)
{
    for (int i = 0; i < 4000 && app.job().running(); ++i)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds{2});
    }
    app.tick(100.0);
}
} // namespace

TEST_CASE("dados carregam e a linha torta e descartada sem derrubar a tabela")
{
    const test::TempDir dir;
    write_data(dir);
    Fixture f{dir};

    CHECK(f.app.load_report().ok());
    CHECK(f.app.data().pictos().size() == 5); // inclui o "p_lixo", que tem id
    CHECK(f.app.data().find_picto("p_atk") != nullptr);
    CHECK(f.app.data().find_picto("nao_existe") == nullptr);
}

TEST_CASE("arquivo ausente e reportado como erro, nao ignorado em silencio")
{
    const test::TempDir dir;
    // Sem nenhum data/: otimizar sem tabela nao e degradacao, e resultado errado.
    Fixture f{dir};

    CHECK_FALSE(f.app.load_report().ok());
    CHECK(f.app.load_report().errors.size() == 5);
    CHECK(f.app.status_line().find("dados incompletos") != std::string::npos);
}

TEST_CASE("element_bonus vem nomeado por elemento, nao por indice")
{
    const test::TempDir dir;
    write_data(dir);
    Fixture f{dir};

    const auto* fogo = f.app.data().find_picto("p_fogo");
    REQUIRE(fogo != nullptr);
    CHECK(fogo->stats.bonus_for(Element::Fire) == doctest::Approx(0.5));
    CHECK(fogo->stats.bonus_for(Element::Ice) == doctest::Approx(0.0));
}

TEST_CASE("dano atual usa base, arma, pictos equipados e luminas ativas")
{
    const test::TempDir dir;
    write_data(dir);
    Fixture f{dir};
    f.app.select_skill("sk_fire");
    f.app.select_enemy("e_sirene");

    const auto damage = f.app.current_damage();
    REQUIRE(damage.has_value());
    // 300 (base) + 250 (arma) + 100 (picto) = 650 de ataque
    CHECK(damage->breakdown.damage_bonus == doctest::Approx(1.25)); // lumina
    CHECK(damage->breakdown.weakness == doctest::Approx(1.6));      // fogo x fogo
    CHECK(damage->breakdown.hits == doctest::Approx(2.0));
    CHECK(damage->expected() > 0.0);
}

TEST_CASE("break entra no calculo quando marcado")
{
    const test::TempDir dir;
    write_data(dir);
    Fixture f{dir};
    f.app.select_skill("sk_fire");
    f.app.select_enemy("e_sirene");

    const auto normal = f.app.current_damage()->expected();
    f.app.set_target_broken(true);
    const auto broken = f.app.current_damage()->expected();
    CHECK(broken == doctest::Approx(normal * 2.0));
}

TEST_CASE("sem party valida nao ha dano nem requisicao, e o erro aparece")
{
    const test::TempDir dir;
    write_data(dir);

    PartySnapshot empty;
    empty.valid = false;
    empty.error = "leitura do estado do jogo ainda nao implementada (M0/M3)";
    Fixture f{dir, empty};

    CHECK_FALSE(f.app.current_damage().has_value());
    CHECK_FALSE(f.app.build_request().has_value());
    CHECK(f.app.status_line() == empty.error);
}

TEST_CASE("a busca so considera o que o jogador tem, por padrao")
{
    const test::TempDir dir;
    write_data(dir);

    auto party = sample_party();
    party.owned_picto_ids = {"p_atk"}; // so um picto no inventario
    Fixture f{dir, party};
    f.app.select_skill("sk_fire");

    const auto request = f.app.build_request();
    REQUIRE(request.has_value());
    CHECK_FALSE(request->include_unowned);

    std::size_t owned = 0;
    for (const auto& picto : request->pictos)
    {
        owned += picto.owned ? 1 : 0;
    }
    CHECK(owned == 1);
}

TEST_CASE("toggle de nao obtidos muda a requisicao e e persistido")
{
    const test::TempDir dir;
    write_data(dir);
    Fixture f{dir};

    f.app.set_include_unowned(true);
    CHECK(f.app.build_request()->include_unowned);
    CHECK(std::filesystem::exists(dir.file("settings.json")));
}

TEST_CASE("otimizar roda em thread e produz sugestao melhor ou igual a atual")
{
    const test::TempDir dir;
    write_data(dir);
    Fixture f{dir};
    f.app.select_skill("sk_fire");
    f.app.select_enemy("e_sirene");

    const auto before = f.app.current_damage()->expected();
    f.app.start_optimization();
    wait_for_job(f.app);

    REQUIRE(f.app.last_result().has_value());
    REQUIRE_FALSE(f.app.last_result()->top.empty());
    // A build atual esta no espaco de busca, entao a sugerida nunca pode ser pior.
    CHECK(f.app.last_result()->top.front().expected_damage >= before * (1.0 - 1e-9));
}

TEST_CASE("o diff diz exatamente o que trocar")
{
    const test::TempDir dir;
    write_data(dir);
    Fixture f{dir};
    f.app.select_skill("sk_fire");
    f.app.select_enemy("e_sirene");
    f.app.start_optimization();
    wait_for_job(f.app);

    REQUIRE(f.app.last_result().has_value());
    const auto diff = f.app.diff_against_current(f.app.last_result()->top.front());

    CHECK(diff.suggested_damage >= diff.current_damage);
    CHECK(diff.delta() >= 0.0);
    // Nada em add pode estar tambem em remove.
    for (const auto* added : diff.add_pictos)
    {
        for (const auto* removed : diff.remove_pictos)
        {
            CHECK(added->id != removed->id);
        }
    }
}

TEST_CASE("diff contra a propria build atual nao sugere troca nenhuma")
{
    const test::TempDir dir;
    write_data(dir);
    Fixture f{dir};
    f.app.select_skill("sk_fire");

    opt::Candidate same;
    same.pictos = {f.app.data().find_picto("p_atk")};
    same.luminas = {f.app.data().find_lumina("l_dmg")};
    same.expected_damage = f.app.current_damage()->expected();

    const auto diff = f.app.diff_against_current(same);
    CHECK(diff.empty());
    CHECK(diff.delta() == doctest::Approx(0.0));
}

TEST_CASE("texto exportado tem personagem, itens e o dano")
{
    const test::TempDir dir;
    write_data(dir);
    Fixture f{dir};
    f.app.select_skill("sk_fire");

    const auto text = f.app.export_build_text();
    CHECK(text.find("Maelle") != std::string::npos);
    CHECK(text.find("Lame") != std::string::npos);   // picto equipado
    CHECK(text.find("Force") != std::string::npos);  // lumina ativa
    CHECK(text.find("Brasier") != std::string::npos);
}

TEST_CASE("o overlay reflete a troca de picto no menu, sem pedir")
{
    const test::TempDir dir;
    write_data(dir);
    Fixture f{dir};
    f.app.select_skill("sk_fire");

    const auto before = f.app.current_damage()->expected();

    auto party = sample_party();
    party.characters[0].equipped_picto_ids = {"p_atk", "p_mix"};
    f.source->set(party);

    f.app.tick(0.1); // antes do poll
    CHECK(f.app.current_damage()->expected() == doctest::Approx(before));

    f.app.tick(AppController::kPartyPollSeconds + 0.2);
    CHECK(f.app.current_damage()->expected() > before);
}

TEST_CASE("selecao de personagem satura no tamanho da party")
{
    const test::TempDir dir;
    write_data(dir);
    Fixture f{dir};

    f.app.select_character(99);
    CHECK(f.app.selected_character() == 0);
    CHECK(f.app.character() != nullptr);
}
