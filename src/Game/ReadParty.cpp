#include "Game/ReadParty.hpp"

#include "Support/Log.hpp"

namespace e33
{
// TODO(M0 e M3 — só possível com o jogo aberto): navegar do objeto de
// personagem até a lista de pictos equipados e ler nome, stats e luminas.
//
// M0 é o primeiro milestone do projeto justamente porque decide a viabilidade:
// se não der para achar os pictos na memória, o plano B documentado é voltar ao
// parsing de save. O caminho recomendado:
//   1. Protótipo em Lua (recarrega sem fechar o jogo): percorrer os objetos do
//      Unreal procurando a classe de personagem e imprimir as propriedades.
//   2. Confirmar que o id dos pictos bate com a tabela extraída pelo
//      tools/extract.ts, senão o otimizador cruza dados errados em silêncio.
//   3. Só então portar para cá.
//
// Pronto quando: trocar um picto no menu do jogo atualiza o overlay sozinho.

PartySnapshot UnrealPartySource::read()
{
    PartySnapshot snapshot;
    snapshot.valid = false;
    snapshot.error = "leitura do estado do jogo ainda nao implementada (M0/M3)";
    return snapshot;
}

bool UnrealPartySource::available() const
{
    return false;
}
} // namespace e33
