#pragma once

#include <cstdint>
#include <vector>

namespace Hydro
{
	// What an NPC can do for the player, for the choice menu.
	enum class NpcOption : uint8_t { Talk, Quest, Shop, Leave };

	class NpcRules
	{
	public:
		// The options this NPC offers, in display order. `Leave` is added only when there are two or more real options;
		// a single option comes back alone and the director runs it without a menu.
		static std::vector<NpcOption> Options(bool hasLines, bool hasQuest, bool hasShop)
		{
			std::vector<NpcOption> result;
			if (hasLines)
			{
				result.push_back(NpcOption::Talk);
			}
			if (hasQuest)
			{
				result.push_back(NpcOption::Quest);
			}
			if (hasShop)
			{
				result.push_back(NpcOption::Shop);
			}
			if (result.size() >= 2)
			{
				result.push_back(NpcOption::Leave);
			}
			return result;
		}
	};
}
