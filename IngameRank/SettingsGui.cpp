#include "pch.h"
#include "IngameRank.h"

void IngameRank::RenderSettings() {

	ImGui::NewLine();

	CVarWrapper enableCvar = cvarManager->getCvar("ingamerank_enabled");
	if (enableCvar) {
		bool enabled = enableCvar.getBoolValue();
		if (ImGui::Checkbox("Enable Plugin", &enabled)) {
			enableCvar.setValue(enabled);
		}
	}

	CVarWrapper divisionCvar = cvarManager->getCvar("ingamerank_show_division");
	if (divisionCvar) {
		bool value = divisionCvar.getBoolValue();
		if (ImGui::Checkbox("Show Division", &value)) {
			divisionCvar.setValue(value);
		}
	}

	CVarWrapper showPlaylistCvar = cvarManager->getCvar("ingamerank_show_playlist");
	if (showPlaylistCvar) {
		bool value = showPlaylistCvar.getBoolValue();
		if (ImGui::Checkbox("Show Playlist", &value)) {
			showPlaylistCvar.setValue(value);
		}
	}


	CVarWrapper unrankedCvar = cvarManager->getCvar("ingamerank_calculate_unranked");
	if (unrankedCvar) {
		bool value = unrankedCvar.getBoolValue();
		if (ImGui::Checkbox("Calculate Unranked Ranks (only Solo, Doubles and Standard)", &value)) {
			unrankedCvar.setValue(value);
		}
	}
	
	ImGui::NewLine();
	ImGui::TextUnformatted("Playlist");
	//ImGui::Separator();

	CVarWrapper extramodesCvar = cvarManager->getCvar("ingamerank_include_extramodes");
	CVarWrapper tournamentsCvar = cvarManager->getCvar("ingamerank_include_tournaments");
	CVarWrapper playlistCvar = cvarManager->getCvar("ingamerank_playlist");
	int selectedPlaylist = playlistCvar.getIntValue();
	if (ImGui::BeginCombo("", PLAYLIST_NAMES.at(selectedPlaylist).name.c_str()))
	{
		for (int key : PLAYLIST_ORDER)
		{
			bool disabled = (PLAYLIST_NAMES.at(key).condition == PLCondition::EXTRAMODE and !extramodesCvar.getBoolValue())
				|| (PLAYLIST_NAMES.at(key).condition == PLCondition::TOURNAMENT and !tournamentsCvar.getBoolValue());

			const bool is_selected = (selectedPlaylist == key);
			if (ImGui::Selectable(PLAYLIST_NAMES.at(key).name.c_str(), is_selected, ImGuiSelectableFlags_Disabled * disabled)) {
				selectedPlaylist = key;
				playlistCvar.setValue(key);
			}

			// Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
			if (is_selected)
				ImGui::SetItemDefaultFocus();
		}
		ImGui::EndCombo();
	}

	if (extramodesCvar) {
		bool value = extramodesCvar.getBoolValue();
		if (ImGui::Checkbox("Include Extra Modes", &value)) {
			extramodesCvar.setValue(value);
		}
	}

	if (tournamentsCvar) {
		bool value = tournamentsCvar.getBoolValue();
		if (ImGui::Checkbox("Include Tournaments", &value)) {
			tournamentsCvar.setValue(value);
		}
	}


	ImGui::NewLine();
	ImGui::Separator();
	ImGui::TextUnformatted("Cycle playlist with \"F8\" or change it in the console with");
	ImGui::TextUnformatted("bind <key> ingamerank_cycleplaylist");
}
