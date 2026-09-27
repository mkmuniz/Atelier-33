# Fixtures de dano (M4)

Cada arquivo `.json` aqui é **uma medição real feita em jogo**. O teste
`as fixtures reais do repositorio...` compara a previsão da fórmula com essas
medições e falha se o erro médio passar de 2%. Enquanto esta pasta só tiver este
README, o teste passa sem medir nada — e o README do projeto diz que a fórmula
não está calibrada, em vez de anunciar uma precisão que ninguém verificou.

## Formato

```json
{
  "id": "sirene_fogo_break",
  "note": "Maelle, Rapière, sem buff. Boss em break, golpe nao critico.",
  "skill": "sk_brasier",
  "enemy": "en_sirene",
  "broken": true,
  "kind": "non_crit",
  "base_stats": { "attack": 320, "crit_rate": 0.05 },
  "pictos": ["pic_lame", "pic_flamme"],
  "luminas": ["lum_force"],
  "observed_damage": 4821
}
```

- `kind`: `non_crit` (padrão), `crit` ou `expected`. O crítico é aleatório, então
  a fixture precisa dizer qual dos três números foi anotado.
- ids de picto, lumina, skill e inimigo têm de existir em `data/<versao>/`. Uma
  fixture que cita um id inexistente é pulada, não medida contra outra build.

## Como coletar sem jogar 40 horas

1. Save com todos os bosses liberados (Nexus) como save de desenvolvimento.
2. Save editor para montar builds específicas rápido.
3. Vinte fixtures bem escolhidas valem mais que duzentas aleatórias. O que se
   quer cobrir são os multiplicadores, não repetições do mesmo caso:

| Cobertura | Por quê |
|---|---|
| sem break / com break | isola o multiplicador de break |
| sem fraqueza / com fraqueza | isola o multiplicador elemental |
| não crítico / crítico | separa o multiplicador de crítico da base |
| um buff / buff empilhado | pega teto de stack errado |
| alvo de defesa baixa / alta | valida a curva de mitigação |
| skill de 1 golpe / vários golpes | pega dano por golpe vs total |

4. Publique o erro médio no README. É o número que separa uma ferramenta de um
   chute bem-apresentado.
