#pragma once

#include <memory>

#include <Mod/CppUserModBase.hpp>

#include "Core/AppController.hpp"
#include "UI/OverlayState.hpp"

namespace e33
{
class PictoOptimizerMod final : public RC::CppUserModBase
{
public:
    PictoOptimizerMod();
    ~PictoOptimizerMod() override;

    void on_unreal_init() override;
    void on_update() override;

private:
    void render();
    void poll_hotkey();
    void ensure_theme();

    std::unique_ptr<AppController> m_app;
    ui::OverlayState m_overlay{};
    bool m_hotkey_was_down{false};
    bool m_theme_applied{false};
};
} // namespace e33
