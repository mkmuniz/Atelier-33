/**
 * M2 — transforma um dump do FModel em JSON tipado e validado.
 *
 *   pnpm extract --dump /caminho/para/FModel/Output/Exports --version 1.5.0
 *
 * Roda em qualquer SO. O dump em si é feito no Windows (FModel é .NET/WPF), mas
 * a transformação só lê arquivos: copie a pasta de exports e rode aqui.
 *
 * Falha ALTO quando uma coluna esperada some. É assim que se descobre que o
 * patch quebrou os dados antes dos usuários descobrirem — e é por isso que este
 * script roda no CI.
 */
import { parseArgs } from "node:util";
import { mkdir, readFile, writeFile } from "node:fs/promises";
import path from "node:path";

import {
  TABLE_NAMES,
  TABLE_SCHEMAS,
  configSchema,
  type TableConfig,
  type TableName,
} from "./schema.js";

export class ExtractionError extends Error {}

/**
 * Formato de DataTable exportado pelo FModel: um array com um objeto cujo campo
 * `Rows` mapeia nome da linha -> propriedades. Aceitamos também um objeto solto
 * com `Rows`, porque versões diferentes do FModel variam nesse envelope.
 */
export function readRows(raw: unknown, sourceName: string): Record<string, unknown> {
  const candidates = Array.isArray(raw) ? raw : [raw];
  for (const candidate of candidates) {
    if (candidate && typeof candidate === "object" && "Rows" in candidate) {
      const rows = (candidate as { Rows: unknown }).Rows;
      if (rows && typeof rows === "object" && !Array.isArray(rows)) {
        return rows as Record<string, unknown>;
      }
    }
  }
  throw new ExtractionError(
    `${sourceName}: nenhum DataTable com "Rows" encontrado. ` +
      `O FModel exportou outra coisa, ou o nome da tabela mudou no patch.`,
  );
}

/** Grava `value` em `target` seguindo um caminho pontuado ("stats.attack"). */
function assignPath(target: Record<string, unknown>, dotted: string, value: unknown): void {
  const parts = dotted.split(".");
  let node = target;
  for (const part of parts.slice(0, -1)) {
    if (typeof node[part] !== "object" || node[part] === null) {
      node[part] = {};
    }
    node = node[part] as Record<string, unknown>;
  }
  node[parts.at(-1)!] = value;
}

function normalizeElement(value: unknown): string {
  if (typeof value !== "string") return "none";
  // O dump traz enums como "EElement::Fire" ou "Fire".
  const leaf = value.split("::").at(-1) ?? value;
  return leaf.toLowerCase();
}

export function mapRow(
  id: string,
  row: Record<string, unknown>,
  config: TableConfig,
  table: TableName,
): Record<string, unknown> {
  const out: Record<string, unknown> = { id };

  const missing: string[] = [];
  for (const [field, column] of Object.entries(config.columns)) {
    const present = column in row;
    if (!present) {
      if (config.required.includes(field)) missing.push(`${field} (coluna "${column}")`);
      continue;
    }
    let value = row[column];
    if (field === "element" || field === "weakness") value = normalizeElement(value);
    assignPath(out, field, value);
  }

  if (missing.length > 0) {
    throw new ExtractionError(
      `${table}/${id}: coluna(s) obrigatoria(s) ausente(s): ${missing.join(", ")}. ` +
        `Se o patch renomeou a coluna, ajuste tools/tables.json.`,
    );
  }

  // Defaults do schema, aplicados aqui e não no C++, para que o arquivo gerado
  // seja auto-contido e legível.
  if (table === "skills") {
    out.hits ??= 1;
    out.can_crit ??= true;
    out.element ??= "none";
  }
  if (table === "weapons") out.element ??= "none";
  if (table === "enemies") {
    out.weakness ??= "none";
    out.defense ??= 0;
    out.weakness_multiplier ??= 1.5;
    out.break_multiplier ??= 1.5;
  }
  out.stats ??= {};
  return out;
}

export function extractTable(
  table: TableName,
  raw: unknown,
  config: TableConfig,
): unknown[] {
  const rows = readRows(raw, config.source);
  const schema = TABLE_SCHEMAS[table];

  const out: unknown[] = [];
  const problems: string[] = [];
  for (const [id, row] of Object.entries(rows)) {
    if (!row || typeof row !== "object") {
      problems.push(`${id}: linha nao e um objeto`);
      continue;
    }
    const mapped = mapRow(id, row as Record<string, unknown>, config, table);
    const parsed = schema.safeParse(mapped);
    if (!parsed.success) {
      problems.push(`${id}: ${parsed.error.issues.map((i) => `${i.path.join(".")} ${i.message}`).join("; ")}`);
      continue;
    }
    out.push(parsed.data);
  }

  if (out.length === 0) {
    throw new ExtractionError(
      `${table}: nenhuma linha valida em ${rows ? Object.keys(rows).length : 0} linha(s).\n` +
        problems.slice(0, 5).map((p) => `  - ${p}`).join("\n"),
    );
  }
  // Uma linha torta é aceitável; metade da tabela torta é um patch quebrando os
  // dados, e passar isso adiante em silêncio é como o usuário acaba com um
  // otimizador que recomenda a build errada.
  if (problems.length > out.length * 0.1) {
    throw new ExtractionError(
      `${table}: ${problems.length} linha(s) rejeitada(s) contra ${out.length} aceita(s).\n` +
        problems.slice(0, 5).map((p) => `  - ${p}`).join("\n"),
    );
  }
  for (const problem of problems) {
    console.warn(`aviso: ${table}/${problem}`);
  }
  return out;
}

async function main(): Promise<void> {
  const { values } = parseArgs({
    options: {
      dump: { type: "string" },
      version: { type: "string" },
      config: { type: "string", default: "tools/tables.json" },
      out: { type: "string", default: "data" },
    },
  });

  if (!values.dump) {
    console.error("uso: pnpm extract --dump <pastaExportsDoFModel> [--version 1.5.0]");
    process.exitCode = 1;
    return;
  }

  const config = configSchema.parse(JSON.parse(await readFile(values.config!, "utf8")));
  const version = values.version ?? config.version;
  const outDir = path.join(values.out!, version);
  await mkdir(outDir, { recursive: true });

  for (const table of TABLE_NAMES) {
    const tableConfig = config.tables[table];
    if (!tableConfig) {
      throw new ExtractionError(`tools/tables.json nao define a tabela "${table}"`);
    }
    const sourcePath = path.join(values.dump, `${tableConfig.source}.json`);
    const raw = JSON.parse(await readFile(sourcePath, "utf8"));
    const rows = extractTable(table, raw, tableConfig);

    const payload = { [table]: rows };
    await writeFile(path.join(outDir, `${table}.json`), `${JSON.stringify(payload, null, 2)}\n`);
    console.log(`${table}: ${rows.length} linha(s) -> ${path.join(outDir, `${table}.json`)}`);
  }
}

// Só executa como CLI; o módulo é importado pelos testes.
if (process.argv[1] && import.meta.url.endsWith(path.basename(process.argv[1]))) {
  main().catch((error: unknown) => {
    console.error(error instanceof Error ? error.message : String(error));
    process.exitCode = 1;
  });
}
