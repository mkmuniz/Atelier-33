#include "UI/Panels.hpp"

#include <cstdio>

#include <imgui.h>

#include "UI/Theme.hpp"

namespace e33::ui
{
void draw_settings_panel(AppController& app, OverlayState& state)
{
    auto& settings = app.settings();

    char hotkey[32]{};
    std::snprintf(hotkey, sizeof(hotkey), "%s", settings.hotkey.c_str());
    ImGui::SetNextItemWidth(120.0f);
    if (ImGui::InputText("Hotkey", hotkey, sizeof(hotkey)))
    {
        if (virtual_key_from_name(hotkey))
        {
            settings.hotkey = hotkey;
            static_cast<void>(settings.save());
        }
    }
    ImGui::SameLine();
    ImGui::TextDisabled("(evite J: e do Gramophone Everywhere)");

    ImGui::SetNextItemWidth(220.0f);
    if (ImGui::SliderFloat("Escala da fonte", &settings.font_scale, Settings::kMinFontScale,
                           Settings::kMaxFontScale, "%.2fx"))
    {
        static_cast<void>(settings.save());
    }

    if (theme::checkbox("Modo compacto (so o numero)", &settings.compact_mode))
    {
        static_cast<void>(settings.save());
    }
    if (theme::checkbox("Abrir o overlay ao iniciar", &settings.start_open))
    {
        static_cast<void>(settings.save());
    }
    if (theme::checkbox("Log verboso", &settings.verbose_log))
    {
        static_cast<void>(settings.save());
    }

    theme::rule();
    ImGui::Text("Dados: versao %s", app.data().version().c_str());
    ImGui::TextDisabled("%zu pictos, %zu luminas, %zu skills, %zu inimigos",
                        app.data().pictos().size(), app.data().luminas().size(),
                        app.data().skills().size(), app.data().enemies().size());

    // A taxa de erro da formula e o que separa "ferramenta" de "chute
    // bem-apresentado". Enquanto nao houver fixtures medidas, dizer isso.
    theme::rule();
    ImGui::TextWrapped("Coeficientes da formula: escala %.3f, expoente de ataque %.3f, "
                       "critico base x%.2f.",
                       app.coefficients().global_scale, app.coefficients().attack_exponent,
                       app.coefficients().crit_base);
    ImGui::TextColored(theme::color::kGold,
                       "Ainda nao calibrados contra dano medido em jogo (M4).");

    static_cast<void>(state);
}
} // namespace e33::ui
