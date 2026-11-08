---
name: "eonlore-rules-data"
description: "Use when adding or changing an Eonlore rule (a pure calculation in Game/src/Rules) and its test, a data type or table row (HY_DATA, .hytable, .hydef), or run state on the game session (State/, RpgSession, saving)."
---

# Eonlore: rules, data and run state

These three are what make scripts small. A script calls a rule, changes state, and reads data; it never holds the maths or the run's memory itself.

## Rules (`Game/src/Rules/`)

A rule is a static function of `class <Area>Rules` that takes plain values and returns a result. Copy `Rules/ItemRules.h` (small) or `Rules/QuestRules.h` (templates with lookups passed in).

```cpp
#pragma once

#include <cstdint>
#include <vector>

namespace Hydro
{
	enum class NpcOption : uint8_t { Talk, Quest, Shop, Leave };

	// What an NPC offers the player, in display order.
	class NpcRules
	{
	public:
		static std::vector<NpcOption> Options(bool hasLines, bool hasQuest, bool hasShop)
		{
			std::vector<NpcOption> result;
			if (hasLines)
			{
				result.push_back(NpcOption::Talk);
			}
			// ...
			return result;
		}
	};
}
```

(The real file is `Rules/NpcRules.h`. Rules that change state take it by reference and return an enum result: `Rules/ShopRules.h`, `Rules/ItemRules.h`.)

- **In:** numbers, data structs, state (by reference when the rule changes it, like `ItemRules::UseInField`), random rolls as `float` arguments or a `std::function<float()>`. Lookups the rule needs (an item's count, a definition by row) come in as lambdas.
- **Out:** a result struct or enum (`HitResult`, `UseResult`) that says what happened. The script decides what to show.
- **Never:** `Entity`, `Scene`, UI, effects, cameras, `Get()` of session state, statics, logging.
- Existing rules: `BattleRules`, `LootRules`, `PartyRules`, `ItemRules`, `QuestRules`, `RouteRules`, `OccluderRules`, `ShopRules`, `NpcRules`. Add to the matching one before making a new class.
- Include `<cstdint>` when you use `uint8_t`; include only `Data/`, `Rules/` and plain `State/` headers, never `"Hydro.h"` (see the tests below).
- **Changing a rule's parameters breaks its callers.** Search every use (`findstr /s /n "ShopRules::Buy" Game\src\*.h Tools\Tests\*.cpp`) and update them, tests included, in the same step.

### Its test

Add checks to `Tools/Tests/GameRulesTest.cpp` inside `TestGameRules`, with `RULE_CHECK(condition)`; include your rule header at the top. Fixed numbers, every random roll given (`Rolls({ 0.5f, 0.0f })`), no data files. Cover the normal case, each edge (zero, not enough, the maximum) and one case per branch.

The tests run with `bash scripts/check.sh --tests` (needs bash and g++: WSL or a cloud session) and must end with "0 failed". If you can't run them, say so: the user runs them.

- **The test build has no renderer:** include only headers that don't pull in `"Hydro.h"`. `State/QuestLog.h` and every script or view do, so they can't be tested here; test the pure rule they call instead.
- Store a result before looking at it: `const std::vector<NpcOption> all = NpcRules::Options(true, true, true);`. Never `std::any_of(f().begin(), f().end(), ...)`: two calls are two different vectors.
- Check the whole result (`all == std::vector<NpcOption>({ ... })`), not only that something is in it.

## Data (`Game/src/Data/`, files in `Project/Assets/Data/`)

| Kind | Declare | File | Read |
| --- | --- | --- | --- |
| Table (many rows of one kind) | `HY_DATA(Table, DisplayField(Name)) struct FooDefinition` | `.hytable` | `Data::GetTable<T>(handle).Rows()`, `Data::GetRow<T>(rowRef)` |
| Definition (one record) | `HY_DATA(Definition) struct FooDefinition` | `.hydef` | `Data::Get<T>(handle)` |
| Group of fields | `HY_DATA() struct Foo` | inside another | |
| Choice | `HY_ENUM() enum class Foo : uint8_t` | saved by name | |

Copy `Data/ItemDefinition.h`. Fields are `HY_PROPERTY(...)` with defaults; only the fields the game reads. A missing row gives defaults and one warning: check `IsValid()` on `Data::GetRow`.

- **Adding rows** to an existing table: edit the `.hytable` YAML by hand, matching the existing rows (a row reference is `{Table: <that table's Handle>, Row: Name}`; copy handles from the target file's `Handle:` line, never invent one).
- **A new table or definition file**: the user creates it in Oxygen (it gets a new `Handle`). Write the type, build, then ask them to create the file and tell you its handle.
- A script property that reads it defaults to the file: `HY_PROPERTY(Asset(DataTable, FooDefinition)) AssetHandle Foos = 1234ull; // Assets/Data/Foos.hytable`.
- **Renaming** a type or field: `FormerName(...)`, see the rules file. Renaming a row changes every reference to it: search the project for `Row: OldName` first.

## Run state (`Game/src/State/`)

What the run remembers across scenes (party, items, quests, flags) lives on `RpgSession` (`State/RpgSession.h`), created at Play, destroyed at Stop: no reset code anywhere.

To add state:
1. A plain struct in `State/<WhatItHolds>.h`: fields and small methods that keep it consistent (`Inventory::Add`, `Remove`). Content by `DataRowRef`, never by pointer. No `Scene`, `Entity` or UI.
2. A member of `RpgSession` with a one-line comment.
3. A `static Foo& Get();` on the struct, defined in `State/RpgSession.cpp` next to the others (`return RpgSession::Get().Foo;`).
4. If it must survive a save: a line in `SaveGame::Write` and one in `SaveGame::Load` (`State/SaveGame.cpp`), reading with a default when the key is missing.
5. Only if a view shows it and needs to know when it changed: a `Revision` counter, private fields, and every changing method bumps it.

Small flags that belong to a bigger state (a chest opened: `Inventory::MarkFound`) go there instead of a new struct.

## Before you finish

- [ ] The rule is pure (no entities, UI, statics, `Get()`) and returns a result
- [ ] Tests added for every branch; said whether they ran
- [ ] Data: only fields the game reads, defaults that work, no invented handles
- [ ] State: on the session, referenced by row, saved if it must survive a save
- [ ] Lint and build as in the `eonlore-script` checklist
