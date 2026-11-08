# Qwen Code for Eonlore game scripting

These files set up Qwen Code with a local model (Ollama, `qwen3.6-64k`) as a scripting partner for Eonlore's game code. Engine work goes to a frontier model; Qwen writes the brief for it (`hydro-handoff`).

## What's here

| File | What it is | In the prompt |
| --- | --- | --- |
| `QWEN.md` (repository root) | Who Qwen is here: game scripting only, where things are, how to check, when to hand off | Always (keep it short) |
| `.qwen/rules/eonlore-game-code.md` | Layers and folders, serialized names, C++ conventions | When Qwen reads or edits `Game/src/**` or `Tools/Tests/**` |
| `.qwen/skills/eonlore-script/SKILL.md` | Scripts, views, game components: template, property types, hooks, the API a script can use, checklist | On demand (`/skills`, or picked by the model) |
| `.qwen/skills/eonlore-rules-data/SKILL.md` | Rules and their tests, data types and rows, run state and saving | On demand |
| `%USERPROFILE%\.qwen\skills\hydro-handoff\SKILL.md` | The brief for a frontier model when a task is engine work (shared by Hydro and Eonlore) | On demand |

The text is written for a ~35B local model with a 64k context: the always-loaded part is small, the rest loads when needed.

## Start

```powershell
cd C:\Users\joeln\Eonlore
qwen
```

The first time, trust the folder when Qwen asks: project rules and skills only load in a trusted workspace. Check with `/skills`: `eonlore-script`, `eonlore-rules-data` and `hydro-handoff` should be listed.

## Test prompts

1. **Read only:** `Explain what QuestGiverScript does and which state and rules it uses. Don't edit.`
2. **Small script:** `Use eonlore-script: write a LanternFlickerScript in Scripts/Common that makes a Light2DComponent's intensity flicker with a Speed and Strength property. Lint and build it.`
3. **Rule and test:** `Use eonlore-rules-data: add a ShopRules::Buy rule (money, price, count -> bought, money left) with tests. Don't wire it to a script yet.`
4. **Hand-off:** `Add a SetImage function to the runtime UI so a view can swap an icon.` It should refuse to edit and write a brief.

## Keeping it in step

The engine's rules are in Hydro (`docs/Scripting.md`, the `hydro-engine-architecture` skill). When they change (a new hook, a new property type, a new API in "What a script can use"), update the tables in `eonlore-script` and the rules file. The model reads these files as instructions: still review its diffs and press Play before committing.
