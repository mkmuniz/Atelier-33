-- E33 Picto Optimizer — UE4SS C++ mod
--
-- PRE-REQUISITO: xmake f --ue4ss=C:/path/to/RE-UE4SS
-- Só compila no Windows/MSVC: o target é uma DLL carregada pelo UE4SS.

set_xmakever("2.8.0")
set_languages("c++23")
set_arch("x64")

option("ue4ss")
    set_default("")
    set_showmenu(true)
    set_description("Caminho para o checkout do RE-UE4SS")
option_end()

target("PictoOptimizer")
    set_kind("shared")
    set_basename("main")

    add_files("src/**.cpp")
    add_includedirs("src")

    on_load(function (target)
        local sdk = get_config("ue4ss")
        if sdk and sdk ~= "" then
            target:add("includedirs", path.join(sdk, "UE4SS/include"))
            target:add("includedirs", path.join(sdk, "deps/first/File/include"))
            target:add("includedirs", path.join(sdk, "deps/first/DynamicOutput/include"))
            target:add("includedirs", path.join(sdk, "deps/third/imgui"))
        end
    end)

    if is_plat("windows") then
        add_defines("NOMINMAX", "WIN32_LEAN_AND_MEAN")
        add_cxflags("/utf-8", "/EHsc")
    end

-- Calc/ não conhece ImGui nem ponteiro do Unreal, então roda em teste nativo
-- (inclusive no macOS). É aqui que as fixtures de dano são validadas.
target("calc_tests")
    set_kind("binary")
    set_default(false)
    add_files("src/Calc/*.cpp", "tests/*.cpp")
    add_includedirs("src")
