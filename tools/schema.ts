import { z } from "zod";

/**
 * Schemas de saída: é contra isto que o C++ em src/Data/GameData.cpp lê. Se os
 * dois divergirem, o mod carrega uma tabela vazia em silêncio, que é o pior
 * modo de falhar — por isso a validação acontece aqui, na geração.
 */
export const ELEMENTS = [
  "none",
  "fire",
  "ice",
  "lightning",
  "earth",
  "light",
  "dark",
] as const;

export const elementSchema = z.enum(ELEMENTS);

export const statsSchema = z
  .object({
    attack: z.number().optional(),
    defense: z.number().optional(),
    health: z.number().optional(),
    speed: z.number().optional(),
    // Frações, nunca percentuais: 0.25 e não 25. O C++ assume o mesmo, e a
    // divergência entre os dois inflaria o dano estimado em cem vezes.
    crit_rate: z.number().min(-1).max(1).optional(),
    crit_damage: z.number().optional(),
    damage_bonus: z.number().optional(),
    element_bonus: z.record(elementSchema, z.number()).optional(),
  })
  .strict();

export const pictoSchema = z
  .object({
    id: z.string().min(1),
    name: z.string().min(1),
    lumina_cost: z.number().int().min(0),
    stats: statsSchema,
  })
  .strict();

export const luminaSchema = z
  .object({
    id: z.string().min(1),
    name: z.string().min(1),
    cost: z.number().int().min(0),
    stats: statsSchema,
  })
  .strict();

export const weaponSchema = z
  .object({
    id: z.string().min(1),
    name: z.string().min(1),
    element: elementSchema,
    stats: statsSchema,
  })
  .strict();

export const skillSchema = z
  .object({
    id: z.string().min(1),
    name: z.string().min(1),
    power: z.number(),
    element: elementSchema,
    hits: z.number().int().min(1),
    can_crit: z.boolean(),
  })
  .strict();

export const enemySchema = z
  .object({
    id: z.string().min(1),
    name: z.string().min(1),
    defense: z.number().min(0),
    weakness: elementSchema,
    weakness_multiplier: z.number().min(0),
    break_multiplier: z.number().min(0),
  })
  .strict();

export const TABLE_SCHEMAS = {
  pictos: pictoSchema,
  luminas: luminaSchema,
  weapons: weaponSchema,
  skills: skillSchema,
  enemies: enemySchema,
} as const;

export type TableName = keyof typeof TABLE_SCHEMAS;
export const TABLE_NAMES = Object.keys(TABLE_SCHEMAS) as TableName[];

/** Mapeamento de colunas, lido de tools/tables.json. */
export const tableConfigSchema = z
  .object({
    source: z.string().min(1),
    required: z.array(z.string()).default([]),
    columns: z.record(z.string(), z.string()),
  })
  .strict();

export const configSchema = z.object({
  version: z.string().min(1),
  tables: z.record(z.string(), tableConfigSchema),
});

export type TableConfig = z.infer<typeof tableConfigSchema>;
