---
name: "eonlore-script"
description: "Use when writing or changing an Eonlore game script (HY_SCRIPT on an entity), a view that shows state on UI, or a game component (HY_COMPONENT). Template, properties, hooks, the API a script can use, and a checklist."
---

# Eonlore: scripts, views and game components

Full guide: `..\Hydro\docs\Scripting.md` (open only the section you need). Engine headers: `..\Hydro\Hydro\src\Hydro\`.

## 1. Decide what you are writing

| The task | Write | Where |
| --- | --- | --- |
| Behaviour on one entity (moves, reacts, runs a flow) | Script | `Game/src/Scripts/<Area>/<Name>Script.h`, or `Scripts/Common/` if several areas use it |
| Showing state on UI entities | View (plain class the script owns) | Next to its script, or `Game/src/Views/` if shared |
| Values on an entity, no behaviour | Game component (`HY_COMPONENT`) | Next to the script that reads it |
| A calculation (damage, cost, chance, order) | A rule: use `eonlore-rules-data` | `Game/src/Rules/` |
| Something the run must remember across scenes | State: use `eonlore-rules-data` | `Game/src/State/` |

Look first: is there a script that already does most of it? Search `docs/CodeMap.md` for the behaviour.

## 2. Script template

```cpp
#pragma once

#include "Hydro.h"

#include <string>

namespace Hydro
{
	// What it does in one or two lines, and where it goes.
	HY_SCRIPT()
	class LanternFlickerScript : public ScriptableEntity
	{
	public:
		HY_PROPERTY(Range(0, 10), Tooltip("Flickers per second"))
		float Speed = 3.0f;
		HY_PROPERTY(Tooltip("The light that flickers; empty: this entity"))
		EntityRef Light;

	protected:
		void OnCreate() override
		{
			// Find what you need once. Properties are already applied here.
		}

		void OnUpdate(float deltaTime) override
		{
			m_Time += deltaTime;
		}

	private:
		float m_Time = 0.0f; // the script's own state: plain members, gone with the scene
	};
}
```

- Public `HY_PROPERTY` fields are saved per entity and shown in the inspector; the initialiser is the default.
- Overrides stay `protected`. No work in a constructor. Header-only.
- A new file is picked up by the build (premake globs `Game/src/**`); the build regenerates `Generated/`.
- Attach it in Oxygen: select the entity, **Add Component > Script > LanternFlickerScript**.

## 3. Property types

| Type | For | Arguments |
| --- | --- | --- |
| `bool`, `int`, `float` | Tuning | `Range(min, max)`, `Min`, `Max` |
| `glm::vec2/3/4` | Positions, sizes, colours | `Color` |
| `std::string` | Names, tags (prefer refs and rows) | |
| `AssetHandle` | Texture, prefab, definition, table | `Asset(Texture2D)`, `Asset(Prefab)`, `Asset(Definition, T)`, `Asset(DataTable, T)` |
| `DataRowRef` | One row: an item, enemy, quest | `DataRow(ItemDefinition)` |
| `EntityRef` | Another entity | `Required` |
| `ComponentRef<T>` | A script or component on another entity | `Required` |
| `HY_ENUM` enum, `HY_DATA` struct, `std::vector<T>` of these | Choices, groups, lists | the element's |

All take `Tooltip("...")`, `DisplayName("...")`, `ReadOnly`, `Hidden`, `FormerName("...")`. `Required` refs can still be empty at runtime: check `Get()`.

## 4. Hooks (all optional)

| Hook | When |
| --- | --- |
| `OnCreate()` | Once, before the first update. Other scripts exist with their properties, but their `OnCreate` may not have run |
| `OnUpdate(float deltaTime)` | Every frame while the scene plays |
| `OnDestroy()` | Entity or script removed, or scene ends |
| `OnCollisionEnter/Exit(Entity other)` | After physics, 2D solid colliders |
| `OnTriggerEnter/Exit(Entity other)` | After physics, sensors (3D sends only these) |
| `bool OnUIEvent(const UIEvent& event)` | A button or canvas at or below this entity; return `true` when handled |
| `OnDrawGizmos(Gizmos& gizmos)` | Editor view, also outside Play (routes, ranges) |

`other` may be pending destroy: check `other.IsValid()` and use `TryGetComponent<T>()`.

## 5. What a script can use

| Need | Call |
| --- | --- |
| Own entity | `GetEntity()`, `GetComponent<T>()`, `TryGetComponent<T>()` |
| Position | `GetWorldPosition()`, `SetWorldPosition(p)`; others: `scene.GetWorldPosition(e)`, `scene.SetWorldPosition(e, p)` |
| Scene | `Scene& scene = *GetEntity().GetScene();` `CreateEntity`, `DuplicateEntity`, `GetEntityByUUID`, `DestroyEntity` (deferred: safe in callbacks) |
| Run state | `Inventory::Get()`, `PartyState::Get()`, `QuestLog::Get()`, `BattleState::Get()`; `RpgSession::Get()` |
| Data | `Data::Get<T>(handle)`, `Data::GetRow<T>(rowRef)` (check `IsValid()`), `Data::GetTable<T>(handle).Rows()` |
| Input | `Input::IsKeyPressed(KeyCode::E)` is "held": make your own edge (`SpawnerScript`). Ignore keys while `UI::CapturesInput(scene)` |
| Physics | `Physics2D::SetLinearVelocity` / `GetLinearVelocity`; `Physics3D::Raycast`, `Physics3D::OverlapSphere` |
| Animation | `SpriteAnimator::Play(entity, "Clip")` (safe every frame), `Stop`, `IsPlaying` |
| Spawning | `PrefabSerializer::Instantiate(scene, prefabHandle)`; effects `Vfx::Play(scene, effectPrefab, position)` |
| Scenes | `SceneTransition::Push("Battle")`, `Pop()`, `Replace(...)` |
| UI | `UI::SetText`, `UI::SetVisible`, `UI::IsVisible`, `UI::Focus`; `ViewHelpers::SetAlpha`, `SetBarFill`, `Find` |
| Log | `HY_INFO("{} did {}", a, b)`, `HY_WARN(...)` |

Not there yet, don't invent them: timers, tweens, coroutines, an event queue, `OnFixedUpdate`, enable/disable hooks, runtime debug drawing. Use member timers and `deltaTime`.

Also not there (all were invented once): `SceneLabel`, `SceneText`, `SceneCanvas`, `SceneFind`, `LabelSetText` / `LabelSetEnabled` / `LabelSetColor`, `EntitySetVisible`, `Event` / `EventType::KeyDown` / `KeyCodeEvent`, `Find<T>(table, row)`, `DataRowRef::TableId`, `Inventory::Slot::ItemRow`. The real names are in the table above and in §6.

## 6. A view

Copy `Game/src/Scripts/Field/InventoryView.h` (a menu list) or `Views/MessageBannerView.h` (a simple panel); `Scripts/Field/ShopView.h` and `NpcChoiceView.h` are full examples of a screen on its own canvas.

```cpp
class ThingView
{
public:
	enum class Request : uint8_t { None, Pick, Close }; // what the player asked for; the script acts on it

	void Bind(Scene& scene)                              // once, in the script's OnCreate
	{
		m_Scene = &scene;                                // a pointer: a Scene can't be copied
		m_Canvas = ViewHelpers::Find(scene, "Thing Canvas", "Thing");
		m_Row[0] = ViewHelpers::Find(scene, "Thing Row 0", "Thing");
	}

	bool HandleEvent(const UIEvent& event, Request& request) // the script passes OnUIEvent on; true when it was ours
	{
		request = Request::None;
		if ((entt::entity)event.Target != m_Row[0])
		{
			return false;
		}
		if (event.Type == UIEventType::Submit)
		{
			request = Request::Pick;
		}
		return true;
	}

	void Present()                                       // writes the UI from state and data; changes neither
	{
		UI::SetText(EntityOf(m_Row[0]), std::to_string(Inventory::Get().Money));
	}

private:
	Entity EntityOf(entt::entity entity) const { return Entity{ entity, m_Scene }; }

	Scene* m_Scene = nullptr;
	entt::entity m_Canvas = entt::null;
	entt::entity m_Row[1] = {};
};
```

- **The only UI calls:** `UI::SetText(entity, std::string)`, `UI::SetVisible`, `UI::IsVisible`, `UI::Focus(button)`, `UI::ClearFocus(scene)`, `UI::GetFocused(scene)`, `UI::SetInteractable`, `UI::CapturesInput(scene)` (`Hydro/Scene/UI/UI.h`); `ViewHelpers::Find`, `SetVisible`, `SetAlpha`, `SetBarFill`. A colour: `entity.TryGetComponent<UITextComponent>()->Color = glm::vec4(...)` (see `InventoryView::Present`).
- **The only UI events:** `UIEventType::Submit` (Enter/Space/Z), `Cancel` (Esc/Backspace/X), `FocusEnter`, `FocusExit`. Up/Down move the focus between buttons by themselves. **There are no key events:** a view never reads keys. Other keys (Left/Right, Q/E) are polled by the script (`KeyPressed` in `FieldDirectorScript`) and passed to the view as a call (`ShopView::SwitchMode`).
- A Cancel with nothing focused goes to the capturing canvas itself: compare `event.Target` with the canvas too.
- **Views never change state** (money, items, quests) and never call a rule that does; they may call pure display helpers (`ShopRules::SellPrice`). The script calls the rule, then the view's `Present()`.
- **Views never include a script** (`FieldDirectorScript.h`): the director passes its words (`GetFieldText(...)`) in as `std::string`.
- New tags a view finds must exist in the scene: they're built by `scripts/game/ui/build_game_ui.py`, which is a hand-off (QWEN.md). Name the tags in your summary so the builder can add them.

## 7. A game component

Copy `Game/src/Scripts/Field/WaypointComponent.h`: `HY_COMPONENT()` above a `struct` of `HY_PROPERTY` fields, includes `"Hydro/Scene/Scripting/ReflectionMacros.h"`. No functions with logic. Scripts read it with `TryGetComponent<WaypointComponent>()`.

## 8. Before you finish

- [ ] Right layer and folder; no rule maths in the script; no run state in members or statics
- [ ] Every function, type and tag you used exists (you searched for it)
- [ ] No `!`, braces everywhere, `m_PascalCase`, `namespace Hydro` only
- [ ] No renamed class or property without `FormerName`
- [ ] C++17 only; no `std::span`, `std::format`, `fmt::format`
- [ ] Each engine call copied from a file you opened (you can name the file and line)
- [ ] Lint passed: `py ..\Hydro\scripts\lint_conventions.py --workspace . <files>`
- [ ] Compiled: `bash scripts/check.sh <files>` for every new or changed header (or said "not compiled")
- [ ] Built: `cmd /c "scripts\Build.bat Debug --project EonloreGame --no-pause"`; on failure read `bin\logs\Build-Debug-errors.log` and fix
- [ ] Tell the user: which entity to attach it to, which properties to set, and what to check by pressing Play
