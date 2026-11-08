#pragma once

#include "Hydro/Asset/Asset.h"
#include "Hydro/Scene/Scripting/ReflectionMacros.h"

#include <string>
#include <vector>

// What the field's people and signs say. Rows are found by the name in the tag without spaces ("Npc Village Elder" ->
// VillageElder, "Sign Shrine" -> Shrine).
namespace Hydro
{
	// Forward-declare InventoryDefinition so Shop can reference it.
	struct InventoryDefinition;

	HY_DATA(Table, DisplayField(Name))
	struct NpcDefinition
	{
		HY_PROPERTY(Tooltip("The name on the dialogue box (when the dialogue has no speaker of its own)"))
		std::string Name;
		HY_PROPERTY(Asset(Definition, DialogueDefinition), Tooltip("What they say when the player talks to them"))
		AssetHandle Dialogue = 0;
		HY_PROPERTY(
		    Asset(Definition, InventoryDefinition),
		    Tooltip("What this NPC sells (0: not a merchant)"))
		AssetHandle Shop = 0;
	};

	HY_DATA(Table)
	struct SignDefinition
	{
		HY_PROPERTY(Tooltip("The lines shown in the message box (two fit)"))
		std::vector<std::string> Lines;
	};
}
