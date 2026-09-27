#pragma once

#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "Config/Settings.hpp"
#include "Data/GameData.hpp"
#include "Game/PartyState.hpp"
#include "Optimizer/Job.hpp"

namespace e33
{
// Diferença entre a build atual e uma sugerida, item a item. É o que o painel
// de comparação mostra: não o número final, mas o que exatamente trocar.
struct BuildDiff
{
    std::vector<const Picto*> add_pictos{};
    std::vector<const Picto*> remove_pictos{};
    std::vector<const Lumina*> add_luminas{};
    std::vector<const Lumina*> remove_luminas{};
    double current_damage{0.0};
    double suggested_damage{0.0};

    [[nodiscard]] double delta() const { return suggested_damage - current_damage; }
    [[nodiscard]] double delta_percent() const;
    [[nodiscard]] bool empty() const;
};

// Estado do mod. A UI lê daqui e chama daqui; não existe caminho da UI para a
// memória do jogo que não passe por esta classe.
class AppController
{
public:
    explicit AppController(std::unique_ptr<IPartySource> party);

    void initialize(const std::filesystem::path& mod_dir);
    void tick(double now_seconds);

    [[nodiscard]] Settings& settings() { return m_settings; }
    [[nodiscard]] const GameData& data() const { return m_data; }
    [[nodiscard]] const GameData::LoadReport& load_report() const { return m_load_report; }
    [[nodiscard]] const PartySnapshot& party() const { return m_party; }
    [[nodiscard]] const std::string& status_line() const { return m_status; }
    [[nodiscard]] opt::Job& job() { return m_job; }
    [[nodiscard]] const calc::FormulaCoefficients& coefficients() const { return m_coefficients; }

    [[nodiscard]] std::size_t selected_character() const { return m_selected_character; }
    void select_character(std::size_t index);
    void select_skill(std::string_view skill_id);
    void select_enemy(std::string_view enemy_id);
    [[nodiscard]] const std::string& selected_skill() const { return m_selected_skill; }
    [[nodiscard]] const std::string& selected_enemy() const { return m_selected_enemy; }
    void set_target_broken(bool broken) { m_target_broken = broken; }
    [[nodiscard]] bool target_broken() const { return m_target_broken; }
    void set_include_unowned(bool include);
    [[nodiscard]] bool include_unowned() const { return m_settings.include_unowned; }

    [[nodiscard]] const CharacterSnapshot* character() const;
    [[nodiscard]] std::optional<calc::DamageResult> current_damage() const;
    [[nodiscard]] std::optional<opt::Request> build_request() const;

    void start_optimization();
    void cancel_optimization();
    [[nodiscard]] const std::optional<opt::Result>& last_result() const { return m_last_result; }

    [[nodiscard]] BuildDiff diff_against_current(const opt::Candidate& candidate) const;

    // Build atual como texto, para colar no Discord (M6).
    [[nodiscard]] std::string export_build_text() const;

    static constexpr double kPartyPollSeconds = 0.25;

private:
    [[nodiscard]] std::vector<const Picto*> equipped_pictos() const;
    [[nodiscard]] std::vector<const Lumina*> active_luminas() const;

    std::unique_ptr<IPartySource> m_party_source;
    Settings m_settings{};
    GameData m_data{};
    GameData::LoadReport m_load_report{};
    calc::FormulaCoefficients m_coefficients{};
    PartySnapshot m_party{};
    opt::Job m_job{};
    std::optional<opt::Result> m_last_result{};

    std::size_t m_selected_character{0};
    std::string m_selected_skill{};
    std::string m_selected_enemy{};
    bool m_target_broken{false};
    double m_last_poll{-1.0};
    std::string m_status{};
};
} // namespace e33
