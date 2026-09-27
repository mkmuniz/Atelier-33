// Harness nativo do overlay do Picto Optimizer.
//
// Desenha a MESMA janela que o mod desenha em jogo, com uma party editável no
// lugar da leitura de memória. Serve para iterar UI, cálculo e busca no macOS
// ou no Linux, sem o jogo e sem Windows. Não faz parte do pacote distribuído.
//
//   xmake build harness && xmake run harness

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <filesystem>
#include <memory>
#include <string>

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#if defined(__APPLE__)
#define GL_SILENCE_DEPRECATION
#include <OpenGL/gl3.h>
#else
#include <GL/gl.h>
#endif
#include <GLFW/glfw3.h>

#include "Core/AppController.hpp"
#include "Support/Log.hpp"
#include "UI/Panels.hpp"
#include "Screenshot.hpp"
#include "UI/Theme.hpp"

namespace
{
constexpr const char* kSampleDir = "harness/sample";

e33::PartySnapshot make_party(const e33::GameData& data)
{
    e33::CharacterSnapshot maelle;
    maelle.id = "maelle";
    maelle.name = "Maelle";
    maelle.base_stats.attack = 320.0;
    maelle.base_stats.crit_rate = 0.05;
    maelle.weapon_id = "wep_rapiere";
    maelle.equipped_picto_ids = {"pic_lame", "pic_garde"};
    maelle.active_lumina_ids = {"lum_vigueur"};
    maelle.picto_slots = 3;
    maelle.lumina_budget = 10;

    e33::CharacterSnapshot lune;
    lune.id = "lune";
    lune.name = "Lune";
    lune.base_stats.attack = 280.0;
    lune.base_stats.crit_rate = 0.12;
    lune.weapon_id = "wep_brulante";
    lune.equipped_picto_ids = {"pic_flamme"};
    lune.picto_slots = 3;
    lune.lumina_budget = 12;

    e33::PartySnapshot party;
    party.characters = {std::move(maelle), std::move(lune)};
    party.valid = true;
    // Tudo obtido por padrão; o painel permite tirar itens do inventário para
    // ver o efeito do toggle "incluir não obtidos".
    for (const auto& picto : data.pictos())
    {
        party.owned_picto_ids.push_back(picto.id);
    }
    for (const auto& lumina : data.luminas())
    {
        party.owned_lumina_ids.push_back(lumina.id);
    }
    return party;
}

// Painel que substitui a leitura de memória: edita a party e devolve ao mod.
void draw_game_stub(e33::AppController& app, e33::MockPartySource& source,
                    e33::PartySnapshot& party)
{
    ImGui::SetNextWindowPos(ImVec2{880.0f, 40.0f}, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2{380.0f, 520.0f}, ImGuiCond_FirstUseEver);
    ImGui::Begin("Jogo simulado (substitui a leitura de memoria)");
    ImGui::TextWrapped("Em jogo, isto vem de Game/ReadParty. Aqui voce edita a mao: o overlay "
                       "tem de reagir sozinho, como reage quando voce troca um picto no menu.");
    ImGui::Separator();

    bool changed = false;
    if (ImGui::Checkbox("Estado do jogo disponivel", &party.valid))
    {
        party.error = party.valid ? "" : "leitura do estado do jogo indisponivel";
        changed = true;
    }

    for (auto& character : party.characters)
    {
        ImGui::PushID(character.id.c_str());
        if (ImGui::CollapsingHeader(character.name.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::SetNextItemWidth(120.0f);
            auto attack = static_cast<float>(character.base_stats.attack);
            if (ImGui::DragFloat("Ataque base", &attack, 1.0f, 0.0f, 2000.0f, "%.0f"))
            {
                character.base_stats.attack = attack;
                changed = true;
            }
            ImGui::SetNextItemWidth(120.0f);
            if (ImGui::SliderInt("Slots", &character.picto_slots, 0, 5))
            {
                changed = true;
            }
            ImGui::SetNextItemWidth(120.0f);
            if (ImGui::SliderInt("Orcamento de lumina", &character.lumina_budget, 0, 30))
            {
                changed = true;
            }

            ImGui::TextDisabled("Pictos equipados");
            for (const auto& picto : app.data().pictos())
            {
                const auto it = std::ranges::find(character.equipped_picto_ids, picto.id);
                bool equipped = it != character.equipped_picto_ids.end();
                if (ImGui::Checkbox(picto.name.c_str(), &equipped))
                {
                    if (equipped)
                    {
                        character.equipped_picto_ids.push_back(picto.id);
                    }
                    else
                    {
                        character.equipped_picto_ids.erase(it);
                    }
                    changed = true;
                }
            }
        }
        ImGui::PopID();
    }

    if (changed)
    {
        source.set(party);
    }
    ImGui::Text("Leituras feitas pelo mod: %zu", source.reads());
    ImGui::End();
}
} // namespace

// --shot <arquivo.bmp> [--frames N] [--demo]
// Renderiza N quadros, grava o framebuffer e sai. Serve para gerar a imagem do
// README sem depender do foco de janela.
int main(int argc, char** argv)
{
    std::string shot_path;
    int shot_frame = 30;
    bool demo = false;
    for (int i = 1; i < argc; ++i)
    {
        const std::string arg = argv[i];
        if (arg == "--shot" && i + 1 < argc)
        {
            shot_path = argv[++i];
        }
        else if (arg == "--frames" && i + 1 < argc)
        {
            shot_frame = std::atoi(argv[++i]);
        }
        else if (arg == "--demo")
        {
            demo = true;
        }
    }
    const bool shot_mode = !shot_path.empty();

    if (glfwInit() == 0)
    {
        std::fprintf(stderr, "glfwInit falhou\n");
        return 1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);

    // Em modo captura a janela encosta no overlay: a imagem do README tem de
    // mostrar o mod, não a moldura do harness em volta dele.
    GLFWwindow* window = glfwCreateWindow(shot_mode ? 900 : 1440, shot_mode ? 760 : 900,
                                          "Picto Optimizer — harness", nullptr, nullptr);
    if (window == nullptr)
    {
        std::fprintf(stderr, "glfwCreateWindow falhou\n");
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    // Mesmo estilo e mesma fonte que o mod usa em jogo. O harness só serve
    // para julgar a aparência se for exatamente a mesma configuração.
    e33::ui::theme::apply_style();
    e33::ui::theme::load_fonts("assets");
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 150");

    e33::log::set_verbose(true);

    // Carrega os dados uma vez para montar a party de exemplo, depois entrega a
    // fonte ao controller.
    e33::GameData probe;
    probe.load(std::filesystem::path{kSampleDir} / "data" / "1.5.0");
    auto party = make_party(probe);

    auto* source = new e33::MockPartySource{party};
    e33::AppController app{std::unique_ptr<e33::IPartySource>{source}};
    app.initialize(std::filesystem::path{kSampleDir});

    e33::ui::OverlayState overlay;
    overlay.open = true;

    if (demo)
    {
        // Estado de vitrine: uma skill elemental contra o alvo que tem a
        // fraqueza correspondente, para a imagem mostrar o breakdown cheio em
        // vez de uma coluna de multiplicadores em 1.000.
        app.select_skill("sk_brasier");
        app.select_enemy("en_sirene");
        app.set_target_broken(true);
    }

    int frame = 0;

    while (glfwWindowShouldClose(window) == 0)
    {
        glfwPollEvents();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        app.tick(glfwGetTime());
        e33::ui::draw_overlay(app, overlay);
        if (!shot_mode)
        {
            draw_game_stub(app, *source, party);
        }

        if (!overlay.open)
        {
            ImGui::Begin("Overlay fechado");
            if (ImGui::Button("Reabrir overlay (hotkey em jogo)"))
            {
                overlay.open = true;
            }
            ImGui::End();
        }

        ImGui::Render();
        int width = 0;
        int height = 0;
        glfwGetFramebufferSize(window, &width, &height);
        glViewport(0, 0, width, height);
        glClearColor(0.03f, 0.028f, 0.025f, 1.0f); // obsidiana, como o fundo do jogo
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        if (shot_mode && ++frame >= shot_frame)
        {
            std::vector<std::uint8_t> pixels(
                static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4u);
            glPixelStorei(GL_PACK_ALIGNMENT, 1);
            glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
            if (e33::harness::write_bmp(shot_path, width, height, pixels))
            {
                std::printf("screenshot: %s (%dx%d)\n", shot_path.c_str(), width, height);
            }
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }

        glfwSwapBuffers(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
