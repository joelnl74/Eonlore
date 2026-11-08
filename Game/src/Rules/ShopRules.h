#pragma once

#include "Data/InventoryDefinition.h"
#include "Data/ItemDefinition.h"
#include "State/Inventory.h"

#include <algorithm>

namespace Hydro
{
	// Buying and selling: plain rules on Inventory and ItemDefinition, no UI.
	class ShopRules
	{
	public:
		// Most of one item the bag holds; also the most bought at once, which keeps the cost far from int overflow.
		static constexpr int MaxStack = 99;

		enum class BuyResult : uint8_t { Bought, NotForSale, NotEnoughMoney, BadCount, BagFull };
		enum class SellResult : uint8_t { Sold, NotSellable, NoneLeft, BadCount };

		// True when `item` appears in the merchant's stock.
		static bool InStock(const InventoryDefinition& stock, const DataRowRef& item)
		{
			for (const InventoryEntry& entry : stock.Items)
			{
				if (entry.Item == item)
				{
					return true;
				}
			}
			return false;
		}

		// Whether the item can be sold back to any shop (Price > 0 and not KeyItem).
		static bool CanSell(const ItemDefinition& item)
		{
			return item.Price > 0 && item.Kind != ItemKind::KeyItem;
		}

		static BuyResult Buy(Inventory& inventory, const InventoryDefinition& stock, const DataRowRef& itemRow, const ItemDefinition& item, int count)
		{
			if (count < 1 || count > MaxStack)
			{
				return BuyResult::BadCount;
			}
			if (InStock(stock, itemRow) == false)
			{
				return BuyResult::NotForSale;
			}
			if (item.Price == 0)
			{
				return BuyResult::NotForSale;
			}
			if (inventory.CountOf(itemRow) + count > MaxStack)
			{
				return BuyResult::BagFull;
			}
			const int cost = item.Price * count;
			if (inventory.Money < cost)
			{
				return BuyResult::NotEnoughMoney;
			}
			inventory.Money -= cost;
			inventory.Add(itemRow, count);
			return BuyResult::Bought;
		}

		static SellResult Sell(Inventory& inventory, const DataRowRef& itemRow, const ItemDefinition& item, int count)
		{
			if (count < 1)
			{
				return SellResult::BadCount;
			}
			if (CanSell(item) == false)
			{
				return SellResult::NotSellable;
			}
			const int held = inventory.CountOf(itemRow);
			if (held < count)
			{
				return SellResult::NoneLeft;
			}
			inventory.Remove(itemRow, count);
			inventory.Money += SellPrice(item) * count;
			return SellResult::Sold;
		}

		// Half the price, but never 0 for an item that has one: a 1-aster item still sells for 1.
		static int SellPrice(const ItemDefinition& item)
		{
			if (item.Price <= 0)
			{
				return 0;
			}
			return std::max(1, item.Price / 2);
		}
	};
}
