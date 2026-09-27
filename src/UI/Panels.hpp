#pragma once

#include "Core/AppController.hpp"
#include "UI/OverlayState.hpp"

namespace e33::ui
{
void draw_overlay(AppController& app, OverlayState& state);

void draw_build_panel(AppController& app, OverlayState& state);
void draw_optimize_panel(AppController& app, OverlayState& state);
void draw_compare_panel(AppController& app, OverlayState& state);
void draw_settings_panel(AppController& app, OverlayState& state);
} // namespace e33::ui
