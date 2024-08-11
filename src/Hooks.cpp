#include "Hooks.h"
#include "SpellExperience.h"
#include "Input.h"

namespace Hooks
{
	struct ProcessInputQueue
	{
		static void thunk(RE::BSTEventSource<RE::InputEvent*>* a_dispatcher, RE::InputEvent* const* a_events)
		{
			if (a_events) {
				MANAGER(Input)->ProcessInputEvents(a_events);
			}

			if (MANAGER(SpellExperience)->IsSpellExperienceOpen()) {
				constexpr RE::InputEvent* const dummy[] = { nullptr };
				func(a_dispatcher, dummy);
			} else {
				func(a_dispatcher, a_events);
			}
		}
		static inline REL::Relocation<decltype(thunk)> func;
	};

	struct IsMenuOpen
	{
		static bool thunk(RE::UI* a_this, const RE::BSFixedString& a_menu)
		{
			auto result = func(a_this, a_menu);

			if (!result) {
				result = MANAGER(SpellExperience)->IsSpellExperienceOpen();
			}

			return result;
		}
		static inline REL::Relocation<decltype(thunk)> func;
	};

	struct TakeScreenshot
	{
		static void thunk(char const* a_path, RE::BSGraphics::TextureFileFormat a_format)
		{
			func(a_path, a_format);

			if (MANAGER(SpellExperience)->IsSpellExperienceOpen()) {
				// reshow cursor after Debug.Notification hides it
				SKSE::GetTaskInterface()->AddUITask([] {
					RE::UIMessageQueue::GetSingleton()->AddMessage(RE::CursorMenu::MENU_NAME, RE::UI_MESSAGE_TYPE::kShow, nullptr);
				});
			}
		}
		static inline REL::Relocation<decltype(thunk)> func;
	};

	void Install()
	{
		REL::Relocation<std::uintptr_t> inputUnk(RELOCATION_ID(67315, 68617), 0x7B);
		stl::write_thunk_call<ProcessInputQueue>(inputUnk.address());

		REL::Relocation<std::uintptr_t> hudMenuUserEvent(RELOCATION_ID(50748, 51643), 0x1E);
		stl::write_thunk_call<IsMenuOpen>(hudMenuUserEvent.address());

		REL::Relocation<std::uintptr_t> take_ss{ RELOCATION_ID(35556, 36555), OFFSET(0x48E, 0x454) };  // Main::Swap
		stl::write_thunk_call<TakeScreenshot>(take_ss.address());

		logger::info("Installed dialogue hooks");
	}
}
