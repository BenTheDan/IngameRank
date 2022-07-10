#include "IngameRank.h"


BAKKESMOD_PLUGIN(IngameRank, "IngameRank", "1.0", 0)

void IngameRank::onLoad() {
	//Register cvars
	cvarManager->registerCvar("ingamerank_enabled", "1", "Enable the plugin", false, true, 0.0f, true, 1.0f, true).bindTo(pluginActive);
	cvarManager->registerCvar("ingamerank_show_division", "1", "Show the division", false, true, 0.0f, true, 1.0f, true);
	cvarManager->registerCvar("ingamerank_show_playlist", "1", "Show the playlist", false, true, 0.0f, true, 1.0f, true);
	cvarManager->registerCvar("ingamerank_playlist", "0", "What playlist to show the ranks of", false, false, 0.0f, false, 30.0f, false);
	cvarManager->registerCvar("ingamerank_include_extramodes", "0", "Include extra modes in playlists", false, true, 0.0f, true, 1.0f, true);
	cvarManager->registerCvar("ingamerank_include_tournaments", "0", "Include tournaments in playlists", false, true, 0.0f, true, 1.0f, true);
	cvarManager->registerCvar("ingamerank_calculate_unranked", "1", "Calculate unranked ranks based on mmr", false, true, 0.0f, true, 1.0f, true);
	cvarManager->registerCvar("ingamerank_autoplaylist", "0", "Automatically set the playlist at the start of a match", false, true, 0.0f, true, 1.0f, true);
	cvarManager->registerNotifier("ingamerank_cycleplaylist", std::bind(&IngameRank::cyclePlaylist, this, std::placeholders::_1), "Cycles the playlist", PERMISSION_ALL);


	//AddOnValueChanged
	cvarManager->getCvar("ingamerank_playlist").addOnValueChanged([this](std::string old, CVarWrapper cvar) {
		int value = cvar.getIntValue();
		if (playlist_names.find(value) == playlist_names.end()) { 
			cvar.setValue(0);
			return;
		}
		else if (playlist_names[value].condition == PLCondition::EXTRAMODE
			&& !cvarManager->getCvar("ingamerank_include_extramodes").getBoolValue()
			|| playlist_names[value].condition == PLCondition::TOURNAMENT
			&& !cvarManager->getCvar("ingamerank_include_tournaments").getBoolValue())
		{
			cvar.setValue(0);
			return;
		}
		else if (*pluginActive) {
			if (isSBOpen) openScoreboard("");
			else {
				playlist_changed = std::chrono::system_clock::now();
				uiScale = gameWrapper->GetUIScale();
				canvas_size = gameWrapper->GetScreenSize();
				gameWrapper->RegisterDrawable(std::bind(&IngameRank::renderPlaylist, this, std::placeholders::_1));
			}
		}
	});
	cvarManager->getCvar("ingamerank_include_extramodes").addOnValueChanged([this](std::string, CVarWrapper cvar) {
		if (!cvar.getBoolValue()) {
			CVarWrapper playlist_cvar = cvarManager->getCvar("ingamerank_playlist");
			int playlist = playlist_cvar.getIntValue();
			if (playlist >= 27 && playlist <= 30) playlist_cvar.setValue(0);
		}
	});
	cvarManager->getCvar("ingamerank_include_tournaments").addOnValueChanged([this](std::string, CVarWrapper cvar) {
		if (!cvar.getBoolValue()) {
			CVarWrapper playlist_cvar = cvarManager->getCvar("ingamerank_playlist");
			if (playlist_cvar.getIntValue() == 34) playlist_cvar.setValue(0);
		}
	});


	//Hook events
	gameWrapper->HookEvent("Function TAGame.GFxData_GameEvent_TA.OnOpenScoreboard", std::bind(&IngameRank::openScoreboard, this, std::placeholders::_1));
	gameWrapper->HookEvent("Function TAGame.GFxData_GameEvent_TA.OnCloseScoreboard", std::bind(&IngameRank::closeScoreboard, this, std::placeholders::_1));
	gameWrapper->HookEvent("Function TAGame.PRI_TA.OnTeamChanged", std::bind(&IngameRank::teamUpdate, this, std::placeholders::_1));
	gameWrapper->HookEvent("Function TAGame.PRI_TA.GetBotName", std::bind(&IngameRank::teamUpdate, this, std::placeholders::_1));
	gameWrapper->HookEvent("Function TAGame.GameEvent_Soccar_TA.OnMatchEnded", [this](std::string eventName) {
		if (isSBOpen) closeScoreboard("");
	});
	gameWrapper->HookEvent("Function TAGame.GRI_TA.Destroyed", std::bind(&IngameRank::griDestroyed, this, std::placeholders::_1));
	gameWrapper->HookEvent("Function TAGame.GameEvent_Soccar_TA.OnAllTeamsCreated", [this](std::string eventName) {
		if (cvarManager->getCvar("ingamerank_autoplaylist").getBoolValue() && *pluginActive) {
			cvarManager->getCvar("ingamerank_playlist").setValue(gameWrapper->GetMMRWrapper().GetCurrentPlaylist());
		}
	});
	gameWrapper->HookEvent("Function ReplayDirector_TA.Playing.EndState", [this](std::string eventName) {
		isReplaying = false;
		if (isSBOpen) openScoreboard("");
	});
	gameWrapper->HookEvent("Function ReplayDirector_TA.Playing.BeginState", [this](std::string eventName) {
		isReplaying = true;
		if (isSBOpen) openScoreboard("");
	});
	mmrToken = gameWrapper->GetMMRWrapper().RegisterMMRNotifier(std::bind(&IngameRank::updateRankFor, this, std::placeholders::_1));
	
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
}

void IngameRank::openScoreboard(std::string eventName) {
	if (!*pluginActive) return;
#ifdef _DEBUG
	if (!gameWrapper->IsInGame() && !gameWrapper->IsInOnlineGame() || gameWrapper->IsInFreeplay() || gameWrapper->IsInReplay()) return;
	ServerWrapper sw = gameWrapper->IsInOnlineGame() ? gameWrapper->GetOnlineGame() : gameWrapper->GetGameEventAsServer();
#else
	if (!gameWrapper->IsInOnlineGame()) return;
	ServerWrapper sw = gameWrapper->GetOnlineGame();
#endif

	if (sw.IsNull()) return;
	MMRWrapper mmrWrapper = gameWrapper->GetMMRWrapper();
	if (sw.GetbMatchEnded()) return;

#ifndef _DEBUG
	if (std::find(excluded_playlists.begin(), excluded_playlists.end(), mmrWrapper.GetCurrentPlaylist()) != excluded_playlists.end()) return;
#endif

	if (!isSBOpen) updateScores();
	isSBOpen = true;
	show_rank_on = cvarManager->getCvar("ranked_showranks").getBoolValue() && cvarManager->getCvar("ranked_showranks_casual").getBoolValue();

	toRender.clear();


	//-----Black Magic------------
	canvas_size = gameWrapper->GetScreenSize();
	if (float(canvas_size.X) / float(canvas_size.Y) > 1.5f) scale = 0.507f * canvas_size.Y / SCOREBOARD_HEIGHT;
	else scale = 0.615f * canvas_size.X / SCOREBOARD_WIDTH;

	uiScale = gameWrapper->GetUIScale();
	mutators = mmrWrapper.GetCurrentPlaylist() == 34;
	Vector2F center = Vector2F{ float(canvas_size.X) / 2, float(canvas_size.Y) / 2 };
	float mutators_center = canvas_size.X - 1005.0f * scale * uiScale;
	if (mutators_center < center.X && mutators) center.X = mutators_center;
	center.X -= isReplaying * SKIP_TICK_SHIFT * scale * uiScale;
	int team_difference = num_blues - num_oranges;
	center.Y += IMBALANCE_SHIFT * (team_difference - ((num_blues == 0) != (num_oranges == 0)) * (team_difference >= 0 ? 1 : -1));

	image_scale = 0.48f;
	float tier_X = -SCOREBOARD_LEFT - IMAGE_WIDTH * image_scale;
	float tier_Y_blue = -BLUE_BOTTOM + (6 * (4 - num_blues)); 
	float tier_Y_orange = ORANGE_TOP;
	int div_X = int(std::roundf(center.X + (-SCOREBOARD_LEFT - 100.0f * image_scale) * scale * uiScale));
	image_scale *= scale * uiScale;
	//------End Black Magic---------


	//Get cvars
	int playlist = cvarManager->getCvar("ingamerank_playlist").getIntValue();
	bool show_division = cvarManager->getCvar("ingamerank_show_division").getBoolValue();
	bool calculate_unranked = cvarManager->getCvar("ingamerank_calculate_unranked").getBoolValue();
	bool include_extras = cvarManager->getCvar("ingamerank_include_extramodes").getBoolValue();
	bool include_tournaments = cvarManager->getCvar("ingamerank_include_tournaments").getBoolValue();
	bool show_playlist = cvarManager->getCvar("ingamerank_show_playlist").getBoolValue();

	int blues = -1;
	int oranges = -1;
	if (playlist_names.find(playlist) == playlist_names.end()) { //This should be handled in the OnValueChanged but better safe than sorry.
		playlist = 0;
		cvarManager->log("Incorrect value for cvar \"ingamerank_playlist\", defaulting to 0!");
		cvarManager->getCvar("ingamerank_playlist").setValue(0);
	}
	for (size_t i = 0; i < leaderboard.size(); i++)
	{
		if (leaderboard[i].team == 0) blues++;
		else if (leaderboard[i].team == 1) oranges++;
		if(leaderboard[i].isBot || leaderboard[i].team > 1) continue;

		//-------Decide Rank To Display------------
		SkillRank sr;
		int temp_playlist = playlist;
		bool isUnranked = false;
		bool isSynced = true;
		std::string idString = leaderboard[i].uid.GetIdString();
		if (player_ranks.find(idString) == player_ranks.end()) {
			updateRankFor(leaderboard[i].uid);
		}

		if (playlist == 0) { //"Best"
			int best_pl = 0;
			SkillRank best_rank = { -1, 0, 0 };
			for (auto& pid: playlist_names)
			{
				if (pid.second.condition == PLCondition::EXTRAMODE && !include_extras || pid.second.condition == PLCondition::TOURNAMENT && !include_tournaments) continue;
				PlaylistRank current = player_ranks[idString].ranks[pid.second.index];
				if (!current.isSynced) continue;

				if (current.isUnranked && !calculate_unranked) {
					if (best_rank.Tier < 0) {
						best_rank.Tier = 0;
						best_rank.Division = -1;
					}
				} else if (best_rank.Tier + (best_rank.Division + 1) * 0.1f < current.skillrank.Tier + (current.skillrank.Division + 1) * 0.1f) {
					best_rank = current.skillrank;
					best_pl = current.playlist_id;
					isUnranked = current.isUnranked;
				}
			}
			sr = best_rank;
			temp_playlist = best_pl;
			if (best_rank.Tier == -1) isSynced = false;
		}
		else { //Any other
			PlaylistRank current = player_ranks[idString].ranks[playlist_names[playlist].index];
			if (current.isUnranked && !calculate_unranked) {
				sr.Tier = 0;
				sr.Division = -1;
			} else sr = current.skillrank;
			isUnranked = current.isUnranked;
			isSynced = current.isSynced;
		}
		if (sr.Tier == 22) {
			sr.Division = -1;
		}
		if (sr.Tier == 0) {
			sr.Division = -1;
			isUnranked = false;
		}
		if (!isSynced) { 
			sr.Tier = 23;
			sr.Division = -1;
			isUnranked = false;
		}

		//--------Calculate Image Positions---------------------
		float Y;
		bool do_show_division = show_division && sr.Division > -1 && sr.Tier < 22;
		if (leaderboard[i].team == 0) { Y = tier_Y_blue - BANNER_DISTANCE * (num_blues - blues) + 9; }
		else { Y = tier_Y_orange + BANNER_DISTANCE * (oranges); }
		float X = tier_X - 100.0f * do_show_division * 0.48f;

		Y = center.Y + Y*scale*uiScale;
		X = center.X + X*scale*uiScale;

		toRender.push_back(image{ tiers[sr.Tier], Vector2{int(std::roundf(X)), int(std::roundf(Y))}, image_scale });
		if (isUnranked) toRender.push_back(image{ tiers[0], Vector2{int(std::roundf(X)), int(std::roundf(Y))}, image_scale * 0.5f});
		if (do_show_division) {
			std::shared_ptr<ImageWrapper> img = divisions[int((sr.Tier - 1) / 3 + 1)];
			for (size_t i = 0; i < 4; i++)
			{
				toRender.push_back(image{ i <= sr.Division ? img : divisions[0], {div_X, int(std::roundf(Y + (3 - i) * 25 * image_scale)) }, image_scale });
			}
		}
		if (show_playlist && playlist == 0 && isSynced) {
			toRender.push_back(image{ playlists[playlist_names[temp_playlist].index], Vector2{int(std::roundf(X - 100.0f * image_scale)), int(std::roundf(Y))}, image_scale });
		}
	}
	gameWrapper->UnregisterDrawables();
	gameWrapper->RegisterDrawable(std::bind(&IngameRank::render, this, std::placeholders::_1));
}

void IngameRank::closeScoreboard(std::string eventName) {
	if (isSBOpen) {
		gameWrapper->UnregisterDrawables();
		toRender.clear();
		isSBOpen = false;
	}
}

void IngameRank::griDestroyed(std::string eventName) {
	leaderboard.clear();
	player_ranks.clear();
	mmrs.clear();
	closeScoreboard("");
}

void IngameRank::teamUpdate(std::string eventName) {
	if (isSBOpen) {
		updateScores(true);
		openScoreboard("");
	}
}

void IngameRank::render(CanvasWrapper canvas) {
	canvas.SetColor((char)255, (char)255, (char)255, (char)255);
	for (size_t i = 0; i < toRender.size(); i++)
	{
		canvas.SetPosition(toRender[i].position);
		canvas.DrawTexture(toRender[i].img.get(), toRender[i].scale);

#ifdef _DEBUG
		//Draws a box around the rendered images
		int X1 = toRender[i].position.X;
		int X2 = X1 + int(toRender[i].img->GetSize().X * toRender[i].scale);
		int Y1 = toRender[i].position.Y;
		int Y2 = Y1 + int(toRender[i].img->GetSize().Y * toRender[i].scale);
		canvas.DrawLine(Vector2{ X1, Y1 }, Vector2{ X2, Y1 });
		canvas.DrawLine(Vector2{ X1, Y1 }, Vector2{ X1, Y2 });
		canvas.DrawLine(Vector2{ X1, Y2 }, Vector2{ X2, Y2 });
		canvas.DrawLine(Vector2{ X2, Y1 }, Vector2{ X2, Y2 });
#endif

	}
	if (!show_rank_on) {
		canvas.SetColor(255, 0, 0, 255);
		canvas.SetPosition(Vector2{ 0, 0 });
		canvas.DrawString("Turn on \"Ranked->Show player MMR on scoreboard\" and \"Ranked->Show MMR in casual playlists\" in the bakkesmod menu!", 1.5f, 1.5f);
	}
	renderPlaylist(canvas);
}

void IngameRank::renderPlaylist(CanvasWrapper canvas) {
	int playlist = cvarManager->getCvar("ingamerank_playlist").getIntValue();
	std::string playlist_txt = "Playlist: " + playlist_names[playlist].name;

	float opacity = 1.0f;
	if (!isSBOpen) {
		std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
		std::chrono::duration<double> elapsed = now - playlist_changed;
		double elapsed_seconds = elapsed.count();
		if (elapsed_seconds < 0.0) { playlist_changed = now; elapsed_seconds = 0.0; }
		else if (elapsed_seconds > 4.0) { gameWrapper->UnregisterDrawables(); return; }
		opacity = float(std::sqrt(4.0 - elapsed_seconds) / 2.0);
	}

	canvas.SetColor(LinearColor{255.0f, 255.0f, 255.0f, opacity*255.0f});
	float posX = canvas_size.X - 75.0f * scale * uiScale - 10;
	if (playlist != 0) {
		canvas.SetPosition(Vector2{ int(std::roundf(posX)), 10 });
		canvas.DrawTexture(playlists[playlist_names[playlist].index].get(), 0.5f * scale * uiScale);
	}

	Vector2F string_size = canvas.GetStringSize(playlist_txt);
	canvas.SetPosition(Vector2{ int(std::roundf(posX - string_size.X * scale * uiScale * 2.0f)), int(std::roundf(10 + 25.0f*scale*uiScale - (string_size.Y/2)))});
	canvas.DrawString(playlist_txt, scale * uiScale * 2.0f, scale * uiScale * 2.0f);
}

Vector2 IngameRank::rankFromMMR(float mmr, int playlist) {
	if (playlist == 10) playlist = 0;
	else if (playlist == 11) playlist = 1;
	else if (playlist == 13) playlist = 2;
	else return Vector2{ -1, -1 };
	float temp = mmr;
	int rank = 1;
	float subtrahend = 155.0f;
	if (playlist != 0) subtrahend = 175.0f;
	temp -= subtrahend;
	while (temp >= 0 && rank < 22) {
		subtrahend = 60.0f;
		rank += 1;
		if (playlist != 0) {
			if (rank >= 12) subtrahend = 80.0f;
			if (rank >= 15) subtrahend = 120.0f;
			if (rank >= 18) subtrahend = 140.0f;
			if (rank >= 20) subtrahend = 160.0f;
		}
		temp -= subtrahend;
	}
	if (rank == 22) return Vector2{ 22, 0 };
	temp += subtrahend;
	subtrahend += 15;
	int div = int(temp * (4 / subtrahend));
	return Vector2{ rank, div };
}

std::string IngameRank::to_lower(std::string s) {
	std::for_each(s.begin(), s.end(), [this](char& c) {
		c = std::tolower(c);
	});
	return s;
}

bool IngameRank::compareName(int mmr1, std::string name1, int mmr2, std::string name2) {
	if (mmr1 < mmr2) return true;
	else if (mmr1 == mmr2) {
		return to_lower(name1).compare(to_lower(name2)) == -1;
	}
	else return false;
}

void IngameRank::updateScores(bool keepOrder) {
	if (!*pluginActive) return;
#ifdef _DEBUG
	if (!gameWrapper->IsInGame() && !gameWrapper->IsInOnlineGame() || gameWrapper->IsInFreeplay() || gameWrapper->IsInReplay()) return;
	ServerWrapper sw = gameWrapper->IsInOnlineGame() ? gameWrapper->GetOnlineGame() : gameWrapper->GetGameEventAsServer();
#else
	if (!gameWrapper->IsInOnlineGame()) return;
	ServerWrapper sw = gameWrapper->GetOnlineGame();
#endif

	if (sw.IsNull()) return;
	MMRWrapper mmrWrapper = gameWrapper->GetMMRWrapper();
	if (mmrWrapper.GetCurrentPlaylist() == 6) return;

	int currentPlaylist = mmrWrapper.GetCurrentPlaylist();
	std::vector<std::string> currentUids;
	std::vector<std::string> currentNames;
	bool isThereNew = false;
	int blues = 0;
	int oranges = 0;
	ArrayWrapper<PriWrapper> players = sw.GetPRIs();

	for (size_t i = 0; i < players.Count(); i++)
	{
		PriWrapper priw = players.Get(i);
		if (priw.IsNull()) continue;

		UniqueIDWrapper uidw = priw.GetUniqueIdWrapper();
		int score = priw.GetMatchScore();
		int size = leaderboard.size();
		int index = size;
		int toRemove = -1;
		bool isNew = true;
		bool isBot = priw.GetbBot();

		int mmr = 0;
		if (mmrs.find(uidw.GetIdString()) != mmrs.end()) { mmr = mmrs[uidw.GetIdString()]; }
		else {
			if (mmrWrapper.IsSynced(uidw, currentPlaylist) && !isBot) mmr = mmrWrapper.GetPlayerMMR(uidw, currentPlaylist);
			mmrs[uidw.GetIdString()] = mmr;
		}

		std::string name = priw.GetPlayerName().ToString(); currentNames.push_back(name);
		currentUids.push_back(uidw.GetIdString());

		for (size_t j = 0; j < size; j++)
		{
			if (index == size) {
				if (leaderboard[j].score < score) {
					index = j;
				} else if (leaderboard[j].score == score) {
					if (compareName(mmr, name, mmrs[leaderboard[j].uid.GetIdString()], leaderboard[j].name) && !isBot) index = j;
				}
			}
			if (uidw.GetIdString() == leaderboard[j].uid.GetIdString() && name == leaderboard[j].name) {
				toRemove = j;
				isNew = false;
			}
		}

		if (isNew) { isThereNew = true; updateRankFor(uidw); }
		if (toRemove > -1 && !keepOrder) {
			leaderboard.erase(leaderboard.begin() + toRemove);
			if (index > toRemove) index--;
		}
		if (isNew || !keepOrder) {
			leaderboard.insert(leaderboard.begin() + index, { uidw, score, priw.GetTeamNum(), isBot, name });
		}
		else {
			leaderboard[toRemove].team = priw.GetTeamNum();
		}
		if (priw.GetTeamNum() == 0) blues++;
		else if (priw.GetTeamNum() == 1) oranges++;
	}
	num_blues = blues;
	num_oranges = oranges;

	//remove pris, that no longer exist
	if (isThereNew || players.Count() != leaderboard.size()) {
		for (size_t i = leaderboard.size(); i >= 1; i--)
		{
			bool remove = true;
			for (size_t j = 0; j < currentNames.size(); j++)
			{
				if (currentNames[j] == leaderboard[i - 1].name && currentUids[j] == leaderboard[i - 1].uid.GetIdString()) {
					remove = false;
					break;
				}
			}
			if (remove) { player_ranks.erase(leaderboard[i - 1].uid.GetIdString()); leaderboard.erase(leaderboard.begin() + i - 1); }
		}
	}
}

void IngameRank::cyclePlaylist(std::vector<std::string> params) {
	if (!*pluginActive) return;

	int playlist = cvarManager->getCvar("ingamerank_playlist").getIntValue();
	bool extramodes = cvarManager->getCvar("ingamerank_include_extramodes").getBoolValue();
	bool tournaments = cvarManager->getCvar("ingamerank_include_tournaments").getBoolValue();

	int newPlaylist = playlist;
	int playlist_index = playlist_names[playlist].index;
	for (auto& pid : playlist_names)
	{
		if (pid.second.index > playlist_index) {
			if (!(pid.second.condition == PLCondition::EXTRAMODE && !extramodes || pid.second.condition == PLCondition::TOURNAMENT && !tournaments))
			{
				newPlaylist = pid.first;
				break;
			}
		}
	}
	if (newPlaylist == playlist) newPlaylist = 0;

	cvarManager->getCvar("ingamerank_playlist").setValue(newPlaylist);
}

void IngameRank::updateRankFor(UniqueIDWrapper uid) {
	if (!*pluginActive) return;
#ifdef _DEBUG
	if (!gameWrapper->IsInGame() && !gameWrapper->IsInOnlineGame() || gameWrapper->IsInFreeplay() || gameWrapper->IsInReplay()) return;
#else
	if (!gameWrapper->IsInOnlineGame()) return;
#endif

	MMRWrapper mmrWrapper = gameWrapper->GetMMRWrapper();
	if (mmrWrapper.GetCurrentPlaylist() == 6) return;

	int currentMMR = 0;
	int currentPlaylist = mmrWrapper.GetCurrentPlaylist();
	if (mmrWrapper.IsSynced(uid, currentPlaylist)) { currentMMR = mmrWrapper.GetPlayerMMR(uid, currentPlaylist); }
	mmrs[uid.GetIdString()] = currentMMR;
	PRanks pRanks = {};

	for (auto& pid : playlist_names)
	{
		if (pid.first <= 0) continue;

		bool isSynced = mmrWrapper.IsSynced(uid, pid.first);
		bool isUnranked = false;
		SkillRank sr;
		int mmr;

		if (isSynced) {
			sr = mmrWrapper.GetPlayerRank(uid, pid.first);
			mmr = mmrWrapper.GetPlayerMMR(uid, pid.first);

			if (sr.Tier == 0) {
				isUnranked = true;
				Vector2 calculated = rankFromMMR(mmr, pid.first);
				if (calculated.X > -1) {
					sr.Tier = calculated.X;
					sr.Division = calculated.Y;
				}
			}
		}
		else {
			sr = { -1, -1, 0 };
			mmr = 0;
		}

		pRanks.ranks[pid.second.index] = {
			sr,
			mmr,
			pid.first,
			isUnranked,
			isSynced
		};
	}
	player_ranks[uid.GetIdString()] = pRanks;

	if (isSBOpen) openScoreboard("");
}