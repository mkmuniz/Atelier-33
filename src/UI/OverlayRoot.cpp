#include "UI/Panels.hpp"

#include <imgui.h>

#include "UI/Theme.hpp"

namespace e33::ui
{
namespace
{
void apply_font_scale(float scale)
{
#if defined(IMGUI_VERSION_NUM) && IMGUI_VERSION_NUM >= 19200
    ImGui::GetStyle().FontScaleMain = scale;
#else
    ImGui::GetIO().FontGlobalScale = scale;
#endif
}

void underline_active_tab()
{
    const auto min = ImGui::GetItemRectMin();
    const auto max = ImGui::GetItemRectMax();
    ImGui::GetWindowDrawList()->AddLine(
        ImVec2{min.x, max.y}, ImVec2{max.x, max.y},
        ImGui::ColorConvertFloat4ToU32(theme::color::kGold), 1.5f);
}

bool tab(const char* label)
{
    const bool open = ImGui::BeginTabItem(label);
    if (open)
    {
        underline_active_tab();
    }
    return open;
}

// Modo compacto (M6): só o número, para quem deixa o overlay sempre aberto.
// Sem moldura e sem fundo — uma inscrição dourada sobre o jogo.
void draw_compact(AppController& app)
{
    ImGui::SetNextWindowBgAlpha(0.35f);
    if (ImGui::Begin("##compact", nullptr,
                     ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize
                         | ImGuiWindowFlags_NoFocusOnAppearing))
    {
        if (const auto damage = app.current_damage())
        {
            theme::push_display_font();
            theme::text_value(damage->expected(), "%.0f");
            theme::pop_font();
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("dano esperado");
            }
        }
        else
        {
            theme::text_dim("--");
        }
    }
    ImGui::End();
}
} // namespace

void draw_overlay(AppController& app, OverlayState& state)
{
    if (!state.open)
    {
        return;
    }

    apply_font_scale(app.settings().font_scale);

    if (app.settings().compact_mode)
    {
        draw_compact(app);
        return;
    }

    ImGui::SetNextWindowPos(ImVec2{20.0f, 20.0f}, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2{860.0f, 720.0f}, ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("ATELIER", &state.open))
    {
        ImGui::End();
        return;
    }

    theme::window_ornaments();

    theme::push_small_font();
    theme::text_dim("pictos, luminas e o dano que eles rendem");
    theme::pop_font();

    theme::rule();

    if (!app.load_report().ok())
    {
        ImGui::PushStyleColor(ImGuiCol_Text, theme::color::kBlood);
        ImGui::TextUnformatted(
            "Dados incompletos: os resultados estariam errados, nao so piores.");
        ImGui::PopStyleColor();
        theme::push_small_font();
        for (const auto& error : app.load_report().errors)
        {
            theme::text_dim(error);
        }
        theme::pop_font();
        theme::rule();
    }

    if (!app.party().valid)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, theme::color::kGold);
        ImGui::Text("Estado do jogo indisponivel: %s", app.party().error.c_str());
        ImGui::PopStyleColor();
        theme::rule();
    }

    if (ImGui::BeginTabBar("##tabs", ImGuiTabBarFlags_NoTooltip))
    {
        if (tab("Build"))
        {
            draw_build_panel(app, state);
            ImGui::EndTabItem();
        }
        if (tab("Otimizar"))
        {
            draw_optimize_panel(app, state);
            ImGui::EndTabItem();
        }
        if (tab("Comparar"))
        {
            draw_compare_panel(app, state);
            ImGui::EndTabItem();
        }
        if (tab("Ajustes"))
        {
            draw_settings_panel(app, state);
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    theme::rule();
    theme::push_small_font();
    theme::text_dim(app.status_line());
    theme::pop_font();

    ImGui::End();
}
} // namespace e33::ui
