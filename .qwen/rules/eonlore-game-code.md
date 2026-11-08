---
description: Eonlore game code rules (layers, folders, conventions), applied when reading or editing game sources and rule tests.
paths:
  - "Game/src/**/*.h"
  - "Game/src/**/*.cpp"
  - "Tools/Tests/**"
---
# Eonlore game code rules

## Layers: put each thing where it belongs

| Layer | Folder | Rule |
| --- | --- | --- |
| Data | `Data/` (`HY_DATA` types), files in `Project/Assets/Data/` | Anything to tune, translate or balance. Read through a property that defaults to the file |
| State | `State/`, a member of `RpgSession` | What the run remembers. Plain C++; refers to content by `DataRowRef` / `AssetHandle`, never by pointer. **Never** in a script member or a `static` |
| Rules | `Rules/` (`class <Area>Rules`, static functions) | Pure: data, state and random rolls in, a result struct out. No entities, UI, effects, cameras or statics. Tested in `Tools/Tests/GameRulesTest.cpp` |
| Scripts | `Scripts/Common/` (shared), `Scripts/<Area>/` (one area) | Orchestrate: input and events in, rules called, state changed, views and effects out. No rule maths inline |
| Views | `Views/` (shared) or the area folder | Show state on UI entities; never change state. A plain class the script owns: `Bind(...)` once in `OnCreate`, then `Present`/`Show` |

- A view never includes a script (`FieldDirectorScript.h` includes the views: the other way round is an include loop). Text it shows from `GetFieldText` is passed in by the director.
- Rules include only `Data/`, other `Rules/` and plain `State/` headers (`State/Inventory.h`), never `"Hydro.h"`: the rule tests build without the engine's renderer, so a header that pulls in `Hydro.h` (like `State/QuestLog.h`) can't be included in `GameRulesTest.cpp`.
- An area folder (`Scripts/ShrineGlade/`, ...) never includes another area's files. `Scripts/Common/`, `Field/` and `Battle/` are shared. Used by two areas: move it to the shared folder.
- One script of a type per entity.
- Reaching another object: an `EntityRef` / `ComponentRef<T>` property for what is known while authoring; `other.TryGetComponent<T>()` for what is known only at runtime (a collision's `other`); run state for things that shouldn't know each other. No new `FindEntityByTag` lookups outside a view's `Bind`.
- Data on an entity without behaviour is an `HY_COMPONENT()` struct, not a script.

## Serialized names are an API

Scenes and data files save script class names, property names, data type names and field names. Never rename one without `FormerName("Old")` in its `HY_SCRIPT(...)` / `HY_PROPERTY(...)` / `HY_DATA(...)`, and tell the user to re-save the scenes.

## C++ conventions (the linter fails on the first four)

- **C++17** (`cppdialect "C++17"`): no `std::span`, `std::format`, ranges, concepts or designated initialisers; no `fmt::format`.

- No `!` operator: `== false`, `== nullptr` (`!=` is fine).
- Braces on every `if` / `else` / `for` / `while` body, each brace on its own line.
- Everything in `namespace Hydro`; no other, nested or anonymous namespaces; file-local helpers are `static`.
- Members `m_PascalCase`, statics `s_PascalCase`; types, methods and public fields PascalCase.
- Names: scripts `<Name>Script`, views `<Name>View`, rules `<Area>Rules`, state named for what it holds. Never the suffixes `System`, `Service`, `Handler`, `Helper`, `Controller`, `Module`.
- `#pragma once`; game includes from `Game/src` (`"State/Inventory.h"`), engine as `"Hydro.h"` or `"Hydro/..."`; forward slashes, no `../`.
- Comments: at most two lines, say why; no dates, no plan IDs. Lines up to 140 columns.
- Tabs, CRLF. Don't reformat lines you don't change.
