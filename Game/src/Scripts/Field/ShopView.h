#pragma once

#include "Hydro.h"
#include "Data/InventoryDefinition.h"
#include "Data/ItemDefinition.h"
#include "Rules/ShopRules.h"
#include "State/Inventory.h"
#include "Views/ViewHelpers.h"

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

namespace Hydro
{
	// The shop screen on "Shop Canvas": the merchant's goods (Buy) or the party's sellable items (Sell), the money and the
	// selected item. Turns its rows' events into requests; the director buys or sells (Rules/ShopRules.h), then Present().
	class ShopView
	{
	public:
		static constexpr int MaxRows = 10; // the rows in the scene; more goods aren't shown

		enum class Mode : uint8_t { Buy, Sell };
		enum class Request : uint8_t { None, Buy, Sell, Close };

		void Bind(Scene& scene)
		{
			m_Scene = &scene;
			const char* owner = "Shop";
			m_Canvas = ViewHelpers::Find(scene, "Shop Canvas", owner);
			m_Title = ViewHelpers::Find(scene, "Shop Title", owner);
			m_ModeBuy = ViewHelpers::Find(scene, "Shop Mode Buy", owner);
			m_ModeSell = ViewHelpers::Find(scene, "Shop Mode Sell", owner);
			m_MoneyLabel = ViewHelpers::Find(scene, "Shop Money Label", owner);
			m_MoneyAmount = ViewHelpers::Find(scene, "Shop Money Amount", owner);
			for (int i = 0; i < MaxRows; i++)
			{
				const std::string row = "Shop Row " + std::to_string(i);
				m_Row[i] = ViewHelpers::Find(scene, row, owner);
				m_Name[i] = ViewHelpers::Find(scene, row + " Name", owner);
				m_Price[i] = ViewHelpers::Find(scene, row + " Price", owner);
				m_Count[i] = ViewHelpers::Find(scene, row + " Count", owner);
			}
			m_DetailName = ViewHelpers::Find(scene, "Shop Detail Name", owner);
			m_DetailText = ViewHelpers::Find(scene, "Shop Detail Text", owner);
			m_Help = ViewHelpers::Find(scene, "Shop Help", owner);
		}

		// Starts in Buy mode on the first row. `help` is the key help (the director owns the text, GetFieldText).
		void Open(const std::string& title, const InventoryDefinition& stock, const std::string& help)
		{
			if (m_Scene == nullptr)
			{
				return;
			}
			m_Stock = stock;
			m_Mode = Mode::Buy;
			m_Selected = 0;
			UI::SetText(EntityOf(m_Title), title);
			UI::SetText(EntityOf(m_MoneyLabel), CurrencyName);
			UI::SetText(EntityOf(m_Help), help);
			ViewHelpers::SetVisible(EntityOf(m_Canvas), true);
			Present();
			Focus();
		}

		void Close()
		{
			if (m_Scene == nullptr)
			{
				return;
			}
			ViewHelpers::SetVisible(EntityOf(m_Canvas), false);
			UI::ClearFocus(*m_Scene);
		}

		bool IsOpen() const
		{
			return m_Scene != nullptr && UI::IsVisible(EntityOf(m_Canvas));
		}

		Mode GetMode() const { return m_Mode; }

		// The director calls this on Left/Right (the UI has no events for those keys).
		void SwitchMode()
		{
			m_Mode = m_Mode == Mode::Buy ? Mode::Sell : Mode::Buy;
			m_Selected = 0;
			Present();
			Focus();
		}

		// The item on the selected row; unset when the list is empty.
		DataRowRef GetSelectedItem() const
		{
			return m_Selected >= 0 && m_Selected < (int)m_Rows.size() ? m_Rows[(size_t)m_Selected] : DataRowRef{};
		}

		// On the selection, clamped to the list (a sold-out line disappears from the Sell list).
		void Focus()
		{
			if (m_Rows.empty())
			{
				UI::ClearFocus(*m_Scene);
				return;
			}
			m_Selected = std::clamp(m_Selected, 0, (int)m_Rows.size() - 1);
			UI::Focus(EntityOf(m_Row[m_Selected]));
		}

		// Focus selects, Submit asks to buy or sell one (by the mode), Cancel closes. Returns whether the event was the shop's.
		bool HandleEvent(const UIEvent& event, Request& request)
		{
			request = Request::None;
			if (IsOpen() == false)
			{
				return false;
			}
			if ((entt::entity)event.Target == m_Canvas && event.Type == UIEventType::Cancel)
			{
				request = Request::Close;
				return true;
			}
			const int count = (int)m_Rows.size();
			for (int i = 0; i < count; i++)
			{
				if ((entt::entity)event.Target != m_Row[i])
				{
					continue;
				}
				switch (event.Type)
				{
				case UIEventType::FocusEnter:
					m_Selected = i;
					PresentDetails();
					return true;
				case UIEventType::Submit:
					m_Selected = i;
					request = m_Mode == Mode::Buy ? Request::Buy : Request::Sell;
					return true;
				case UIEventType::Cancel:
					request = Request::Close;
					return true;
				default:
					return true;
				}
			}
			return false;
		}

		// Rebuilds the list for the mode and shows it; call after every buy or sell. Rows past the list are hidden.
		void Present()
		{
			if (m_Scene == nullptr)
			{
				return;
			}
			BuildRows();
			PresentMode(EntityOf(m_ModeBuy), m_Mode == Mode::Buy);
			PresentMode(EntityOf(m_ModeSell), m_Mode == Mode::Sell);
			UI::SetText(EntityOf(m_MoneyAmount), std::to_string(Inventory::Get().Money));
			for (int i = 0; i < MaxRows; i++)
			{
				const bool used = i < (int)m_Rows.size();
				ViewHelpers::SetVisible(EntityOf(m_Row[i]), used);
				if (used == false)
				{
					continue;
				}
				const DataRowRef& row = m_Rows[(size_t)i];
				const DataView<ItemDefinition> item = Data::GetRow<ItemDefinition>(row);
				const int price = item.IsValid() == false ? 0 : (m_Mode == Mode::Buy ? item->Price : ShopRules::SellPrice(*item));
				UI::SetText(EntityOf(m_Name[i]), item.IsValid() ? item->Name : "Missing: " + row.Row);
				UI::SetText(EntityOf(m_Price[i]), std::to_string(price));
				UI::SetText(EntityOf(m_Count[i]), "x" + std::to_string(Inventory::Get().CountOf(row)));
			}
			PresentDetails();
		}

	private:
		// Buy: the stock's priced items (stock never runs out, so its counts are ignored). Sell: what the party holds and may sell.
		void BuildRows()
		{
			m_Rows.clear();
			if (m_Mode == Mode::Buy)
			{
				for (const InventoryEntry& entry : m_Stock.Items)
				{
					const DataView<ItemDefinition> item = Data::GetRow<ItemDefinition>(entry.Item);
					if (item.IsValid() && item->Price > 0 && (int)m_Rows.size() < MaxRows)
					{
						m_Rows.push_back(entry.Item);
					}
				}
				return;
			}
			for (const Inventory::Slot& slot : Inventory::Get().Slots)
			{
				const DataView<ItemDefinition> item = Data::GetRow<ItemDefinition>(slot.Item);
				if (slot.Count > 0 && item.IsValid() && ShopRules::CanSell(*item) && (int)m_Rows.size() < MaxRows)
				{
					m_Rows.push_back(slot.Item);
				}
			}
		}

		void PresentDetails()
		{
			const DataRowRef row = GetSelectedItem();
			if (row.IsSet() == false)
			{
				UI::SetText(EntityOf(m_DetailName), "");
				UI::SetText(EntityOf(m_DetailText), m_Mode == Mode::Buy ? "Nothing for sale." : "Nothing to sell.");
				return;
			}
			const DataView<ItemDefinition> item = Data::GetRow<ItemDefinition>(row);
			UI::SetText(EntityOf(m_DetailName), item.IsValid() ? item->Name : "Missing: " + row.Row);
			UI::SetText(EntityOf(m_DetailText), item.IsValid() ? item->Description : "This item is not in the items table any more.");
		}

		// The active mode's header in gold, the other dimmed.
		static void PresentMode(Entity header, bool active)
		{
			if (UITextComponent* text = header.IsValid() ? header.TryGetComponent<UITextComponent>() : nullptr)
			{
				text->Color = active ? glm::vec4(0.95f, 0.80f, 0.40f, 1.0f) : glm::vec4(0.60f, 0.58f, 0.54f, 1.0f);
			}
		}

		Entity EntityOf(entt::entity entity) const { return Entity{ entity, m_Scene }; }

		Scene* m_Scene = nullptr;
		entt::entity m_Canvas = entt::null, m_Title = entt::null, m_ModeBuy = entt::null, m_ModeSell = entt::null;
		entt::entity m_MoneyLabel = entt::null, m_MoneyAmount = entt::null;
		entt::entity m_Row[MaxRows] = {};
		entt::entity m_Name[MaxRows] = {};
		entt::entity m_Price[MaxRows] = {};
		entt::entity m_Count[MaxRows] = {};
		entt::entity m_DetailName = entt::null, m_DetailText = entt::null, m_Help = entt::null;
		InventoryDefinition m_Stock;
		std::vector<DataRowRef> m_Rows;
		Mode m_Mode = Mode::Buy;
		int m_Selected = 0;
	};
}
