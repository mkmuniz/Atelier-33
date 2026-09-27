#include "Mod.hpp"

#include <Mod/CppUserModBase.hpp>

#define PICTOOPT_API __declspec(dllexport)

extern "C" {
PICTOOPT_API RC::CppUserModBase* start_mod()
{
    return new e33::PictoOptimizerMod{};
}

PICTOOPT_API void uninstall_mod(RC::CppUserModBase* mod)
{
    delete mod;
}
}
