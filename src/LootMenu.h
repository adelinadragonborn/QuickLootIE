#pragma once

// Array.h must be imported or CoreList.h throws errors.
#include "CLIK/Array.h"
#include "CLIK/GFx/Controls/ButtonBar.h"
#include "CLIK/GFx/Controls/ScrollingList.h"
#include "CLIK/TextField.h"
#include "Config/UserSettings.h"
#include "Input/Input.h"
#include "Items/OldItem.h"

using Settings = QuickLoot::Config::UserSettings;

namespace QuickLoot
{
	enum class RefreshFlags : uint8_t
	{
		kNone = 0,
		kAll = static_cast<RefreshFlags>(-1),

		kInventory = 1 << 0,
		kButtonBar = 1 << 1,
		kInfoBar = 1 << 2,
		kWeight = 1 << 3,
		kTitle = 1 << 4,
	};

	class LootMenu : public RE::IMenu
	{
	public:
		static constexpr std::string_view FILE_NAME{ "LootMenuIE" };
		static constexpr std::string_view MENU_NAME{ "LootMenu" };
		static constexpr int CURRENT_MENU_VERSION = 3;

		static void Register();
		static IMenu* CreateInstance();
		static int GetSwfVersion();

		// Dummy constructor that doesn't do any initialization.
		LootMenu(nullptr_t) {}
		LootMenu();

		void SetContainer(const RE::ObjectRefHandle& container, int selectedIndex);
		void OnInputAction(Input::QuickLootAction action);
		void QueueRefresh(RefreshFlags flags);

	private:
		RE::ObjectRefHandle _container{};
		int _selectedIndex = -1;
		RE::stl::enumeration<RefreshFlags> _refreshFlags = RefreshFlags::kAll;

		CLIK::MovieClip _lootMenu;
		CLIK::TextField _title;
		CLIK::TextField _weight;

		CLIK::GFx::Controls::ScrollingList _itemList;
		CLIK::GFx::Controls::ButtonBar _infoBar;
		CLIK::GFx::Controls::ButtonBar _buttonBar;

		std::vector<std::unique_ptr<Items::OldItem>> _itemListImpl;

		RE::GFxValue _itemListProvider;
		RE::GFxValue _infoBarProvider;
		RE::GFxValue _buttonBarProvider;

		void InjectUtilsClass();
		void LoadSwfObject(CLIK::Object& target, std::string_view path) const;
		RE::GFxValue BuildSettingsObject() const;
		static void ResolveAnchorPoint(Config::AnchorPoint anchor, double& fractionX, double& fractionY);

		void OnSelectedIndexChanged(int newIndex);
		void SetSelectedIndex(int newIndex);
		void ScrollUp();
		void ScrollDown();
		void ScrollPrevPage();
		void ScrollNextPage();

		static void DispelEffectsWithArchetype(RE::MagicTarget* target, RE::MagicTarget::Archetype type, bool force);
		void OnTakeAction();
		void TakeStack();
		void TakeAll();
		void Transfer();

		void Refresh(RefreshFlags flags = RefreshFlags::kNone);
		void RefreshInventory();
		void RefreshButtonBar();
		void RefreshInfoBar();
		void RefreshWeight();
		void RefreshTitle();

		[[nodiscard]] static const char* GetActionDisplayName(Input::QuickLootAction action, bool stealing);
		[[nodiscard]] static bool CanDisplay(const RE::TESBoundObject& object);
		[[nodiscard]] bool WouldBeStealing() const;

		// IMenu implementation
		RE::UI_MESSAGE_RESULTS ProcessMessage(RE::UIMessage& message) override;
		void AdvanceMovie(float interval, std::uint32_t currentTime) override;
		void RefreshPlatform() override;
	};
}
