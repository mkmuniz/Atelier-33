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
//   1. Gerar o SDK do jogo com o Dumper-7 (público) e procurar nele a classe de
//      personagem. O SDK dá os offsets dos campos, que é o que substitui a API
//      de reflexão do UE4SS.
//   2. Confirmar que o id dos pictos bate com a tabela extraída pelo
//      tools/extract.ts, senão o otimizador cruza dados errados em silêncio.
//      Os dois vêm do mesmo jogo, mas por caminhos diferentes — dump de
//      arquivo contra leitura de memória — e podem divergir num patch.
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
