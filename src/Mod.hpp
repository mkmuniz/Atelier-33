#pragma once

#include <Mod/CppUserModBase.hpp>

namespace e33
{
// Regra de camadas (ver docs/plano.md): Calc/ não sabe o que é ImGui e UI/ não
// sabe o que é ponteiro de objeto do Unreal. Quando um patch quebrar o mod, o
// estrago fica em Game/.
class PictoOptimizerMod final : public RC::CppUserModBase
{
public:
    PictoOptimizerMod();
    ~PictoOptimizerMod() override;

    void on_unreal_init() override;
    void on_update() override;

private:
    bool m_overlay_open{false};
};
} // namespace e33
