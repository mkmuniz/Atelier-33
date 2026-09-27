#pragma once

#include <cstddef>

namespace e33::ui
{
struct OverlayState
{
    bool open{false};
    int selected_result{0};
    bool show_breakdown{true};
};
} // namespace e33::ui
