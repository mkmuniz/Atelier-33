import assert from "node:assert/strict";
import { test } from "node:test";

import { ExtractionError, extractTable, mapRow, readRows } from "./extract.ts";
import type { TableConfig } from "./schema.ts";

const pictoConfig: TableConfig = {
  source: "DT_jRPG_Pictos",
  required: ["name", "lumina_cost"],
  columns: {
    name: "DisplayName",
    lumina_cost: "LuminaCost",
    "stats.attack": "BonusAttack",
    "stats.crit_rate": "BonusCritChance",
  },
};

// Envelope como o FModel exporta um DataTable.
function dump(rows: Record<string, unknown>): unknown {
  return [{ Type: "DataTable", Name: "DT_jRPG_Pictos", Rows: rows }];
}

test("le o envelope de DataTable do FModel", () => {
  const rows = readRows(dump({ pic_a: { DisplayName: "A" } }), "DT_jRPG_Pictos");
  assert.deepEqual(Object.keys(rows), ["pic_a"]);
});

test("aceita tambem o objeto solto, porque o envelope varia entre versoes do FModel", () => {
  const rows = readRows({ Rows: { pic_a: {} } }, "DT_jRPG_Pictos");
  assert.deepEqual(Object.keys(rows), ["pic_a"]);
});

test("dump sem Rows falha dizendo o que provavelmente aconteceu", () => {
  assert.throws(
    () => readRows([{ Type: "Texture2D" }], "DT_jRPG_Pictos"),
    (error: unknown) => error instanceof ExtractionError && /Rows/.test(String(error)),
  );
});

test("mapeia colunas para o schema do mod, inclusive aninhadas", () => {
  const mapped = mapRow(
    "pic_lame",
    { DisplayName: "Lame", LuminaCost: 3, BonusAttack: 120, BonusCritChance: 0.1 },
    pictoConfig,
    "pictos",
  );
  assert.deepEqual(mapped, {
    id: "pic_lame",
    name: "Lame",
    lumina_cost: 3,
    stats: { attack: 120, crit_rate: 0.1 },
  });
});

test("coluna obrigatoria ausente falha alto e aponta o tables.json", () => {
  assert.throws(
    () => mapRow("pic_x", { DisplayName: "X" }, pictoConfig, "pictos"),
    (error: unknown) =>
      error instanceof ExtractionError &&
      /lumina_cost/.test(String(error)) &&
      /tables\.json/.test(String(error)),
  );
});

test("coluna opcional ausente apenas nao aparece na saida", () => {
  const mapped = mapRow("pic_x", { DisplayName: "X", LuminaCost: 2 }, pictoConfig, "pictos");
  assert.deepEqual(mapped, { id: "pic_x", name: "X", lumina_cost: 2, stats: {} });
});

test("enum do Unreal e normalizado: EElement::Fire vira fire", () => {
  const config: TableConfig = {
    source: "DT_jRPG_Skills",
    required: ["name", "power"],
    columns: { name: "DisplayName", power: "DamageMultiplier", element: "Element" },
  };
  const mapped = mapRow(
    "sk",
    { DisplayName: "Brasier", DamageMultiplier: 1.8, Element: "EElement::Fire" },
    config,
    "skills",
  );
  assert.equal(mapped.element, "fire");
  assert.equal(mapped.hits, 1, "hits ausente vira 1");
  assert.equal(mapped.can_crit, true);
});

test("tabela valida sai validada e completa", () => {
  const rows = extractTable(
    "pictos",
    dump({
      pic_a: { DisplayName: "A", LuminaCost: 3, BonusAttack: 100 },
      pic_b: { DisplayName: "B", LuminaCost: 1, BonusCritChance: 0.2 },
    }),
    pictoConfig,
  );
  assert.equal(rows.length, 2);
  assert.deepEqual(rows[0], { id: "pic_a", name: "A", lumina_cost: 3, stats: { attack: 100 } });
});

test("uma linha torta entre muitas e tolerada", () => {
  const rows: Record<string, unknown> = { ruim: { DisplayName: "", LuminaCost: 1 } };
  for (let i = 0; i < 30; i += 1) {
    rows[`pic_${i}`] = { DisplayName: `P${i}`, LuminaCost: 1 };
  }
  assert.equal(extractTable("pictos", dump(rows), pictoConfig).length, 30);
});

test("metade da tabela torta e um patch quebrando os dados, nao um caso a tolerar", () => {
  const rows: Record<string, unknown> = {};
  for (let i = 0; i < 10; i += 1) {
    rows[`ok_${i}`] = { DisplayName: `P${i}`, LuminaCost: 1 };
    rows[`ruim_${i}`] = { DisplayName: "", LuminaCost: 1 };
  }
  assert.throws(
    () => extractTable("pictos", dump(rows), pictoConfig),
    (error: unknown) => error instanceof ExtractionError && /rejeitada/.test(String(error)),
  );
});

test("tabela vazia falha em vez de gerar um arquivo que o mod carrega vazio", () => {
  assert.throws(
    () => extractTable("pictos", dump({}), pictoConfig),
    (error: unknown) => error instanceof ExtractionError,
  );
});

test("taxa de critico fora de 0..1 e rejeitada: e o sintoma de percentual virando fracao", () => {
  const rows: Record<string, unknown> = {};
  for (let i = 0; i < 20; i += 1) {
    rows[`pic_${i}`] = { DisplayName: `P${i}`, LuminaCost: 1, BonusCritChance: 25 };
  }
  assert.throws(
    () => extractTable("pictos", dump(rows), pictoConfig),
    (error: unknown) => error instanceof ExtractionError && /crit_rate/.test(String(error)),
  );
});
