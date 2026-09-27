#include "Mod.hpp"

using namespace RC;

namespace e33
{
PictoOptimizerMod::PictoOptimizerMod()
{
    ModName = STR("PictoOptimizer");
    ModVersion = STR("0.1.0");
    ModDescription = STR("Overlay in-game: build atual, dano e otimizacao de pictos");
    ModAuthors = STR("mkmuniz");
}

PictoOptimizerMod::~PictoOptimizerMod() = default;

void PictoOptimizerMod::on_unreal_init()
{
    // M3: instalar a leitura do estado ao vivo (party, pictos, luminas, arma).
}

void PictoOptimizerMod::on_update()
{
    // M1: hotkey (F8 por padrao). Evitar J — o Gramophone Everywhere usa.
}
} // namespace e33
