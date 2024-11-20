#pragma once

namespace QuickLoot::Config
{
	enum AnchorPoint : uint8_t
	{
		kTopLeft,
		kCenterLeft,
		kBottomLeft,
		kTopCenter,
		kCenter,
		kBottomCenter,
		kTopRight,
		kCenterRight,
		kBottomRight,
	};

	class Settings
	{
	public:
		Settings() = delete;
		~Settings() = delete;
		Settings(Settings const&) = delete;
		Settings(Settings const&&) = delete;
		Settings operator=(Settings&) = delete;
		Settings operator=(Settings&&) = delete;

		static void Update();

		static bool ShowInCombat();
		static bool ShowWhenEmpty();
		static bool ShowWhenUnlocked();
		static bool ShowInThirdPersonView();
		static bool ShowWhenMounted();
		static bool EnableForAnimals();
		static bool EnableForDragons();
		static bool DispelInvisibility();

		static int GetWindowX();
		static int GetWindowY();
		static float GetWindowScale();
		static AnchorPoint GetWindowAnchor();
		static int GetWindowMinLines();
		static int GetWindowMaxLines();
		static float GetWindowOpacityNormal();
		static float GetWindowOpacityEmpty();

		static bool ShowIconRead();
		static bool ShowIconStolen();
		static bool ShowIconEnchanted();
		static bool ShowIconEnchantedKnown();
		static bool ShowIconEnchantedSpecial();

		static const std::vector<std::string>& GetUserDefinedSortPriority();

		static bool ShowDBMDisplayed();
		static bool ShowDBMFound();
		static bool ShowDBMNew();
		static bool ShowCompNeeded();
		static bool ShowCompCollected();
	};
}
