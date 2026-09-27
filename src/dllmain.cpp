// Entrada da DLL.
//
// Carregada de duas formas, e as duas funcionam com o mesmo binário:
//
//   1. Proxy: a DLL se chama version.dll e fica ao lado do executável do jogo.
//      O Windows a carrega no lugar da do sistema, e os exports abaixo
//      repassam tudo para a verdadeira.
//   2. Injetor: qualquer injetor de DLL. DllMain faz o resto.
//
// Por que version.dll e não dinput8.dll: o Repertoire 33 usa dinput8, e dois
// mods não podem ser o mesmo arquivo na mesma pasta. version.dll é o proxy
// clássico para isso — poucos exports, nenhum no caminho de gráficos ou de
// input, então repassá-los não interfere em nada sensível.

#include "ModHost.hpp"

#if defined(_WIN32)

#include <Windows.h>
#include <winver.h>

namespace
{
HMODULE g_system_version{nullptr};

// Carrega a version.dll de verdade, do diretório do sistema. Sob demanda: se o
// jogo nunca pedir versão de arquivo, a DLL do sistema nem é tocada.
HMODULE system_version()
{
    if (g_system_version != nullptr)
    {
        return g_system_version;
    }
    wchar_t path[MAX_PATH]{};
    const auto length = GetSystemDirectoryW(path, MAX_PATH);
    if (length == 0 || length > MAX_PATH - 16)
    {
        return nullptr;
    }
    lstrcatW(path, L"\\version.dll");
    g_system_version = LoadLibraryW(path);
    return g_system_version;
}

template <typename Fn>
Fn original(const char* name)
{
    HMODULE module = system_version();
    return module == nullptr ? nullptr : reinterpret_cast<Fn>(GetProcAddress(module, name));
}

DWORD WINAPI bootstrap(LPVOID module)
{
    // Fora do loader lock: DllMain não pode carregar outras DLLs nem criar
    // dispositivos D3D, e a instalação dos hooks faz as duas coisas.
    e33::ModHost::instance().start(static_cast<HMODULE>(module));
    return 0;
}
} // namespace

// Repasse de cada export da version.dll. Um falho devolve o valor de erro que
// a API documenta, nunca lixo: quem chama espera poder testar GetLastError.
#define FORWARD(ret, name, params, args, failure)                                        \
    extern "C" __declspec(dllexport) ret WINAPI name params                              \
    {                                                                                    \
        using Fn = ret(WINAPI*) params;                                                   \
        const auto fn = original<Fn>(#name);                                             \
        return fn == nullptr ? (failure) : fn args;                                      \
    }

FORWARD(BOOL, GetFileVersionInfoA,
        (LPCSTR f, DWORD h, DWORD len, LPVOID data), (f, h, len, data), FALSE)
FORWARD(BOOL, GetFileVersionInfoW,
        (LPCWSTR f, DWORD h, DWORD len, LPVOID data), (f, h, len, data), FALSE)
FORWARD(BOOL, GetFileVersionInfoExA,
        (DWORD flags, LPCSTR f, DWORD h, DWORD len, LPVOID data), (flags, f, h, len, data), FALSE)
FORWARD(BOOL, GetFileVersionInfoExW,
        (DWORD flags, LPCWSTR f, DWORD h, DWORD len, LPVOID data), (flags, f, h, len, data), FALSE)
FORWARD(DWORD, GetFileVersionInfoSizeA, (LPCSTR f, LPDWORD h), (f, h), 0)
FORWARD(DWORD, GetFileVersionInfoSizeW, (LPCWSTR f, LPDWORD h), (f, h), 0)
FORWARD(DWORD, GetFileVersionInfoSizeExA, (DWORD flags, LPCSTR f, LPDWORD h), (flags, f, h), 0)
FORWARD(DWORD, GetFileVersionInfoSizeExW, (DWORD flags, LPCWSTR f, LPDWORD h), (flags, f, h), 0)
// Estes quatro sao declarados em winver.h com ponteiros NAO-const. Divergir da
// declaracao do sistema e erro de compilacao, entao a assinatura e copiada dali
// como esta, const ausente incluido.
FORWARD(DWORD, VerFindFileA,
        (DWORD flags, LPSTR name, LPSTR win, LPSTR app, LPSTR cur, PUINT curlen, LPSTR dest,
         PUINT destlen),
        (flags, name, win, app, cur, curlen, dest, destlen), 0)
FORWARD(DWORD, VerFindFileW,
        (DWORD flags, LPWSTR name, LPWSTR win, LPWSTR app, LPWSTR cur, PUINT curlen,
         LPWSTR dest, PUINT destlen),
        (flags, name, win, app, cur, curlen, dest, destlen), 0)
FORWARD(DWORD, VerInstallFileA,
        (DWORD flags, LPSTR src, LPSTR dest, LPSTR srcdir, LPSTR destdir, LPSTR curdir,
         LPSTR tmp, PUINT tmplen),
        (flags, src, dest, srcdir, destdir, curdir, tmp, tmplen), 0)
FORWARD(DWORD, VerInstallFileW,
        (DWORD flags, LPWSTR src, LPWSTR dest, LPWSTR srcdir, LPWSTR destdir, LPWSTR curdir,
         LPWSTR tmp, PUINT tmplen),
        (flags, src, dest, srcdir, destdir, curdir, tmp, tmplen), 0)
FORWARD(DWORD, VerLanguageNameA, (DWORD lang, LPSTR name, DWORD size), (lang, name, size), 0)
FORWARD(DWORD, VerLanguageNameW, (DWORD lang, LPWSTR name, DWORD size), (lang, name, size), 0)
FORWARD(BOOL, VerQueryValueA,
        (LPCVOID block, LPCSTR sub, LPVOID* out, PUINT len), (block, sub, out, len), FALSE)
FORWARD(BOOL, VerQueryValueW,
        (LPCVOID block, LPCWSTR sub, LPVOID* out, PUINT len), (block, sub, out, len), FALSE)

#undef FORWARD

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID)
{
    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(module);
        if (HANDLE thread = CreateThread(nullptr, 0, bootstrap, module, 0, nullptr);
            thread != nullptr)
        {
            CloseHandle(thread);
        }
        break;
    case DLL_PROCESS_DETACH:
        e33::ModHost::instance().stop();
        break;
    default:
        break;
    }
    return TRUE;
}

#endif
