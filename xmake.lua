-- E33 Picto Optimizer — UE4SS C++ mod
--
-- Tres targets (a DLL do mod e CMake; ver CMakeLists.txt):
--   tests           Calc/, Optimizer/ e Data/. Roda em qualquer SO, sem o jogo.
--   bench           Tempo da busca: o plano exige < 3s num caso tipico.
--   harness         Preview nativo do overlay em ImGui, sem o jogo.
--
-- Só a DLL depende de Windows. Ver docs/DEV-MACOS.md.

set_xmakever("2.8.0")
set_languages("c++23")
set_allowedmodes("debug", "release")
add_rules("mode.debug", "mode.release")

add_requires("nlohmann_json")
add_requires("doctest")
-- Só o harness nativo precisa de ImGui + GLFW; a DLL usa o ImGui do UE4SS.
add_requires("imgui", {configs = {glfw = true, opengl3 = true}})

-- A regra de camadas do plano, expressa no build: nada nesta lista inclui
-- ImGui ou header do Unreal, e é por isso que ela compila e roda no macOS.
local core_files = {
    "src/Support/*.cpp",
    "src/Model/*.cpp",
    "src/Config/*.cpp",
    "src/Core/*.cpp",
    "src/Data/*.cpp",
    "src/Calc/*.cpp",
    "src/Optimizer/*.cpp",
}

-- A DLL do mod NAO e construida aqui.
--
-- O fluxo suportado pelo UE4SS e CMake, compilando o mod junto com o RE-UE4SS
-- (add_subdirectory); nao ha import library publicada para linkar de fora. Ver
-- CMakeLists.txt na raiz. Este arquivo cuida so dos alvos nativos, que rodam
-- em qualquer sistema e nao precisam do jogo.

target("tests")
    set_kind("binary")
    set_rundir("$(projectdir)")
    add_files("tests/*.cpp")
    add_files(core_files)
    add_includedirs("src")
    add_packages("nlohmann_json", "doctest")

target("bench")
    set_kind("binary")
    set_default(false)
    set_rundir("$(projectdir)")
    add_files("bench/*.cpp")
    add_files(core_files)
    add_includedirs("src")
    add_packages("nlohmann_json")

target("harness")
    set_kind("binary")
    set_default(false)
    set_rundir("$(projectdir)")
    add_files("harness/*.cpp", "src/UI/*.cpp")
    add_files(core_files)
    add_includedirs("src")
    add_packages("nlohmann_json", "imgui")
    if is_plat("macosx") then
        add_frameworks("OpenGL", "Cocoa", "IOKit", "CoreVideo")
    end

