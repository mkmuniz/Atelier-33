# Como testar este mod

Duas metades: o que dá para verificar aqui, sem o jogo, e o que só o PC com o
jogo responde.

## No MacBook, agora

```sh
xmake f -y                                  # baixa nlohmann_json, doctest, imgui, glfw
xmake build tests && xmake run tests        # suíte de lógica
xmake build bench && xmake run bench        # tempo da busca contra o alvo de 3s
xmake build harness && xmake run harness    # o overlay numa janela nativa

pnpm install && pnpm test && pnpm typecheck # o extrator
```

### O que a suíte cobre

| Área | O que é verificado |
|---|---|
| `Calc/Formula` | o produto do breakdown reproduz o total; fraqueza, break, crítico e seus limites |
| `Calc/Buffs` | teto de empilhamento; slot vazio não quebra a soma |
| `Calc/Fixtures` | erro relativo contra dano medido; fixture com id inexistente é pulada |
| `Optimizer/Prune` | dominado sai, trade-off fica, cópias idênticas deixam uma |
| `Optimizer/Search` | **o resultado bate com força bruta**, inclusive com orçamento de lumina |
| `Optimizer/Job` | thread, cancelamento, resultado colhido uma vez só |
| `Data/GameData` | linha torta é pulada, arquivo ausente é erro reportado |
| `Core/AppController` | troca de picto no menu reflete no overlay sozinha; diff do que trocar |

O teste que mais importa é o `branch and bound chega no mesmo otimo que a forca
bruta`. Ele já pegou três bugs que devolviam silenciosamente a build errada — e
é o único jeito de saber que a poda não está descartando a resposta.

### O que fazer no harness

1. `xmake run harness` abre o overlay e a janela "Jogo simulado".
2. Aba **Build**: escolha Skill e Alvo. O breakdown mostra cada multiplicador;
   os fatores multiplicados têm de dar o número de cima.
3. Marque **Alvo em break** e veja o dano subir pelo multiplicador do inimigo.
4. Na janela do jogo simulado, marque ou desmarque um picto de Maelle. O overlay
   atualiza sozinho em ~0,25s — é a definição do M3.
5. Aba **Otimizar** → **Otimizar**. Veja progresso e o rótulo do resultado:
   "Otimo provado" ou "Parou no teto de busca".
6. Aba **Comparar**: a lista exata de o que equipar e o que remover.
7. Ajustes → **Modo compacto**: o overlay vira só o número.

## No PC com o jogo (o que falta)

Nesta ordem:

1. **M0** — achar os pictos equipados na memória (`src/Game/ReadParty.cpp`).
   Protótipo em Lua primeiro. **Este milestone decide a viabilidade**: se não
   der, o plano B documentado é voltar ao parsing de save.
2. **Pré-requisito do overlay** — confirmar como a versão de UE4SS usada registra
   uma janela ImGui própria sobre o jogo. Hoje o mod registra uma aba
   (`register_tab`), que sempre funciona mas exige a janela do UE4SS aberta.
3. **M2 de dados** — rodar o FModel, copiar os exports e
   `pnpm extract --dump <exports> --version <versao>`. Ajustar
   `tools/tables.json` até o extrator parar de reclamar de coluna ausente: as
   colunas ali são um chute informado, não uma leitura do dump real.
4. **M4** — coletar as fixtures de dano (ver `tests/fixtures/README.md`) e
   ajustar `data/<versao>/formula.json` até o erro médio ficar abaixo de 2%.
   Só então publicar o número no README.
5. **Bench de novo no PC** — os 0,23s medidos aqui são de um M4 Mac; o número que
   vale para o usuário é o da máquina dele, em release.

Ambiente que economiza horas: save com todos os bosses liberados, save editor
para montar builds rápido, modo janela sem borda e o console do UE4SS aberto.
