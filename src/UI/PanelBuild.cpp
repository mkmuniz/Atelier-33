#include "UI/Panels.hpp"

#include <imgui.h>

#include "UI/Theme.hpp"

namespace e33::ui
{
namespace
{
// O breakdown é o motivo de o painel existir: o número final sozinho é
// indistinguível de um chute, e o usuário não tem como conferir.
void draw_breakdown(const calc::DamageResult& damage)
{
    if (!ImGui::BeginTable("##breakdown", 2,
                           ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg
                               | ImGuiTableFlags_SizingStretchProp))
    {
        return;
    }
    const auto row = [](const char* label, double value, const char* fmt) {
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::TextUnformatted(label);
        ImGui::TableNextColumn();
        ImGui::Text(fmt, value);
    };

    const auto& b = damage.breakdown;
    row("base (ataque x skill x mitigacao)", b.base, "%.1f");
    row("bonus de dano", b.damage_bonus, "x%.3f");
    row("afinidade elemental", b.element_bonus, "x%.3f");
    row("fraqueza do alvo", b.weakness, "x%.3f");
    row("break", b.brk, "x%.3f");
    row("golpes", b.hits, "x%.0f");
    row("critico (taxa)", damage.crit_rate * 100.0, "%.1f%%");
    row("critico (multiplicador)", damage.crit_multiplier, "x%.3f");
    ImGui::EndTable();
}
} // namespace

void draw_build_panel(AppController& app, OverlayState& state)
{
    const auto& party = app.party();
    if (party.characters.empty())
    {
        ImGui::TextWrapped("Nenhum personagem lido do jogo.");
        return;
    }

    const auto current_index = app.selected_character();
    ImGui::SetNextItemWidth(280.0f);
    if (ImGui::BeginCombo("Personagem", party.characters[current_index].name.c_str()))
    {
        for (std::size_t i = 0; i < party.characters.size(); ++i)
        {
            if (ImGui::Selectable(party.characters[i].name.c_str(), i == current_index))
            {
                app.select_character(i);
            }
        }
        ImGui::EndCombo();
    }

    const auto* skill = app.data().find_skill(app.selected_skill());
    ImGui::SetNextItemWidth(280.0f);
    if (ImGui::BeginCombo("Skill", skill != nullptr ? skill->name.c_str() : "(nenhuma)"))
    {
        for (const auto& option : app.data().skills())
        {
            if (ImGui::Selectable(option.name.c_str(), option.id == app.selected_skill()))
            {
                app.select_skill(option.id);
            }
        }
        ImGui::EndCombo();
    }

    const auto* enemy = app.data().find_enemy(app.selected_enemy());
    ImGui::SetNextItemWidth(280.0f);
    if (ImGui::BeginCombo("Alvo", enemy != nullptr ? enemy->name.c_str() : "(generico)"))
    {
        for (const auto& option : app.data().enemies())
        {
            if (ImGui::Selectable(option.name.c_str(), option.id == app.selected_enemy()))
            {
                app.select_enemy(option.id);
            }
        }
        ImGui::EndCombo();
    }

    bool broken = app.target_broken();
    if (theme::checkbox("Alvo em break", &broken))
    {
        app.set_target_broken(broken);
    }

    theme::rule();

    const auto damage = app.current_damage();
    if (!damage)
    {
        ImGui::TextDisabled("Sem dano calculavel: faltam skill, personagem ou dados.");
        return;
    }

    theme::heading("Dano esperado");
    ImGui::SameLine();
    theme::push_display_font();
    theme::text_value(damage->expected(), "%.0f");
    theme::pop_font();
    ImGui::SameLine();
    ImGui::TextDisabled("(min %.0f / crit %.0f)", damage->minimum(), damage->maximum());

    theme::checkbox("Mostrar breakdown", &state.show_breakdown);
    if (state.show_breakdown)
    {
        draw_breakdown(*damage);
    }

    theme::rule();
    if (theme::button("Copiar build como texto"))
    {
        ImGui::SetClipboardText(app.export_build_text().c_str());
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("Formato de colar no Discord");
    }
}
} // namespace e33::ui
