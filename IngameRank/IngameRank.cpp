#include "pch.h"
#include "IngameRank.h"

#include <unordered_set>
#include "ScoreboardPosition.h"


BAKKESMOD_PLUGIN(IngameRank, "IngameRank", plugin_version, PLUGINTYPE_FREEPLAY)

std::shared_ptr<CVarManagerWrapper> _globalCvarManager;

void IngameRank::onLoad() {
	_globalCvarManager = cvarManager;

	//Register cvars
	cvarManager->registerCvar("ingamerank_enabled", "1", "Enable the plugin", false, true, 0.0f, true, 1.0f, true).bindTo(pluginActive);
	cvarManager->registerCvar("ingamerank_show_division", "0", "Show the division", false, true, 0.0f, true, 1.0f, true);
	cvarManager->registerCvar("ingamerank_show_playlist", "1", "Show the playlist", false, true, 0.0f, true, 1.0f, true);
	cvarManager->registerCvar("ingamerank_playlist", "-1", "What playlist to show the ranks of", false, false, -1.0f, false, 30.0f, true);
	cvarManager->registerCvar("ingamerank_include_extramodes", "0", "Include extra modes in playlists", false, true, 0.0f, true, 1.0f, true);
	cvarManager->registerCvar("ingamerank_include_tournaments", "0", "Include tournaments in playlists", false, true, 0.0f, true, 1.0f, true);
	cvarManager->registerCvar("ingamerank_calculate_unranked", "1", "Calculate unranked ranks based on mmr", false, true, 0.0f, true, 1.0f, true);
	cvarManager->registerNotifier("ingamerank_cycleplaylist", std::bind(&IngameRank::cyclePlaylist, this, std::placeholders::_1), "Cycles the playlist", PERMISSION_ALL);


	// Handle the change of the playlist selection
	cvarManager->getCvar("ingamerank_playlist").addOnValueChanged([this](std::string old, CVarWrapper cvar) {
		if (cvar.IsNull() || !cvarManager || !gameWrapper) return;

		// If the value is invalid reset it to -1 (Current)
		int value = cvar.getIntValue();
		if (PLAYLIST_NAMES.find(value) == PLAYLIST_NAMES.end()) {
			cvar.setValue(-1);
			return;
		}
		else if (PLAYLIST_NAMES.at(value).condition == PLCondition::EXTRAMODE && !cvarManager->getCvar("ingamerank_include_extramodes").getBoolValue()  // Playlist set to an extra mode, but extra modes not enabled
			|| PLAYLIST_NAMES.at(value).condition == PLCondition::TOURNAMENT && !cvarManager->getCvar("ingamerank_include_tournaments").getBoolValue()) // Playlist set to tournaments, but tournaments not enabled
		{
			cvar.setValue(-1);
			return;
		}
		else if (*pluginActive && !isSBOpen) {
			// Display the new playlist selection if the scoreboard is not already open

			playlist_changed = std::chrono::system_clock::now();
			uiScale = gameWrapper->GetInterfaceScale() * gameWrapper->GetDisplayScale();
			canvas_size = gameWrapper->GetScreenSize();
			sbPosInfo.scale = getSbPosInfo(canvas_size, uiScale, false, 0, 0, false).scale;	// Compute scaling factor if not already done by updateDisplay()
			gameWrapper->RegisterDrawable(std::bind(&IngameRank::renderPlaylist, this, std::placeholders::_1));
		}

		if (value == -1) {
			if (gameWrapper->IsInOnlineGame()) {
				// Set the display plalist to the current gamemode if playlist is set to -1 (Current)
				MMRWrapper mmrWrapper = gameWrapper->GetMMRWrapper();
				int current_playlist = mmrWrapper.GetCurrentPlaylist();

				if (PLAYLIST_NAMES.find(current_playlist) == PLAYLIST_NAMES.end()) {
					// If the current playlist is not in the list (aka not ranked), just set the display to 0 (Best)
					display_playlist = 0;
				}
				else {
					// Otherwise set it to the current
					display_playlist = current_playlist;
				}
			}
			else {
				display_playlist = 0;
			}
		}
		else {
			// Otherwise set it to the selected value
			display_playlist = value;
		}

		// Refresh the display with the new playlist selection
		updateDisplay();
		});

	cvarManager->getCvar("ingamerank_include_extramodes").addOnValueChanged([this](std::string, CVarWrapper cvar) {
		if (cvar.IsNull() && !cvarManager) return;

		if (!cvar.getBoolValue()) {
			// If the value is set to disabled and the selected playlist is an extra mode, set it to the default -1 (Current)
			CVarWrapper playlist_cvar = cvarManager->getCvar("ingamerank_playlist");
			if (playlist_cvar.IsNull()) return;
			int playlist = playlist_cvar.getIntValue();
			if (PLAYLIST_NAMES.at(playlist).condition == PLCondition::EXTRAMODE) playlist_cvar.setValue(-1);
		}
		});

	cvarManager->getCvar("ingamerank_include_tournaments").addOnValueChanged([this](std::string, CVarWrapper cvar) {
		if (cvar.IsNull() || !cvarManager) return;

		if (!cvar.getBoolValue()) {
			// If the value is set to disabled and the selected playlist is set to tournament, set it to the default -1 (Current)
			CVarWrapper playlist_cvar = cvarManager->getCvar("ingamerank_playlist");
			if (playlist_cvar.IsNull()) return;
			if (PLAYLIST_NAMES.at(playlist_cvar.getIntValue()).condition == PLCondition::TOURNAMENT) playlist_cvar.setValue(-1);
		}
		});


	//Hook events
	gameWrapper->HookEvent("Function TAGame.GFxData_GameEvent_TA.OnOpenScoreboard", std::bind(&IngameRank::openScoreboard, this, std::placeholders::_1));
	gameWrapper->HookEvent("Function TAGame.GFxData_GameEvent_TA.OnCloseScoreboard", std::bind(&IngameRank::closeScoreboard, this, std::placeholders::_1));
	gameWrapper->HookEvent("Function TAGame.GameEvent_Soccar_TA.OnMatchEnded", [this](std::string eventName) {
		if (isSBOpen) closeScoreboard("");
		});
	gameWrapper->HookEvent("Function TAGame.PRI_TA.OnTeamChanged", [this](std::string event_name) {updateDisplay(); });
	gameWrapper->HookEvent("Function TAGame.GRI_TA.Destroyed", std::bind(&IngameRank::griDestroyed, this, std::placeholders::_1));
	gameWrapper->HookEvent("Function TAGame.GameEvent_Soccar_TA.OnAllTeamsCreated", [this](std::string eventName) {
		// Called when the match is loaded, so we can set the display playlist if the selection is on -1 (Current)

		if (!cvarManager || !gameWrapper) return;

		CVarWrapper playlist_cvar = cvarManager->getCvar("ingamerank_playlist");
		if (playlist_cvar.IsNull()) return;

		if (playlist_cvar.getIntValue() == -1) {
			// If the playlist selection is "Current" set display playlist to the current playlist

			int current_playlist = gameWrapper->GetMMRWrapper().GetCurrentPlaylist();
			if (PLAYLIST_NAMES.find(current_playlist) == PLAYLIST_NAMES.end()) {
				// If the current playlist is not ranked, set the display to 0 (Best)
				display_playlist = 0;
			}
			else {
				// Otherwise set it to the current playlist
				display_playlist = current_playlist;
			}
		}
		else {
			// Otherwise set it to the selected playlist

			if (PLAYLIST_NAMES.find(playlist_cvar.getIntValue()) == PLAYLIST_NAMES.end()) {
				// Make sure the playlist cvar has a valid value
				display_playlist = 0;
			}
			else {
				display_playlist = playlist_cvar.getIntValue();
			}
		}
		});

	gameWrapper->HookEvent("Function ReplayDirector_TA.Playing.EndState", [this](std::string eventName) {
		// Refresh the display if an instant replay has ended
		isReplaying = false;
		updateDisplay();
		});
	gameWrapper->HookEvent("Function ReplayDirector_TA.Playing.BeginState", [this](std::string eventName) {
		// Refresh the display if an instant replay has started so the images can move out of the way of the check marks
		isReplaying = true;
		updateDisplay();
		});

	// Handle changes in the team and "Ghost Players" staying on the scoreboard
	gameWrapper->HookEventWithCaller<ActorWrapper>(
		"Function TAGame.GameEvent_Soccar_TA.ScoreboardSort",
		[this](ActorWrapper gameEvent, void* params, std::string eventName) {
			RecordScoreboardComparison(gameEvent, params, eventName);
		});
	gameWrapper->HookEventWithCaller<ActorWrapper>(
		"Function TAGame.PRI_TA.GetScoreboardStats",
		[this](auto args...) { ComputeScoreboardInfo();
		});
	gameWrapper->HookEvent("Function TAGame.GameEvent_Soccar_TA.Destroyed", [this](...) {
		comparisons.clear();
		ComputeScoreboardInfo();
		});
	// End ghost players

	//Handle mmr changes
	mmrToken = gameWrapper->GetMMRWrapper().RegisterMMRNotifier([this](UniqueIDWrapper uid) {updateRankFor(uid, true); });

	//Load images
	std::filesystem::path dataFolder = gameWrapper->GetDataFolderW();
	dataFolder = dataFolder / "assets" / "IngameRank";
	for (size_t i = 0; i < 24; i++) tiers[i] = std::make_shared<ImageWrapper>(dataFolder / "Tiers" / (std::to_string(i) + ".png"), true);
	for (size_t i = 0; i < 8; i++) divisions[i] = std::make_shared<ImageWrapper>(dataFolder / "Divisions" / (std::to_string(i) + ".png"), true);
	for (size_t i = 0; i < 8; i++) playlists[i] = std::make_shared<ImageWrapper>(dataFolder / "Playlists" / (std::to_string(i) + ".png"), true);

	//First launch condition
	if (std::filesystem::exists(dataFolder / "FIRST_LAUNCH.file")) {
		cvarManager->log("FirstLaunch");
		cvarManager->setBind("F8", "ingamerank_cycleplaylist");
		std::filesystem::remove(dataFolder / "FIRST_LAUNCH.file");
	}
}

void IngameRank::onUnload() {
	gameWrapper->UnregisterDrawables();
	gameWrapper->UnhookEvent("Function TAGame.GFxData_GameEvent_TA.OnOpenScoreboard");
	gameWrapper->UnhookEvent("Function TAGame.GFxData_GameEvent_TA.OnCloseScoreboard");
	gameWrapper->UnhookEvent("Function TAGame.PRI_TA.OnTeamChanged");
	gameWrapper->UnhookEvent("Function TAGame.GameEvent_Soccar_TA.OnMatchEnded");
	gameWrapper->UnhookEvent("Function TAGame.GRI_TA.Destroyed");
	gameWrapper->UnhookEvent("Function TAGame.GameEvent_Soccar_TA.OnAllTeamsCreated");
	gameWrapper->UnhookEvent("Function ReplayDirector_TA.Playing.EndState");
	gameWrapper->UnhookEvent("Function ReplayDirector_TA.Playing.BeginState");
	gameWrapper->UnhookEvent("Function TAGame.GameEvent_Soccar_TA.ScoreboardSort");
	gameWrapper->UnhookEvent("Function TAGame.PRI_TA.GetScoreboardStats");
	gameWrapper->UnhookEvent("Function TAGame.GameEvent_Soccar_TA.Destroyed");
}

void IngameRank::openScoreboard(std::string eventName) {
	if (!*pluginActive) return;
	if (!gameWrapper) return;
#ifdef _DEBUG
	// When _DEBUG is defined allow the plugin to run in private games aswell
	if (!gameWrapper->IsInGame() && !gameWrapper->IsInOnlineGame() || gameWrapper->IsInFreeplay() || gameWrapper->IsInReplay()) return;
#else
	// Else only allow in online
	if (!gameWrapper->IsInOnlineGame()) return;
	MMRWrapper mmrWrapper = gameWrapper->GetMMRWrapper();
	if (std::find(EXCLUDED_PLAYLISTS.begin(), EXCLUDED_PLAYLISTS.end(), mmrWrapper.GetCurrentPlaylist()) != EXCLUDED_PLAYLISTS.end()) return;
#endif
	isSBOpen = true;

	updateDisplay();
}

void IngameRank::closeScoreboard(std::string eventName) {
	if (!gameWrapper) return;

	if (isSBOpen) {
		gameWrapper->UnregisterDrawables();
		toRender.clear();
		isSBOpen = false;
	}
}

void IngameRank::griDestroyed(std::string eventName) {
	computedInfo.sortedPlayers.clear();
	player_ranks.clear();
	//mmrs.clear();
	closeScoreboard("");
}

void IngameRank::updateDisplay() {
	if (!gameWrapper || !cvarManager) return;
	if (!isSBOpen) return;

	ServerWrapper sw = gameWrapper->IsInOnlineGame() ? gameWrapper->GetOnlineGame() : gameWrapper->GetGameEventAsServer();
	if (sw.IsNull()) return;

	MMRWrapper mmrWrapper = gameWrapper->GetMMRWrapper();
	if (sw.GetbMatchEnded()) {
		closeScoreboard("");
		return; 
	}

	// whether the 'show player mmr' cvars are enabled
	show_rank_on = cvarManager->getCvar("ranked_showranks").getBoolValue() && cvarManager->getCvar("ranked_showranks_casual").getBoolValue();

	toRender.clear();

	canvas_size = gameWrapper->GetScreenSize();
	uiScale = gameWrapper->GetInterfaceScale() * gameWrapper->GetDisplayScale();
	mutators = mmrWrapper.GetCurrentPlaylist() == 34; // When playing a tournament
	sbPosInfo = getSbPosInfo(canvas_size, uiScale, mutators, computedInfo.bluePlayerCount, computedInfo.orangePlayerCount, isReplaying);

	//Get cvars
	int playlist = display_playlist;
	bool show_division = cvarManager->getCvar("ingamerank_show_division").getBoolValue();
	bool calculate_unranked = cvarManager->getCvar("ingamerank_calculate_unranked").getBoolValue();
	bool include_extras = cvarManager->getCvar("ingamerank_include_extramodes").getBoolValue();
	bool include_tournaments = cvarManager->getCvar("ingamerank_include_tournaments").getBoolValue();
	bool show_playlist = cvarManager->getCvar("ingamerank_show_playlist").getBoolValue();

	if (PLAYLIST_NAMES.find(playlist) == PLAYLIST_NAMES.end()) { //This should be handled in the OnValueChanged but better safe than sorry.
		playlist = 0;
	}

	int blues = -1;  // For counting players to be able to calculate image positions
	int oranges = -1;
	for (Pri pri : computedInfo.sortedPlayers)
	{
		if (pri.team == 0) blues++;
		else if (pri.team == 1) oranges++;
		if (pri.isBot || pri.team > 1) continue;

		DisplayRank displayRank = displayRankOf(pri, include_extras, include_tournaments, calculate_unranked);

		precomputeRankImages(pri, displayRank, oranges, blues, playlist, show_division, show_playlist, calculate_unranked);
	}
	gameWrapper->UnregisterDrawables();
	gameWrapper->RegisterDrawable(std::bind(&IngameRank::render, this, std::placeholders::_1));
}

//
// Would be used for alphabetical sorting
//
//std::string IngameRank::to_lower(std::string s) {
//	std::for_each(s.begin(), s.end(), [](char& c) {
//		c = std::tolower(c);
//		});
//	return s;
//}
//
//bool IngameRank::compareName(int mmr1, std::string name1, int mmr2, std::string name2) {
//	if (mmr1 < mmr2) return true;
//	else if (mmr1 == mmr2) {
//		return to_lower(name1).compare(to_lower(name2)) == -1;
//	}
//	else return false;
//}
//

void IngameRank::cyclePlaylist(std::vector<std::string> params) {
	if (!*pluginActive) return;
	if (!cvarManager) return;

	int playlist = cvarManager->getCvar("ingamerank_playlist").getIntValue();
	bool extramodes = cvarManager->getCvar("ingamerank_include_extramodes").getBoolValue();
	bool tournaments = cvarManager->getCvar("ingamerank_include_tournaments").getBoolValue();

	int newPlaylist = playlist;
	int playlist_index = PLAYLIST_NAMES.at(playlist).index;
	for (auto& pid : PLAYLIST_NAMES)
	{
		if (pid.second.index > playlist_index) {
			if (!(pid.second.condition == PLCondition::EXTRAMODE && !extramodes || pid.second.condition == PLCondition::TOURNAMENT && !tournaments))
			{
				newPlaylist = pid.first;
				break;
			}
		}
	}
	if (newPlaylist == playlist) newPlaylist = -1;

	cvarManager->getCvar("ingamerank_playlist").setValue(newPlaylist);
}