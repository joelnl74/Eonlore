# Eonlore: Qwen (local model) instructions

Eonlore is a JRPG on the Hydro engine. This repository holds the game only: game code in `Game/src` (C++17, header-only scripts), the project and content in `Project/` (`Assets/Data/*.hytable`, `*.hydef`, scenes), rule tests in `Tools/Tests/GameRulesTest.cpp`. The engine is a separate repository next to this one: `..\Hydro`.

## Your job: game scripting with the user

You write and change **game code**: scripts, views, run state, rules and their tests, data types. You work with the user in small steps.

**Engine work is not yours.** Stop and use the `hydro-handoff` skill (a brief for a frontier model) when the task needs any of these:
- a change under `..\Hydro` (engine, editor, runtime, renderer, physics, UI system, reflection generator)
- a new engine API or hook (something a script can't do with what exists)
- scene or prefab files (`*.hyscene`, `*.hyprefab`) or the UI builder `scripts/game/ui/build_game_ui.py`
- a refactor across more than ~5 files, or a script over ~800 lines

Never edit anything in `..\Hydro`.

## Hard rules (each one was broken once)

- **C++17 only.** Not `std::span`, `std::format`, `<ranges>`, concepts, `consteval`, designated initialisers (`{ .Price = 5 }`) or `[[likely]]`. Not `fmt::format` (no game file uses it): build strings with `std::string` + `std::to_string`.
- **Every call must already exist.** Copy each engine or game call from a file you opened (`Game/src/...`) or a header you found with `findstr /s /n "Name" ..\Hydro\Hydro\src\*.h`. If you can't point at the line it comes from, don't write it. Plausible names like `SceneLabel`, `LabelSetText`, `SceneFind` or `EventType::KeyDown` **do not exist**, and views get no key events at all (see the `eonlore-script` skill, §6).
- **A clean linter is not working code.** The linter checks style only, and `Build.bat` only compiles headers that something includes, so a new view or rule can be broken and the build still passes. A new or changed header must pass `bash scripts/check.sh <files>` (it compiles each header on its own); if you can't run bash, say "not compiled" instead of "done".
- **Change a function, change its callers.** When you change a rule's or view's signature, search for every use (`findstr /s /n "ShopRules::Buy" Game\src\*.h Tools\Tests\*.cpp`) and update them in the same step, tests included. Then run `bash scripts/check.sh --tests`.
- **Stay inside the step.** When a plan gives you steps (`docs/ShopPlan.md` Q1–Q7), do only those; files the plan gives to Claude (`FieldDirectorScript.h`, `build_game_ui.py`, scenes, prefabs) are not yours unless the step says so.

## How to work

- **Find before you write.** `docs/CodeMap.md` has one line per game file. For the engine API, `..\Hydro\docs\Scripting.md` is the guide; search headers in `..\Hydro\Hydro\src\Hydro`. Never call a function you haven't found.
- **Copy the nearest example.** `Game/src/Scripts/Examples/` (TriggerZoneScript, SpawnerScript), `Views/MessageBannerView.h`, `Rules/ItemRules.h`.
- **Skills:** `eonlore-script` for scripts, views and game components; `eonlore-rules-data` for rules, tests, data types and run state. The rules for game code load by themselves when you open a file in `Game/src`.
- **Keep it small.** Change only what the task needs. Show the plan in a few lines before editing more than two files.
- **Check, in this order:**
  1. Style: `py ..\Hydro\scripts\lint_conventions.py --workspace . <files>`.
  2. Compile each new or changed header: `bash scripts/check.sh <files>` (WSL). The only expected warning is "#pragma once in main file".
  3. Rules or tests changed: `bash scripts/check.sh --tests` (must end with "0 failed").
  4. Build the game library: `cmd /c "scripts\Build.bat Debug --project EonloreGame --no-pause"` (errors in `bin\logs\Build-Debug-errors.log`).

  Report which of these ran and what they said. Say what you couldn't verify: the user presses Play in Oxygen.
- **New file → a line in `docs/CodeMap.md`** (same table style as its neighbours).
- **Never** commit, push, or edit `Game/src/Generated/` (the build regenerates it).
- Keep CRLF line endings, tabs, braces on their own lines.
