/**
 * M2 — transforma um dump do FModel em JSON tipado e validado.
 *
 *   pnpm extract --version 1.5.0 --dump /path/to/FModel/Output/Exports
 *
 * Roda em qualquer SO: o dump do FModel é feito no Windows, mas a transformação
 * é só leitura de arquivo. Copie a pasta de exports para cá e rode aqui.
 *
 * Falha ALTO se uma coluna esperada sumiu — é assim que se descobre que o patch
 * quebrou os dados antes dos usuários descobrirem. Por isso vai no CI.
 */
import { parseArgs } from "node:util";
import { mkdir, writeFile } from "node:fs/promises";
import path from "node:path";

const TABLES = ["pictos", "luminas", "weapons", "skills", "enemies"] as const;
type Table = (typeof TABLES)[number];

const { values } = parseArgs({
  options: {
    version: { type: "string" },
    dump: { type: "string" },
  },
});

if (!values.version) {
  console.error("usage: pnpm extract --version <gameVersion> [--dump <fmodelExportsDir>]");
  process.exit(1);
}

const gameVersion = values.version;
const outDir = path.join("data", gameVersion);

async function extract(table: Table): Promise<unknown[]> {
  // TODO(M2): localizar o DataTable correspondente no dump, normalizar as
  // colunas e validar com zod. Para os encontros, a tabela é DT_jRPG_Encounters.
  throw new Error(`extractor for "${table}" not implemented yet`);
}

async function main(): Promise<void> {
  await mkdir(outDir, { recursive: true });
  for (const table of TABLES) {
    const rows = await extract(table);
    await writeFile(path.join(outDir, `${table}.json`), `${JSON.stringify(rows, null, 2)}\n`);
    console.log(`${table}: ${rows.length} rows`);
  }
}

await main();
