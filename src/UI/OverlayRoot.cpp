#include "UI/Panels.hpp"

#include <imgui.h>

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

// Modo compacto (M6): só o número, para quem deixa o overlay sempre aberto.
void draw_compact(AppController& app)
{
    ImGui::SetNextWindowBgAlpha(0.65f);
    if (ImGui::Begin("##compact", nullptr,
                     ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize
                         | ImGuiWindowFlags_NoFocusOnAppearing))
    {
        if (const auto damage = app.current_damage())
        {
            ImGui::Text("%.0f", damage->expected());
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("dano esperado — clique com o botao direito para o modo completo");
            }
        }
        else
        {
            ImGui::TextDisabled("--");
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

    // TODO(M1, verificar no PC): se os nomes franceses saírem cortados em jogo,
    // é o glyph range da fonte que o UE4SS carrega, não este código.
    ImGui::SetNextWindowPos(ImVec2{40.0f, 40.0f}, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2{820.0f, 520.0f}, ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Picto Optimizer", &state.open))
    {
        ImGui::End();
        return;
    }

    if (!app.load_report().ok())
    {
        ImGui::TextColored(ImVec4{1.0f, 0.55f, 0.55f, 1.0f},
                           "Dados incompletos: os resultados estariam errados, nao so piores.");
        for (const auto& error : app.load_report().errors)
        {
            ImGui::BulletText("%s", error.c_str());
        }
        ImGui::Separator();
    }

    if (!app.party().valid)
    {
        ImGui::TextColored(ImVec4{1.0f, 0.75f, 0.3f, 1.0f}, "Estado do jogo indisponivel: %s",
                           app.party().error.c_str());
        ImGui::Separator();
    }

    if (ImGui::BeginTabBar("##tabs"))
    {
        if (ImGui::BeginTabItem("Build"))
        {
            draw_build_panel(app, state);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Otimizar"))
        {
            draw_optimize_panel(app, state);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Comparar"))
        {
            draw_compare_panel(app, state);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Ajustes"))
        {
            draw_settings_panel(app, state);
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    ImGui::Separator();
    ImGui::TextDisabled("%s", app.status_line().c_str());
    ImGui::End();
}
} // namespace e33::ui
