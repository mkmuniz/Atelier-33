#include "UI/Panels.hpp"

#include <imgui.h>

#include "UI/Theme.hpp"

namespace e33::ui
{
void draw_optimize_panel(AppController& app, OverlayState& state)
{
    auto& job = app.job();

    if (job.running())
    {
        // Progresso e botao de cancelar nao sao enfeite: a busca roda em outra
        // thread justamente para a UI continuar respondendo, e sem esses dois a
        // separacao nao serve de nada para quem esta olhando.
        ImGui::ProgressBar(static_cast<float>(job.progress()), ImVec2{-120.0f, 0.0f});
        ImGui::SameLine();
        if (theme::button("Cancelar"))
        {
            app.cancel_optimization();
        }
        ImGui::TextDisabled("%.2fs", job.elapsed_seconds());
    }
    else
    {
        if (theme::button("Otimizar"))
        {
            app.start_optimization();
        }
        ImGui::SameLine();
        bool include = app.include_unowned();
        if (theme::checkbox("Incluir pictos nao obtidos", &include))
        {
            app.set_include_unowned(include);
        }
    }

    theme::rule();

    const auto& result = app.last_result();
    if (!result)
    {
        ImGui::TextDisabled("Nenhuma busca concluida ainda.");
        return;
    }

    // Distinguir "provado otimo" de "melhor encontrado" importa: o usuario
    // decide se continua procurando na mao ou se pode parar.
    if (result->exhaustive)
    {
        ImGui::TextColored(theme::color::kVerdigris, "Otimo provado");
    }
    else if (result->cancelled)
    {
        ImGui::TextColored(theme::color::kGold, "Cancelada — melhor parcial");
    }
    else if (result->node_limit_hit)
    {
        ImGui::TextColored(theme::color::kGold,
                           "Parou no teto de busca — melhor encontrado, nao provado otimo");
    }
    else
    {
        ImGui::TextColored(theme::color::kGold,
                           "Busca aproximada — melhor encontrado");
    }
    ImGui::TextDisabled("%zu avaliadas, %zu podadas, %zu candidatos apos dominancia",
                        result->evaluated, result->pruned_by_bound,
                        result->candidates_after_dominance);

    if (result->top.empty())
    {
        ImGui::TextDisabled("Nenhuma build encontrada.");
        return;
    }

    const auto current = app.current_damage();
    const auto current_damage = current ? current->expected() : 0.0;

    if (ImGui::BeginTable("##results", 4,
                          ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg
                              | ImGuiTableFlags_SizingStretchProp))
    {
        ImGui::TableSetupColumn("#");
        ImGui::TableSetupColumn("Dano");
        ImGui::TableSetupColumn("vs atual");
        ImGui::TableSetupColumn("Itens");
        ImGui::TableHeadersRow();

        for (int i = 0; i < static_cast<int>(result->top.size()); ++i)
        {
            const auto& candidate = result->top[static_cast<std::size_t>(i)];
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::PushID(i);
            if (ImGui::Selectable("##pick", state.selected_result == i,
                                  ImGuiSelectableFlags_SpanAllColumns))
            {
                state.selected_result = i;
            }
            ImGui::SameLine();
            ImGui::Text("%d", i + 1);

            ImGui::TableNextColumn();
            ImGui::Text("%.0f", candidate.expected_damage);

            ImGui::TableNextColumn();
            const auto delta = current_damage > 0.0
                                   ? (candidate.expected_damage - current_damage)
                                         / current_damage * 100.0
                                   : 0.0;
            ImGui::TextColored(delta >= 0.0 ? theme::color::kVerdigris
                                            : theme::color::kBlood,
                               "%+.1f%%", delta);

            ImGui::TableNextColumn();
            std::string items;
            for (const auto* picto : candidate.pictos)
            {
                items += picto->name + " ";
            }
            if (!candidate.luminas.empty())
            {
                items += "| ";
                for (const auto* lumina : candidate.luminas)
                {
                    items += lumina->name + " ";
                }
            }
            ImGui::TextUnformatted(items.c_str());
            ImGui::PopID();
        }
        ImGui::EndTable();
    }
}
} // namespace e33::ui
