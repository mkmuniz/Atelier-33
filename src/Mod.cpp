#include "Mod.hpp"

#include <filesystem>

#include <imgui.h>

#include "Game/ReadParty.hpp"
#include "Support/Log.hpp"
#include "UI/Panels.hpp"

#if defined(_WIN32)
#include <Windows.h>
#endif

namespace e33
{
namespace
{
std::filesystem::path resolve_mod_dir()
{
#if defined(_WIN32)
    wchar_t buffer[MAX_PATH]{};
    HMODULE self{};
    if (GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS
                               | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                           reinterpret_cast<LPCWSTR>(&resolve_mod_dir), &self)
        != 0)
    {
        if (GetModuleFileNameW(self, buffer, MAX_PATH) != 0)
        {
            // ...\PictoOptimizer\dlls\main.dll -> ...\PictoOptimizer
            return std::filesystem::path{buffer}.parent_path().parent_path();
        }
    }
#endif
    return std::filesystem::current_path();
}
} // namespace

PictoOptimizerMod::PictoOptimizerMod()
{
    ModName = STR("PictoOptimizer");
    ModVersion = STR("0.1.0");
    ModDescription = STR("Overlay in-game: build atual, dano e otimizacao de pictos");
    ModAuthors = STR("mkmuniz");

    m_app = std::make_unique<AppController>(std::make_unique<UnrealPartySource>());
    m_app->initialize(resolve_mod_dir());
    m_overlay.open = m_app->settings().start_open;

    // TODO(pre-requisito do plano, so verificavel no PC): confirmar como a
    // versao de UE4SS usada registra uma janela ImGui PROPRIA sobre o jogo. Ate
    // la, register_tab garante que a UI e alcancavel. Mesma situacao do
    // Boss Music Swapper; a resposta vale para os dois.
    register_tab(STR("Picto Optimizer"), [](CppUserModBase* self) {
        static_cast<PictoOptimizerMod*>(self)->render();
    });
}

PictoOptimizerMod::~PictoOptimizerMod() = default;

void PictoOptimizerMod::on_unreal_init()
{
    // A leitura da party so faz sentido depois que o Unreal subiu. Enquanto o
    // M0 nao existe, o overlay abre e diz que nao consegue ler o jogo — o que e
    // informacao util, diferente de uma janela vazia.
    log::info("mod carregado; leitura do estado do jogo depende do M0");
}

void PictoOptimizerMod::on_update()
{
    poll_hotkey();

#if defined(_WIN32)
    m_app->tick(static_cast<double>(GetTickCount64()) / 1000.0);
#else
    m_app->tick(0.0);
#endif
}

void PictoOptimizerMod::poll_hotkey()
{
#if defined(_WIN32)
    const auto vk = m_app->settings().hotkey_virtual_key();
    if (!vk)
    {
        return;
    }
    // Borda de descida: segurar a tecla nao pode abrir e fechar a cada frame.
    const bool down = (GetAsyncKeyState(*vk) & 0x8000) != 0;
    if (down && !m_hotkey_was_down)
    {
        m_overlay.open = !m_overlay.open;
    }
    m_hotkey_was_down = down;
#endif
}

void PictoOptimizerMod::render()
{
    if (!m_overlay.open)
    {
        return;
    }
    ui::draw_overlay(*m_app, m_overlay);
}
} // namespace e33
