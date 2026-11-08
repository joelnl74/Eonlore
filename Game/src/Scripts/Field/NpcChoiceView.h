#pragma once

#include "Hydro.h"
#include "Rules/NpcRules.h"
#include "Views/ViewHelpers.h"

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

namespace Hydro
{
	// The choice menu on "Choice Canvas" when an NPC offers more than one thing (Talk / Quest / Buy & Sell / Leave). Shows the
	// options and turns their rows' events into requests; the director acts on the pick.
	class NpcChoiceView
	{
	public:
		static constexpr int MaxRows = 4; // the rows in the scene

		enum class Request : uint8_t { None, Pick, Cancel };

		void Bind(Scene& scene)
		{
			m_Scene = &scene;
			const char* owner = "NpcChoice";
			m_Canvas = ViewHelpers::Find(scene, "Choice Canvas", owner);
			m_Title = ViewHelpers::Find(scene, "Choice Title", owner);
			for (int i = 0; i < MaxRows; i++)
			{
				m_Row[i] = ViewHelpers::Find(scene, "Choice " + std::to_string(i), owner);
				m_Text[i] = ViewHelpers::Find(scene, "Choice " + std::to_string(i) + " Name", owner);
			}
		}

		// `labels` are the options' words, in the same order (the director owns the text, GetFieldText).
		void Show(const std::string& title, const std::vector<NpcOption>& options, const std::vector<std::string>& labels)
		{
			if (m_Scene == nullptr)
			{
				return;
			}
			m_Options = options;
			UI::SetText(EntityOf(m_Title), title);
			for (int i = 0; i < MaxRows; i++)
			{
				const bool used = i < (int)m_Options.size();
				ViewHelpers::SetVisible(EntityOf(m_Row[i]), used);
				if (used)
				{
					UI::SetText(EntityOf(m_Text[i]), i < (int)labels.size() ? labels[(size_t)i] : std::string());
				}
			}
			ViewHelpers::SetVisible(EntityOf(m_Canvas), true);
			m_Selected = 0;
			UI::Focus(EntityOf(m_Row[0]));
		}

		void Hide()
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

		// The option of the last Pick.
		NpcOption GetPicked() const
		{
			return m_Selected < (int)m_Options.size() ? m_Options[(size_t)m_Selected] : NpcOption::Leave;
		}

		// Submit on a row picks it; Cancel (on a row or the canvas) backs out. Returns whether the event was this menu's.
		bool HandleEvent(const UIEvent& event, Request& request)
		{
			request = Request::None;
			if (IsOpen() == false)
			{
				return false;
			}
			if ((entt::entity)event.Target == m_Canvas && event.Type == UIEventType::Cancel)
			{
				request = Request::Cancel;
				return true;
			}
			const int count = std::min((int)m_Options.size(), MaxRows);
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
					return true;
				case UIEventType::Submit:
					m_Selected = i;
					request = Request::Pick;
					return true;
				case UIEventType::Cancel:
					request = Request::Cancel;
					return true;
				default:
					return true;
				}
			}
			return false;
		}

	private:
		Entity EntityOf(entt::entity entity) const { return Entity{ entity, m_Scene }; }

		Scene* m_Scene = nullptr;
		entt::entity m_Canvas = entt::null, m_Title = entt::null;
		entt::entity m_Row[MaxRows] = {};
		entt::entity m_Text[MaxRows] = {};
		std::vector<NpcOption> m_Options;
		int m_Selected = 0;
	};
}
