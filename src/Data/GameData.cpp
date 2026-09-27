#include "Data/GameData.hpp"

#include <algorithm>
#include <format>

#include "Support/Json.hpp"
#include "Support/Log.hpp"

namespace e33
{
namespace
{
std::string string_field(const Json& obj, std::string_view key, std::string fallback = {})
{
    const auto it = obj.find(key);
    return (it != obj.end() && it->is_string()) ? it->get<std::string>() : std::move(fallback);
}

double number_field(const Json& obj, std::string_view key, double fallback = 0.0)
{
    const auto it = obj.find(key);
    return (it != obj.end() && it->is_number()) ? it->get<double>() : fallback;
}

bool bool_field(const Json& obj, std::string_view key, bool fallback)
{
    const auto it = obj.find(key);
    return (it != obj.end() && it->is_boolean()) ? it->get<bool>() : fallback;
}

Stats stats_field(const Json& obj)
{
    Stats stats;
    const auto it = obj.find("stats");
    if (it == obj.end() || !it->is_object())
    {
        return stats;
    }
    const auto& s = *it;
    stats.attack = number_field(s, "attack");
    stats.defense = number_field(s, "defense");
    stats.health = number_field(s, "health");
    stats.speed = number_field(s, "speed");
    stats.crit_rate = number_field(s, "crit_rate");
    stats.crit_damage = number_field(s, "crit_damage");
    stats.damage_bonus = number_field(s, "damage_bonus");

    if (const auto elements = s.find("element_bonus");
        elements != s.end() && elements->is_object())
    {
        for (const auto& [name, value] : elements->items())
        {
            if (!value.is_number())
            {
                continue;
            }
            const auto element = element_from_name(name);
            if (element == Element::None)
            {
                continue;
            }
            stats.element_bonus[static_cast<std::size_t>(element)] = value.get<double>();
        }
    }
    return stats;
}

// Lê um arquivo de tabela. `key` é o nome do array dentro do objeto raiz.
template <typename Fn>
bool load_table(const std::filesystem::path& path, std::string_view key,
                GameData::LoadReport& report, Fn&& emit)
{
    const auto text = read_text_file(path);
    if (!text)
    {
        report.errors.push_back(std::format("{} nao encontrado", path.filename().string()));
        return false;
    }
    const auto parsed = parse_json(*text);
    if (!parsed || !parsed->contains(key) || !parsed->at(key).is_array())
    {
        report.errors.push_back(
            std::format("{} nao tem o array \"{}\"", path.filename().string(), key));
        return false;
    }
    std::size_t skipped = 0;
    for (const auto& item : parsed->at(key))
    {
        if (!item.is_object() || string_field(item, "id").empty() || !emit(item))
        {
            ++skipped;
        }
    }
    if (skipped > 0)
    {
        log::warn("{}: {} linha(s) ignorada(s) por estarem incompletas",
                  path.filename().string(), skipped);
    }
    return true;
}
} // namespace

GameData::LoadReport GameData::load(const std::filesystem::path& version_dir)
{
    LoadReport report;
    m_version = version_dir.filename().string();
    m_pictos.clear();
    m_luminas.clear();
    m_weapons.clear();
    m_skills.clear();
    m_enemies.clear();

    load_table(version_dir / "pictos.json", "pictos", report, [this](const Json& item) {
        Picto picto;
        picto.id = string_field(item, "id");
        picto.name = string_field(item, "name", picto.id);
        picto.lumina_cost = static_cast<int>(number_field(item, "lumina_cost"));
        picto.stats = stats_field(item);
        picto.owned = bool_field(item, "owned", true);
        m_pictos.push_back(std::move(picto));
        return true;
    });
    report.pictos = m_pictos.size();

    load_table(version_dir / "luminas.json", "luminas", report, [this](const Json& item) {
        Lumina lumina;
        lumina.id = string_field(item, "id");
        lumina.name = string_field(item, "name", lumina.id);
        lumina.cost = static_cast<int>(number_field(item, "cost"));
        lumina.stats = stats_field(item);
        lumina.owned = bool_field(item, "owned", true);
        m_luminas.push_back(std::move(lumina));
        return true;
    });
    report.luminas = m_luminas.size();

    load_table(version_dir / "weapons.json", "weapons", report, [this](const Json& item) {
        Weapon weapon;
        weapon.id = string_field(item, "id");
        weapon.name = string_field(item, "name", weapon.id);
        weapon.stats = stats_field(item);
        weapon.element = element_from_name(string_field(item, "element", "none"));
        m_weapons.push_back(std::move(weapon));
        return true;
    });
    report.weapons = m_weapons.size();

    load_table(version_dir / "skills.json", "skills", report, [this](const Json& item) {
        Skill skill;
        skill.id = string_field(item, "id");
        skill.name = string_field(item, "name", skill.id);
        skill.power = number_field(item, "power", 1.0);
        skill.element = element_from_name(string_field(item, "element", "none"));
        skill.hits = std::max(1, static_cast<int>(number_field(item, "hits", 1.0)));
        skill.can_crit = bool_field(item, "can_crit", true);
        m_skills.push_back(std::move(skill));
        return true;
    });
    report.skills = m_skills.size();

    load_table(version_dir / "enemies.json", "enemies", report, [this](const Json& item) {
        Target enemy;
        enemy.id = string_field(item, "id");
        enemy.name = string_field(item, "name", enemy.id);
        enemy.defense = number_field(item, "defense");
        enemy.weakness = element_from_name(string_field(item, "weakness", "none"));
        enemy.weakness_multiplier = number_field(item, "weakness_multiplier", 1.5);
        enemy.break_multiplier = number_field(item, "break_multiplier", 1.5);
        m_enemies.push_back(std::move(enemy));
        return true;
    });
    report.enemies = m_enemies.size();

    return report;
}

namespace
{
template <typename T>
const T* find_by_id(const std::vector<T>& items, std::string_view id)
{
    const auto it = std::ranges::find_if(items, [id](const T& item) { return item.id == id; });
    return it == items.end() ? nullptr : &*it;
}
} // namespace

const Picto* GameData::find_picto(std::string_view id) const { return find_by_id(m_pictos, id); }
const Lumina* GameData::find_lumina(std::string_view id) const { return find_by_id(m_luminas, id); }
const Skill* GameData::find_skill(std::string_view id) const { return find_by_id(m_skills, id); }
const Target* GameData::find_enemy(std::string_view id) const { return find_by_id(m_enemies, id); }
} // namespace e33
