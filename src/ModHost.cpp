#include "ModHost.hpp"

#include "Game/ReadParty.hpp"
#include "Platform/Overlay.hpp"
#include "Support/Log.hpp"
#include "UI/Panels.hpp"
#include "UI/Theme.hpp"

namespace e33
{
ModHost& ModHost::instance()
{
    static ModHost host;
    return host;
}

#if defined(_WIN32)
void ModHost::start(HMODULE module)
{
    if (m_started)
    {
        return;
    }
    m_started = true;

    // A pasta da DLL, não o diretório de trabalho: o diretório de trabalho de
    // um mod injetado é o do jogo.
    wchar_t buffer[MAX_PATH]{};
    const auto dll_dir = GetModuleFileNameW(module, buffer, MAX_PATH) != 0
                             ? std::filesystem::path{buffer}.parent_path()
                             : std::filesystem::current_path();

    // A DLL mora ao lado do executavel (e o que o proxy exige), mas os
    // arquivos do mod ficam numa subpasta com o nome dele. Isso evita despejar
    // config.json e data/ dentro da pasta do jogo, e evita que dois mods
    // instalados juntos disputem os mesmos nomes de arquivo.
    m_mod_dir = dll_dir / "Atelier33";

    log::open_file(m_mod_dir / "PictoOptimizer.log");
    log::info("carregando de {}", m_mod_dir.string());

    m_app = std::make_unique<AppController>(std::make_unique<UnrealPartySource>());
    m_app->initialize(m_mod_dir);
    m_overlay.open = m_app->settings().start_open;

    platform::Config config;
    config.mod_dir = m_mod_dir;
    config.on_first_frame = [this] { on_first_frame(); };
    config.on_render = [this] { on_render(); };

    if (!platform::install(std::move(config)))
    {
        log::warn("overlay nao instalado; o mod continua carregado mas sem interface");
    }
}
#endif

void ModHost::stop()
{
    if (!m_started)
    {
        return;
    }
    m_started = false;

    // Cancelar antes de destruir: uma busca em andamento segura ponteiros para
    // o inventário que está prestes a sumir.
    if (m_app)
    {
        m_app->cancel_optimization();
    }
    platform::uninstall();
    m_app.reset();
    log::close_file();
}

void ModHost::on_first_frame()
{
    // Aqui o contexto do ImGui existe e o atlas ainda não foi usado, que é a
    // única janela segura para adicionar fonte.
    ui::theme::apply_style();
    if (!ui::theme::load_fonts(m_mod_dir / "assets"))
    {
        log::warn("fonte propria nao carregada; nomes acentuados podem sair errados");
    }
}

void ModHost::on_render()
{
    if (m_app == nullptr)
    {
        return;
    }

#if defined(_WIN32)
    const auto now = static_cast<double>(GetTickCount64()) / 1000.0;
#else
    const auto now = 0.0;
#endif
    m_app->tick(now);

    if (const auto vk = m_app->settings().hotkey_virtual_key();
        vk && platform::key_pressed(*vk))
    {
        m_overlay.open = !m_overlay.open;
    }

    // Em modo compacto a janela não captura input: ela é só uma inscrição.
    platform::set_wants_input(m_overlay.open && !m_app->settings().compact_mode);

    ui::draw_overlay(*m_app, m_overlay);
}
} // namespace e33
