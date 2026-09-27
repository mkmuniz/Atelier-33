#include "UI/Panels.hpp"

#include <imgui.h>

#include "UI/Theme.hpp"

namespace e33::ui
{
void draw_compare_panel(AppController& app, OverlayState& state)
{
    const auto& result = app.last_result();
    if (!result || result->top.empty())
    {
        ImGui::TextDisabled("Rode uma busca na aba Otimizar para comparar.");
        return;
    }

    const auto index = static_cast<std::size_t>(
        std::max(0, std::min(state.selected_result, static_cast<int>(result->top.size()) - 1)));
    const auto diff = app.diff_against_current(result->top[index]);

    ImGui::Text("Atual: %.0f", diff.current_damage);
    ImGui::SameLine();
    ImGui::Text("-> Sugerida: %.0f", diff.suggested_damage);
    ImGui::SameLine();
    ImGui::TextColored(diff.delta() >= 0.0 ? theme::color::kVerdigris
                                           : theme::color::kBlood,
                       "(%+.1f%%)", diff.delta_percent());

    theme::rule();

    if (diff.empty())
    {
        ImGui::TextColored(theme::color::kVerdigris,
                           "Sua build ja e essa. Nada a trocar.");
        return;
    }

    // A lista exata do que trocar e o produto final: o numero sozinho nao diz
    // ao usuario o que fazer quando ele voltar para o menu do jogo.
    theme::heading("Equipar");
    for (const auto* picto : diff.add_pictos)
    {
        ImGui::BulletText("picto %s", picto->name.c_str());
    }
    for (const auto* lumina : diff.add_luminas)
    {
        ImGui::BulletText("lumina %s (%d pts)", lumina->name.c_str(), lumina->cost);
    }
    if (diff.add_pictos.empty() && diff.add_luminas.empty())
    {
        ImGui::TextDisabled("  (nada)");
    }

    theme::heading("Remover");
    for (const auto* picto : diff.remove_pictos)
    {
        ImGui::BulletText("picto %s", picto->name.c_str());
    }
    for (const auto* lumina : diff.remove_luminas)
    {
        ImGui::BulletText("lumina %s", lumina->name.c_str());
    }
    if (diff.remove_pictos.empty() && diff.remove_luminas.empty())
    {
        ImGui::TextDisabled("  (nada)");
    }
}
} // namespace e33::ui
