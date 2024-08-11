#pragma once

#include <tsl/ordered_map.h>

namespace SpellExperience
{
	inline std::string nameFilter{};

	class Manager :
		public ISingleton<Manager>
	{
	public:
		//static void Register();

		void LoadMCMSettings(const CSimpleIniA& a_ini);
		void InitialiseArchetypes();

		bool IsValid();
		void Draw();

		bool IsSpellExperienceOpen() const;
		void SetSpellExperienceOpen(bool a_open);
		void ToggleActive();

	private:
		void DrawArchetypeList();

		// membersMap
		bool                                                     spellHistoryOpen{ false };
		bool                                                     menuOpenedJustNow{ false };
		bool                                                     unpauseMenu{ false };
		bool                                                     blurMenu{ true };
		std::string                                              currentArchetype = "";
		std::map<std::string, tsl::ordered_map<std::string, RE::FormID>> archetypeGlobals{};

	};

	inline void Manager::DrawArchetypeList()
	{
		ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, 0.0f);
		{
			for (auto& [archName, globalsMap] : archetypeGlobals) {
				// get the global values to check if this archetype is 'discovered' or not
				for (auto& [experienceLabel, globalVar] : globalsMap) {
					auto globalVarForm = RE::TESForm::LookupByID(globalVar)->As<RE::TESGlobal>();
					if (globalVarForm && globalVarForm->value > 0) {
						// we can show this one, so show it

						std::string archNameLower = clib_util::string::tolower(archName);
						std::string nameFilterLower = clib_util::string::tolower(nameFilter);

						if (nameFilter.empty() || archNameLower.contains(nameFilterLower)) {
							auto leafFlags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_SpanAvailWidth;
							auto is_selected = currentArchetype == archName;
							if (is_selected) {
								leafFlags |= ImGuiTreeNodeFlags_Selected;
								ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImGui::GetStyleColorVec4(ImGuiCol_Header));
							}
							ImGui::TreeNodeEx(archName.c_str(), leafFlags);
							if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
								if (archName != currentArchetype) {
									currentArchetype = archName;
									RE::PlaySound("UIMenuFocus");
								}
							}
							if (is_selected) {
								ImGui::PopStyleColor();
							}
						}
						break;  // moves back to next archetype
					}
				}
			}
		}
		ImGui::PopStyleVar();
	}
}
