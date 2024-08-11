#include "SpellExperience.h"

#include "Hooks.h"
#include "Hotkeys.h"
#include "ImGui/IconsFonts.h"
#include "ImGui/Renderer.h"
#include "ImGui/Styles.h"
#include "ImGui/Util.h"

namespace SpellExperience
{
	void Manager::InitialiseArchetypes()
	{
		auto archetypeFormList = RE::TESForm::LookupByEditorID("_SR_ListGlobalsArchetypes")->As<RE::BGSListForm>();

		archetypeGlobals.clear();

		int archCounter = 0;
		archetypeFormList->ForEachForm([&](RE::TESForm* form) {
			std::string translationKey = "$SR_Archetype" + std::to_string(archCounter);
			std::string archName = Translation::Manager::GetSingleton()->GetTranslation(translationKey);
			archetypeGlobals[archName] = {};

			auto archetypeList = form->As<RE::BGSListForm>();
			archetypeList->ForEachForm([&](RE::TESForm* form) {
				std::string editorId(form->GetFormEditorID());
				std::string label = "";
				if (editorId.contains("Novice")) {
					label = "$SR_Novice"_T;
				} else if (editorId.contains("Apprentice")) {
					label = "$SR_Apprentice"_T;
				} else if (editorId.contains("Adept")) {
					label = "$SR_Adept"_T;
				} else if (editorId.contains("Expert")) {
					label = "$SR_Expert"_T;
				} else if (editorId.contains("Master")) {
					label = "$SR_Master"_T;
				}
				archetypeGlobals[archName][label] = form->GetFormID();

				return RE::BSContainer::ForEachResult::kContinue;
			});

			archCounter++;
			return RE::BSContainer::ForEachResult::kContinue;
		});
	}

	void Manager::LoadMCMSettings(const CSimpleIniA& a_ini)
	{
		unpauseMenu = a_ini.GetBoolValue("Settings", "bUnpauseSpellExperience", unpauseMenu);
		blurMenu = a_ini.GetBoolValue("Settings", "bBlurSpellExperience", blurMenu);
	}

	bool Manager::IsValid()
	{
		if (IsSpellExperienceOpen()) {
			return true;
		}

		static constexpr std::array badMenus{
			RE::MainMenu::MENU_NAME,
			RE::MistMenu::MENU_NAME,
			RE::LoadingMenu::MENU_NAME,
			RE::FaderMenu::MENU_NAME,
			"LootMenu"sv,
			"CustomMenu"sv
		};

		if (const auto UI = RE::UI::GetSingleton();
			!UI || !UI->IsShowingMenus() || std::ranges::any_of(badMenus, [&](const auto& menuName) { return UI->IsMenuOpen(menuName); })) {
			return false;
		}

		if (const auto* controlMap = RE::ControlMap::GetSingleton();
			!controlMap || controlMap->contextPriorityStack.back() != RE::UserEvents::INPUT_CONTEXT_ID::kGameplay) {
			return false;
		}

		if (PhotoMode::IsPhotoModeActive()) {
			return false;
		}

		return true;
	}

	void Manager::Draw()
	{
		if (!IsSpellExperienceOpen()) {
			return;
		}

		ImGui::SetNextWindowPos(ImGui::GetNativeViewportPos());
		ImGui::SetNextWindowSize(ImGui::GetNativeViewportSize());

		ImGui::Begin("##Main", nullptr, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBringToFrontOnFocus);
		{
			constexpr auto windowFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar;

			ImGui::SetNextWindowPos(ImGui::GetNativeViewportCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));

			ImGui::PushFont(MANAGER(IconFont)->GetGlobalHistoryFont());

			ImGui::BeginChild("##GlobalHistory", ImGui::GetNativeViewportSize() * 0.8f, ImGuiChildFlags_Border, windowFlags);
			{
				ImGui::ExtendWindowPastBorder();

				ImGui::PushFont(MANAGER(IconFont)->GetHeaderFont());
				{
					ImGui::Indent();
					ImGui::TextUnformatted("$SR_Title"_T);
					ImGui::Unindent();
				}
				ImGui::PopFont();

				ImGui::Spacing(2);
				ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal, ImGui::GetUserStyleVar(ImGui::USER_STYLE::kSeparatorThickness));
				ImGui::Spacing(2);

				auto childSize = ImGui::GetContentRegionAvail();

				float toggleHeight = ImGui::GetFrameHeight() / 1.5f;
				float toggleButtonOffset = ImGui::CalcTextSize("$DH_Date_Text"_T).x + ImGui::GetStyle().ItemSpacing.x + toggleHeight * 0.5f;

				ImGui::BeginGroup();
				{
					auto startPos = childSize.x * 0.25f;                                                        // search box end
					auto endPos = (childSize.x * 0.5f) - toggleButtonOffset - ImGui::GetStyle().ItemSpacing.x;  // "By Date" text start

					ImGui::BeginChild("##Map", { (startPos + endPos) * 0.5f, childSize.y * 0.9125f }, ImGuiChildFlags_None, ImGuiWindowFlags_NoBackground);
					{
						DrawArchetypeList();
					}
					ImGui::EndChild();

					ImGui::SameLine();
					ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical, ImGui::GetUserStyleVar(ImGui::USER_STYLE::kSeparatorThickness));
					ImGui::SameLine();

					childSize = ImGui::GetContentRegionAvail();

					if (!currentArchetype.empty()) {
						ImGui::BeginChild("##Experience", ImVec2(0, childSize.y * 0.9125f), ImGuiChildFlags_None, windowFlags | ImGuiWindowFlags_NoBackground);
						{
							ImGui::Indent();
							{
								ImGui::PushFont(MANAGER(IconFont)->GetHeaderFont());
								{
									ImGui::CenteredText(currentArchetype.c_str(), false);
								}
								ImGui::PopFont();
								ImGui::Spacing(4);

								auto childSizeExperience = ImGui::GetContentRegionAvail();

								ImGui::BeginChild("##ExperienceSections", childSizeExperience, ImGuiChildFlags_None, ImGuiWindowFlags_NoBackground);
								{
									// get the formlist for this archetype - it will have 5 globals in it
									auto globalsMap = archetypeGlobals[currentArchetype];
									for (auto& [experienceLabel, globalVar] : globalsMap) {
										auto globalVarForm = RE::TESForm::LookupByID(globalVar)->As<RE::TESGlobal>();

										ImGui::Text((experienceLabel + ":").c_str());
										ImGui::SameLine(450);

										if (globalVarForm) {
											std::string expValue = std::format("{:.0f}", globalVarForm->value);
											ImGui::Text(expValue.c_str());
										}
										ImGui::Spacing(2);
									}
								}
								ImGui::EndChild();
							}
							ImGui::Unindent();
						}
						ImGui::EndChild();
					}
				}
				ImGui::EndGroup();

				ImGui::Spacing(2);
				ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal, ImGui::GetUserStyleVar(ImGui::USER_STYLE::kSeparatorThickness));

				childSize = ImGui::GetContentRegionAvail();

				ImGui::BeginChild("##BottomBar", ImVec2(childSize.x, childSize.y), ImGuiChildFlags_None, windowFlags | ImGuiWindowFlags_NoBackground);
				{
					childSize = ImGui::GetContentRegionMax();

					ImGui::SameLine();
					ImGui::SetCursorPosY(childSize.y * 0.125f);
					ImGui::SetNextItemWidth(childSize.x * 0.25f);
					if (ImGui::InputTextWithHint("##Name", "$SR_Filter_Text"_T, &nameFilter)) {
						currentArchetype = "";
					}
				}
				ImGui::EndChild();
			}
			ImGui::EndChild();

			ImGui::PopFont();

			ImGui::PushFont(MANAGER(IconFont)->GetButtonFont());
			{
				const auto icon = MANAGER(Hotkeys)->EscapeIcon();

				// exit button position (1784,1015) + offset (32) at 1080p
				static const auto windowSize = RE::BSGraphics::Renderer::GetScreenSize();
				static float      posY = 0.93981481481f * windowSize.height;
				static float      posX = 0.92916666666f * windowSize.width;

				ImGui::SetCursorScreenPos({ posX, posY });
				ImGui::ButtonIconWithLabel("$SR_Exit_Button"_T, icon);
				if (ImGui::IsItemClicked()) {
					SetSpellExperienceOpen(false);
				}
			}
			ImGui::PopFont();
		}
		ImGui::End();

		if (ImGui::IsKeyReleased(ImGuiKey_Escape) || ImGui::IsKeyReleased(ImGuiKey_NavGamepadCancel)) {
			SetSpellExperienceOpen(false);
		}
	}

	bool Manager::IsSpellExperienceOpen() const
	{
		return spellHistoryOpen;
	}

	void Manager::SetSpellExperienceOpen(bool a_open)
	{
		spellHistoryOpen = a_open;
		menuOpenedJustNow = a_open;

		if (a_open) {
			ImGui::Styles::GetSingleton()->RefreshStyle();

			if (blurMenu) {
				RE::UIBlurManager::GetSingleton()->IncrementBlurCount();
			}

			// hides compass but not notifications
			RE::SendHUDMessage::PushHUDMode("WorldMapMode");
			RE::UIMessageQueue::GetSingleton()->AddMessage(RE::CursorMenu::MENU_NAME, RE::UI_MESSAGE_TYPE::kShow, nullptr);

			RE::PlaySound("UIMenuOK");

		} else {
			currentArchetype = "";

			nameFilter.clear();

			if (blurMenu) {
				RE::UIBlurManager::GetSingleton()->DecrementBlurCount();
			}

			RE::SendHUDMessage::PopHUDMode("WorldMapMode");
			RE::UIMessageQueue::GetSingleton()->AddMessage(RE::CursorMenu::MENU_NAME, RE::UI_MESSAGE_TYPE::kHide, nullptr);

			RE::PlaySound("UIMenuCancel");
		}

		if (!unpauseMenu) {
			RE::Main::GetSingleton()->freezeTime = a_open;
		}

		ImGui::Renderer::RenderMenus(a_open);
	}

	void Manager::ToggleActive()
	{
		if (!IsValid()) {
			return;
		}

		SetSpellExperienceOpen(!IsSpellExperienceOpen());
	}

}
