#include "Calc/Fixtures.hpp"

#include <algorithm>
#include <cmath>
#include <numeric>

#include "Calc/Buffs.hpp"
#include "Support/Json.hpp"
#include "Support/Log.hpp"

namespace e33::calc
{
namespace
{
Fixture::Kind kind_from_name(std::string_view name)
{
    if (name == "crit")
    {
        return Fixture::Kind::Crit;
    }
    if (name == "expected")
    {
        return Fixture::Kind::Expected;
    }
    return Fixture::Kind::NonCrit;
}

std::vector<std::string> string_array(const Json& obj, std::string_view key)
{
    std::vector<std::string> out;
    const auto it = obj.find(key);
    if (it == obj.end() || !it->is_array())
    {
        return out;
    }
    for (const auto& item : *it)
    {
        if (item.is_string())
        {
            out.push_back(item.get<std::string>());
        }
    }
    return out;
}
} // namespace

double FixtureError::relative() const
{
    if (observed == 0.0)
    {
        return 0.0;
    }
    return (predicted - observed) / observed;
}

double FixtureReport::mean_absolute_relative_error() const
{
    if (errors.empty())
    {
        return 0.0;
    }
    const auto sum = std::accumulate(errors.begin(), errors.end(), 0.0,
                                     [](double acc, const FixtureError& e) {
                                         return acc + std::abs(e.relative());
                                     });
    return sum / static_cast<double>(errors.size());
}

double FixtureReport::worst_absolute_relative_error() const
{
    double worst = 0.0;
    for (const auto& error : errors)
    {
        worst = std::max(worst, std::abs(error.relative()));
    }
    return worst;
}

std::vector<Fixture> load_fixtures(const std::filesystem::path& dir)
{
    std::vector<Fixture> fixtures;
    std::error_code ec;
    if (!std::filesystem::is_directory(dir, ec))
    {
        return fixtures;
    }

    for (const auto& entry : std::filesystem::directory_iterator{dir, ec})
    {
        if (!entry.is_regular_file() || entry.path().extension() != ".json")
        {
            continue;
        }
        const auto text = read_text_file(entry.path());
        if (!text)
        {
            continue;
        }
        const auto parsed = parse_json(*text);
        if (!parsed || !parsed->is_object())
        {
            log::warn("fixture malformada: {}", entry.path().filename().string());
            continue;
        }

        Fixture fixture;
        fixture.id = parsed->value("id", entry.path().stem().string());
        fixture.note = parsed->value("note", std::string{});
        fixture.skill_id = parsed->value("skill", std::string{});
        fixture.enemy_id = parsed->value("enemy", std::string{});
        fixture.broken = parsed->value("broken", false);
        fixture.observed_damage = parsed->value("observed_damage", 0.0);
        fixture.kind = kind_from_name(parsed->value("kind", std::string{"non_crit"}));
        fixture.picto_ids = string_array(*parsed, "pictos");
        fixture.lumina_ids = string_array(*parsed, "luminas");

        if (const auto stats = parsed->find("base_stats");
            stats != parsed->end() && stats->is_object())
        {
            fixture.base_stats.attack = stats->value("attack", 0.0);
            fixture.base_stats.crit_rate = stats->value("crit_rate", 0.0);
            fixture.base_stats.crit_damage = stats->value("crit_damage", 0.0);
            fixture.base_stats.damage_bonus = stats->value("damage_bonus", 0.0);
        }

        if (fixture.observed_damage <= 0.0 || fixture.skill_id.empty())
        {
            log::warn("fixture {} ignorada: sem dano observado ou sem skill", fixture.id);
            continue;
        }
        fixtures.push_back(std::move(fixture));
    }
    return fixtures;
}

FixtureReport evaluate_fixtures(const std::vector<Fixture>& fixtures, const GameData& data,
                                const FormulaCoefficients& coefficients)
{
    FixtureReport report;
    for (const auto& fixture : fixtures)
    {
        const auto* skill = data.find_skill(fixture.skill_id);
        if (skill == nullptr)
        {
            report.skipped.push_back(fixture.id);
            continue;
        }

        std::vector<const Picto*> pictos;
        bool missing = false;
        for (const auto& id : fixture.picto_ids)
        {
            const auto* picto = data.find_picto(id);
            missing = missing || picto == nullptr;
            if (picto != nullptr)
            {
                pictos.push_back(picto);
            }
        }
        std::vector<const Lumina*> luminas;
        for (const auto& id : fixture.lumina_ids)
        {
            const auto* lumina = data.find_lumina(id);
            missing = missing || lumina == nullptr;
            if (lumina != nullptr)
            {
                luminas.push_back(lumina);
            }
        }
        if (missing)
        {
            // Uma fixture que cita um id inexistente mediria a formula contra
            // uma build diferente da que foi jogada. Pular e avisar.
            report.skipped.push_back(fixture.id);
            continue;
        }

        Target target{};
        if (const auto* enemy = data.find_enemy(fixture.enemy_id))
        {
            target = *enemy;
        }
        target.broken = fixture.broken;

        const auto stats = total_stats(fixture.base_stats, pictos, luminas, {});
        const auto result = compute_damage(stats, *skill, target, coefficients);

        double predicted = result.minimum();
        switch (fixture.kind)
        {
        case Fixture::Kind::Crit:
            predicted = result.maximum();
            break;
        case Fixture::Kind::Expected:
            predicted = result.expected();
            break;
        case Fixture::Kind::NonCrit:
            break;
        }

        report.errors.push_back(FixtureError{.id = fixture.id,
                                             .observed = fixture.observed_damage,
                                             .predicted = predicted});
    }
    return report;
}
} // namespace e33::calc
