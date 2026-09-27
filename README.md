# E33 Picto Optimizer

In-game overlay for **Clair Obscur: Expedition 33**: reads your live build and
searches for a better picto/lumina combination. UE4SS C++ mod with a draggable
ImGui window.

> **Status: pre-alpha (M0).** Not installable yet.

## Why an overlay and not an external window

Tools like Mobalytics are external always-on-top windows because League and TFT
have anti-cheat and forbid injection. E33 is single-player with no anti-cheat and
a mature modding platform (UE4SS), so the overlay reads game state **directly from
memory, live** — no save parsing, no OCR. You open the overlay and your pictos are
already there. It also works in exclusive fullscreen, and ImGui gives dragging,
resizing, collapsing and persisted position for free.

## Installation

**Requires** UE4SS installed in `Expedition 33\Sandfall\Binaries\Win64\`.

1. Extract the `PictoOptimizer` folder to
   `Expedition 33\Sandfall\Binaries\Win64\ue4ss\Mods\PictoOptimizer\`
2. Confirm `enabled.txt` exists inside the folder.
3. Start the game.
4. Press **F8** to open the overlay.

`J` is avoided on purpose — Gramophone Everywhere uses it.

Tested game version: _TBD_. Damage formula mean error: _TBD_.

## Building

```sh
xmake f --ue4ss=C:/path/to/RE-UE4SS -m release
xmake
```

Windows/MSVC only for the mod. The `calc_tests` target and `tools/` run anywhere.

## Data extraction

```sh
pnpm install
pnpm extract --version 1.5.0 --dump /path/to/FModel/Output/Exports
```

Validated against a schema and wired into CI, so a patch that renames a column
fails the build instead of shipping.

## Roadmap

| Milestone | Scope |
|---|---|
| M0 | Lua prototype: print the name of an equipped picto |
| M1 | Empty draggable overlay with configurable hotkey |
| M2 | `tools/extract.ts` — FModel dump to validated JSON |
| M3 | Live state: party, pictos, luminas, weapon, stats |
| M4 | Damage formula with per-multiplier breakdown (<2% mean error) |
| M5 | Optimizer: dominance pruning, branch and bound, beam search |
| M6 | Side-by-side compare, compact mode, export build as text |

M3 is the first publishable version: an overlay that only shows your build already
has an audience, and gives feedback before the expensive part is written.

## Layers

`Calc/` knows nothing about ImGui. `UI/` knows nothing about Unreal object
pointers. When a patch breaks the mod, the damage is in `Game/`.

## License

MIT — see [LICENSE](LICENSE).
