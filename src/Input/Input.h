#pragma once

#include <frozen/map.h>

namespace Input
{
	constexpr inline auto QUICKLOOT_FLAG = static_cast<RE::UserEvents::USER_EVENT_FLAG>(1 << 12);

	class ControlGroups
	{
	public:
		enum Group
		{
			kPageKeys,
			kArrowKeys,
			kMouseWheel,
			kDPAD,
			kTotal
		};

		[[nodiscard]] static ControlGroups& get() noexcept
		{
			static ControlGroups singleton;
			return singleton;
		}

		[[nodiscard]] constexpr bool& operator[](Group a_group) noexcept { return _enabled[a_group]; }
		[[nodiscard]] constexpr bool operator[](Group a_group) const noexcept { return _enabled[a_group]; }

	private:
		ControlGroups() = default;
		ControlGroups(const ControlGroups&) = delete;
		ControlGroups(ControlGroups&&) = delete;

		~ControlGroups() = default;

		ControlGroups& operator=(const ControlGroups&) = delete;
		ControlGroups& operator=(ControlGroups&&) = delete;

		static inline std::array<bool, kTotal> _enabled{ false };
	};
	using Group = ControlGroups::Group;

	class InputManager
	{
	public:
		static void Install()
		{
			if (REL::Module::IsAE()) {
				constexpr std::array locations{
					std::make_pair<std::uint64_t, std::size_t>(53270, 0x17),
					std::make_pair<std::uint64_t, std::size_t>(53299, 0x17),
					std::make_pair<std::uint64_t, std::size_t>(68534, 0x165),
					std::make_pair<std::uint64_t, std::size_t>(68540, 0x266),
				};

				auto& trampoline = SKSE::GetTrampoline();
				for (const auto& [id, offset] : locations) {
					REL::Relocation<std::uintptr_t> target(REL::ID(id), offset);
					_RefreshLinkedMappings = trampoline.write_call<5>(target.address(), RefreshLinkedMappings);
				}
			} else {
				constexpr std::array locations{
					std::make_pair<std::uint64_t, std::size_t>(52374, 0x17),
					std::make_pair<std::uint64_t, std::size_t>(52400, 0x17),
					std::make_pair<std::uint64_t, std::size_t>(67234, 0x113),
					std::make_pair<std::uint64_t, std::size_t>(67240, 0x17B),
				};

				auto& trampoline = SKSE::GetTrampoline();
				for (const auto& [id, offset] : locations) {
					REL::Relocation<std::uintptr_t> target(REL::ID(id), offset);
					_RefreshLinkedMappings = trampoline.write_call<5>(target.address(), RefreshLinkedMappings);
				}
			}
		}

	private:
		class UserEventMap
		{
		public:
			using value_type = std::string_view;

			UserEventMap() :
				_mappings{}
			{
				insert("Favorites"sv);
				insert("Activate"sv);
				insert("Ready Weapon"sv);
			}

			void operator()(std::size_t a_device, RE::ControlMap::UserEventMapping& a_userEvent) const
			{
				using UEFlag = RE::UserEvents::USER_EVENT_FLAG;

				const auto& mapping = _mappings[a_device];
				auto it = mapping.find(a_userEvent.eventID);
				if (it != mapping.end()) {
					if (a_userEvent.userEventGroupFlag.all(UEFlag::kInvalid)) {
						a_userEvent.userEventGroupFlag = UEFlag::kNone;
					}

					a_userEvent.userEventGroupFlag.set(QUICKLOOT_FLAG);
				}
			}

		private:
			void insert(value_type a_value)
			{
				for (std::size_t i = 0; i < RE::INPUT_DEVICES::kTotal; ++i) {
					_mappings[i].insert(a_value);
				}
			}

			struct cicompare
			{
				[[nodiscard]] bool operator()(const value_type& a_lhs, const value_type& a_rhs) const
				{
					return _stricmp(a_lhs.data(), a_rhs.data()) < 0;
				}
			};

			std::array<std::set<value_type, cicompare>, RE::INPUT_DEVICES::kTotal> _mappings;
		};

		class IDCodeMap
		{
		public:
			using value_type = std::uint32_t;

			IDCodeMap()
			{
				using Device = RE::INPUT_DEVICE;
				using Gamepad = RE::BSWin32GamepadDevice::Key;
				using Keyboard = RE::BSWin32KeyboardDevice::Key;
				using Mouse = RE::BSWin32MouseDevice::Key;

				insert(Group::kPageKeys, false, Device::kKeyboard, { Keyboard::kPageUp, Keyboard::kPageDown });
				insert(Group::kArrowKeys, false, Device::kKeyboard, { Keyboard::kUp, Keyboard::kDown, Keyboard::kLeft, Keyboard::kRight });

				insert(Group::kMouseWheel, true, Device::kMouse, { Mouse::kWheelUp, Mouse::kWheelDown });
				insert(Group::kDPAD, true, Device::kGamepad, { Gamepad::kUp, Gamepad::kDown, Gamepad::kLeft, Gamepad::kRight });
			}

			void operator()(std::size_t a_device, RE::ControlMap::UserEventMapping& a_userEvent) const
			{
				const auto& mapping = _mappings[a_device];
				auto it = mapping.find(a_userEvent.inputKey);
				if (it != mapping.end()) {
					it->second->accept(a_userEvent);
				}
			}

			void commit()
			{
				for (const auto& mapping : _mappings) {
					for (const auto& [id, group] : mapping) {
						group->commit();
					}
				}
			}

		private:
			class IControlGroup
			{
			public:
				using value_type = RE::ControlMap::UserEventMapping;

				IControlGroup(Group a_group, bool a_mandatory) noexcept :
					_group(a_group), _mandatory(a_mandatory)
				{}

				virtual ~IControlGroup() = default;

				void accept(value_type& a_mapping)
				{
					if (!_good) return;

					if (const bool canAccept = _mandatory || !a_mapping.linked) {
						_queued.emplace_back(a_mapping);
					} else {
						_good = false;
					}
				}

				void commit() noexcept
				{
					using UEFlag = RE::UserEvents::USER_EVENT_FLAG;

					if (_good) {
						for (auto& todo : _queued) {
							auto& mapping = todo.get();
							if (mapping.userEventGroupFlag.all(UEFlag::kInvalid)) {
								mapping.userEventGroupFlag = UEFlag::kNone;
							}

							mapping.userEventGroupFlag.set(QUICKLOOT_FLAG);
						}
					}

					ControlGroups::get()[_group] = _good;
				}

			private:
				std::vector<std::reference_wrapper<value_type>> _queued;
				Group _group;
				bool _good{ true };
				bool _mandatory;
			};

			void insert(Group a_group, bool a_mandatory, RE::INPUT_DEVICE a_device, std::initializer_list<value_type> a_idCodes)
			{
				const auto group = std::make_shared<IControlGroup>(a_group, a_mandatory);
				for (const auto& idCode : a_idCodes) {
					_mappings[a_device].emplace(idCode, group);
				}
			}

			std::array<std::map<value_type, std::shared_ptr<IControlGroup>>, RE::INPUT_DEVICES::kTotal> _mappings;
		};

		InputManager() = delete;
		InputManager(const InputManager&) = delete;
		InputManager(InputManager&&) = delete;

		~InputManager() = delete;

		InputManager& operator=(const InputManager&) = delete;
		InputManager& operator=(InputManager&&) = delete;

		    static void RefreshLinkedMappings(RE::ControlMap* a_controlMap)
		{
			_RefreshLinkedMappings(a_controlMap);
			if (!a_controlMap) {
				return;
			}

			const auto for_each = [&](std::function<void(RE::ControlMap::UserEventMapping&, std::size_t)> a_functor) {
				std::size_t k_total = RE::UserEvents::INPUT_CONTEXT_ID::kTotal;

				if (REL::Module::get().version().compare(SKSE::RUNTIME_SSE_1_6_1130) == std::strong_ordering::less) {
					k_total = 17; // Hardcoded for Skyrim 1.6.640 or lower
				}

				std::size_t i_total = RE::INPUT_DEVICES::kFlatTotal;

				for (std::size_t k = 0; k < k_total; ++k) {
					auto& map = a_controlMap->controlMap[k];
					if (map) {
						for (std::size_t i = 0; i < i_total; ++i) {
							for (auto& userMapping : map->deviceMappings[i]) {
								a_functor(userMapping, i);
							}
						}
					}
				}
			};

			for_each([=](RE::ControlMap::UserEventMapping& a_mapping, std::size_t) {
				if (a_mapping.userEventGroupFlag.none(RE::UserEvents::USER_EVENT_FLAG::kInvalid)) {
					a_mapping.userEventGroupFlag.reset(QUICKLOOT_FLAG);
				}
			});

			UserEventMap eventMap;
			IDCodeMap idMap;

			for_each([&](RE::ControlMap::UserEventMapping& a_mapping, std::size_t a_device) {
				eventMap(a_device, a_mapping);
				idMap(a_device, a_mapping);
			});

			idMap.commit();
			a_controlMap->ToggleControls(QUICKLOOT_FLAG, true);
		}

		inline static REL::Relocation<decltype(RefreshLinkedMappings)> _RefreshLinkedMappings;
	};
}
