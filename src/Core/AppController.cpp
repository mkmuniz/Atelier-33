#include "Core/AppController.hpp"

#include <algorithm>
#include <format>
#include <utility>

#include "Support/Log.hpp"

namespace e33
{
double BuildDiff::delta_percent() const
{
    if (current_damage <= 0.0)
    {
        return 0.0;
    }
    return (suggested_damage - current_damage) / current_damage * 100.0;
}

bool BuildDiff::empty() const
{
    return add_pictos.empty() && remove_pictos.empty() && add_luminas.empty()
           && remove_luminas.empty();
}

AppController::AppController(std::unique_ptr<IPartySource> party)
    : m_party_source{std::move(party)}
{
}

void AppController::initialize(const std::filesystem::path& mod_dir)
{
    m_settings.load(mod_dir / "settings.json");
    log::set_verbose(m_settings.verbose_log);

    const auto version_dir = mod_dir / "data" / m_settings.data_version;
    m_load_report = m_data.load(version_dir);
    m_coefficients.load(version_dir / "formula.json");

    if (!m_load_report.ok())
    {
        // Sem as tabelas o otimizador não degrada, ele erra. Melhor dizer isso
        // na cara do usuário do que sugerir uma build com dados pela metade.
        m_status = std::format("dados incompletos em {}: {}", version_dir.string(),
                               m_load_report.errors.front());
        log::warn("{}", m_status);
    }
    else
    {
        m_status = std::format("dados {} carregados: {} pictos, {} luminas, {} skills",
                               m_data.version(), m_load_report.pictos, m_load_report.luminas,
                               m_load_report.skills);
        log::info("{}", m_status);
    }

    if (!m_data.skills().empty())
    {
        m_selected_skill = m_data.skills().front().id;
    }
    if (!m_data.enemies().empty())
    {
        m_selected_enemy = m_data.enemies().front().id;
    }
}

void AppController::tick(double now_seconds)
{
    // O overlay tem de refletir a troca de um picto no menu do jogo sem o
    // usuário pedir, mas ler a memória a cada frame é caro à toa.
    if (m_last_poll < 0.0 || now_seconds - m_last_poll >= kPartyPollSeconds)
    {
        m_last_poll = now_seconds;
        m_party = m_party_source->read();
        if (!m_party.valid && !m_party.error.empty())
        {
            m_status = m_party.error;
        }
        m_selected_character = m_party.characters.empty()
                                   ? 0
                                   : std::min(m_selected_character,
                                              m_party.characters.size() - 1);
    }

    if (!m_job.running())
    {
        if (auto result = m_job.take_result())
        {
            m_last_result = std::move(result);
            const auto& r = *m_last_result;
            m_status = std::format("busca: {} avaliadas, {} podadas, {:.2f}s{}", r.evaluated,
                                   r.pruned_by_bound, m_job.elapsed_seconds(),
                                   r.exhaustive ? " (otimo provado)" : " (melhor encontrado)");
        }
    }
}

void AppController::select_character(std::size_t index)
{
    if (m_party.characters.empty())
    {
        m_selected_character = 0;
        return;
    }
    m_selected_character = std::min(index, m_party.characters.size() - 1);
}

void AppController::select_skill(std::string_view skill_id)
{
    m_selected_skill = std::string{skill_id};
}

void AppController::select_enemy(std::string_view enemy_id)
{
    m_selected_enemy = std::string{enemy_id};
}

void AppController::set_include_unowned(bool include)
{
    if (m_settings.include_unowned == include)
    {
        return;
    }
    m_settings.include_unowned = include;
    static_cast<void>(m_settings.save());
}

const CharacterSnapshot* AppController::character() const
{
    if (m_selected_character >= m_party.characters.size())
    {
        return nullptr;
    }
    return &m_party.characters[m_selected_character];
}

std::vector<const Picto*> AppController::equipped_pictos() const
{
    std::vector<const Picto*> out;
    const auto* who = character();
    if (who == nullptr)
    {
        return out;
    }
    for (const auto& id : who->equipped_picto_ids)
    {
        if (const auto* picto = m_data.find_picto(id))
        {
            out.push_back(picto);
        }
        else
        {
            // Id equipado que a tabela não conhece: quase sempre significa que
            // o jogo atualizou e os dados extraídos ficaram velhos.
            log::warn("picto equipado \"{}\" nao existe nos dados {}", id, m_data.version());
        }
    }
    return out;
}

std::vector<const Lumina*> AppController::active_luminas() const
{
    std::vector<const Lumina*> out;
    const auto* who = character();
    if (who == nullptr)
    {
        return out;
    }
    for (const auto& id : who->active_lumina_ids)
    {
        if (const auto* lumina = m_data.find_lumina(id))
        {
            out.push_back(lumina);
        }
    }
    return out;
}

std::optional<calc::DamageResult> AppController::current_damage() const
{
    const auto* who = character();
    const auto* skill = m_data.find_skill(m_selected_skill);
    if (who == nullptr || skill == nullptr)
    {
        return std::nullopt;
    }

    auto base = who->base_stats;
    if (const auto weapon = std::ranges::find_if(
            m_data.weapons(), [who](const Weapon& w) { return w.id == who->weapon_id; });
        weapon != m_data.weapons().end())
    {
        base += weapon->stats;
    }

    Target target{};
    if (const auto* enemy = m_data.find_enemy(m_selected_enemy))
    {
        target = *enemy;
    }
    target.broken = m_target_broken;

    const auto pictos = equipped_pictos();
    const auto luminas = active_luminas();
    const auto stats = calc::total_stats(base, pictos, luminas, {});
    return calc::compute_damage(stats, *skill, target, m_coefficients);
}

std::optional<opt::Request> AppController::build_request() const
{
    const auto* who = character();
    const auto* skill = m_data.find_skill(m_selected_skill);
    if (who == nullptr || skill == nullptr || m_data.pictos().empty())
    {
        return std::nullopt;
    }

    opt::Request request;
    request.base_stats = who->base_stats;
    if (const auto weapon = std::ranges::find_if(
            m_data.weapons(), [who](const Weapon& w) { return w.id == who->weapon_id; });
        weapon != m_data.weapons().end())
    {
        request.base_stats += weapon->stats;
    }

    request.skill = *skill;
    if (const auto* enemy = m_data.find_enemy(m_selected_enemy))
    {
        request.target = *enemy;
    }
    request.target.broken = m_target_broken;
    request.coefficients = m_coefficients;
    request.picto_slots = who->picto_slots;
    request.lumina_budget = who->lumina_budget;
    request.include_unowned = m_settings.include_unowned;

    // Copia o inventário marcando o que o jogador realmente tem. O toggle do M6
    // decide se o que falta entra na busca.
    request.pictos = m_data.pictos();
    for (auto& picto : request.pictos)
    {
        picto.owned = std::ranges::find(m_party.owned_picto_ids, picto.id)
                      != m_party.owned_picto_ids.end();
    }
    request.luminas = m_data.luminas();
    for (auto& lumina : request.luminas)
    {
        lumina.owned = std::ranges::find(m_party.owned_lumina_ids, lumina.id)
                       != m_party.owned_lumina_ids.end();
    }
    return request;
}

void AppController::start_optimization()
{
    auto request = build_request();
    if (!request)
    {
        m_status = "nada para otimizar: sem personagem, skill ou tabela de pictos";
        return;
    }
    m_last_result.reset();
    m_job.start(std::move(*request), opt::Options{.top_n = 5});
    m_status = "buscando...";
}

void AppController::cancel_optimization()
{
    m_job.cancel();
    m_status = "busca cancelada";
}

BuildDiff AppController::diff_against_current(const opt::Candidate& candidate) const
{
    BuildDiff diff;
    const auto current_pictos = equipped_pictos();
    const auto current_luminas = active_luminas();

    const auto has = [](const auto& list, const auto* item) {
        return std::ranges::any_of(list, [item](const auto* other) {
            return other != nullptr && item != nullptr && other->id == item->id;
        });
    };

    for (const auto* picto : candidate.pictos)
    {
        if (!has(current_pictos, picto))
        {
            diff.add_pictos.push_back(picto);
        }
    }
    for (const auto* picto : current_pictos)
    {
        if (!has(candidate.pictos, picto))
        {
            diff.remove_pictos.push_back(picto);
        }
    }
    for (const auto* lumina : candidate.luminas)
    {
        if (!has(current_luminas, lumina))
        {
            diff.add_luminas.push_back(lumina);
        }
    }
    for (const auto* lumina : current_luminas)
    {
        if (!has(candidate.luminas, lumina))
        {
            diff.remove_luminas.push_back(lumina);
        }
    }

    if (const auto current = current_damage())
    {
        diff.current_damage = current->expected();
    }
    diff.suggested_damage = candidate.expected_damage;
    return diff;
}

std::string AppController::export_build_text() const
{
    const auto* who = character();
    if (who == nullptr)
    {
        return {};
    }

    std::string out = std::format("**{}**\n", who->name);
    for (const auto* picto : equipped_pictos())
    {
        out += std::format("- picto: {}\n", picto->name);
    }
    for (const auto* lumina : active_luminas())
    {
        out += std::format("- lumina: {} ({} pts)\n", lumina->name, lumina->cost);
    }
    if (const auto damage = current_damage())
    {
        const auto* skill = m_data.find_skill(m_selected_skill);
        out += std::format("dano esperado ({}): {:.0f}\n",
                           skill != nullptr ? skill->name : m_selected_skill,
                           damage->expected());
    }
    out += std::format("_dados {} — E33 Picto Optimizer_\n", m_data.version());
    return out;
}
} // namespace e33
