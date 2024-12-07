#pragma once

#include "bakkesmod/plugin/bakkesmodplugin.h"
#include "bakkesmod/plugin/pluginwindow.h"
#include "bakkesmod/plugin/PluginSettingsWindow.h"

#include "ScoreboardPosition.h"
#include <filesystem>
#include <chrono>
#include <algorithm>

//#define _DEBUG

#include "version.h"
constexpr auto plugin_version = stringify(VERSION_MAJOR) "." stringify(VERSION_MINOR) "." stringify(VERSION_PATCH);


class IngameRank: public BakkesMod::Plugin::BakkesModPlugin
{
public:
	std::shared_ptr<bool> pluginActive = std::make_shared<bool>(true);

	struct ScoreboardObj
	{
		unsigned char pad[0xB0];
		wchar_t* sorted_names;
	};

	struct SSParams {
		uintptr_t PRI_A;
		uintptr_t PRI_B;

		// if hooking post
		int32_t ReturnValue;
	};

	struct Pri {
		UniqueIDWrapper uid;
		int score{};
		unsigned char team{};
		bool isBot{};
		std::string name;
		OnlinePlatform platform;
		bool ghost_player;

		Pri() {}
		Pri(PriWrapper p) {
			if (!p) { return; }
			uid = p.GetUniqueIdWrapper();
			score = p.GetMatchScore();
			team = p.GetTeamNum2();
			isBot = p.GetbBot();
			name = p.GetPlayerName().ToString();
			platform = p.GetPlatform();
			ghost_player = team > 1;
		}
	};


public:
	void onLoad() override;
	void onUnload() override;

	void ComputeScoreboardInfo();
	void RecordScoreboardComparison(ActorWrapper gameEvent, void* params, std::string eventName);
	void openScoreboard(std::string eventName);
	void closeScoreboard(std::string eventName);
	void updateDisplay();
	void render(CanvasWrapper canvas);
	void renderPlaylist(CanvasWrapper canvas);
	void updateRankFor(UniqueIDWrapper uid, bool callUpdateDisplay = false);
	void griDestroyed(std::string eventName);
	void cyclePlaylist(std::vector<std::string> params);
	void getSortedIds(ActorWrapper caller);
	std::unique_ptr<MMRNotifierToken> mmrToken;

private:

	struct DisplayRank {
		SkillRank skillRank;
		int playlist;
		bool isUnranked;
		bool isSynced;
	};

	struct ComputedScoreboardInfo {
		std::vector<Pri>sortedPlayers;
		int bluePlayerCount{};
		int orangePlayerCount{};
	};

	struct image {
		std::shared_ptr<ImageWrapper> img;
		Vector2 position;
		float scale;
		LinearColor color;
	};

	struct PlaylistRank {
		SkillRank skillrank;
		int mmr;
		int playlist_id;
		bool isUnranked;
		bool isSynced;
	};
	struct PRanks {
		PlaylistRank ranks[9];
	};

	enum PLCondition {
		NONE = 0,
		EXTRAMODE = 1,
		TOURNAMENT = 2
	};
	struct Playlist {
		std::string name;
		int index;
		char condition;
	};

private:

	std::shared_ptr<ImageWrapper> tiers[24];
	std::shared_ptr<ImageWrapper> divisions[8];
	std::shared_ptr<ImageWrapper> playlists[8];

	int display_playlist = 0;

	//std::unordered_map<std::string, int> mmrs;
	std::unordered_map<std::string, PRanks> player_ranks;

	std::vector<image> toRender;
	bool isSBOpen = false;
	bool mutators = false;
	bool isReplaying = false;
	float uiScale = 1.0f;
	Vector2 canvas_size = Vector2{ 1920, 1080 }; // Default value for safety
	SbPosInfo sbPosInfo = { Vector2F{0, 0}, Vector2F{0, 0}, BANNER_DISTANCE, 0.48, 0 }; // Default value just to be safe
	ScoreboardOffsets ScoreboardPos;

	std::chrono::system_clock::time_point playlist_changed;
	bool show_rank_on = false;
	
	const std::map<int, Playlist> PLAYLIST_NAMES = {
		{-1, {"Current", -2, PLCondition::NONE}},
		{0, {"Best", -1, PLCondition::NONE}},
		{10, {"Solo Duel", 0, PLCondition::NONE}},
		{11, {"Doubles", 1, PLCondition::NONE}},
		//{12, {"Solo Standard", 2, PLCondition::NONE}},
		{13, {"Standard", 2, PLCondition::NONE}},
		{27, {"Hoops", 3, PLCondition::EXTRAMODE}},
		{28, {"Rumble", 4, PLCondition::EXTRAMODE}},
		{29, {"Dropshot", 5, PLCondition::EXTRAMODE}},
		{30, {"Snow Day", 6, PLCondition::EXTRAMODE}},
		{34, {"Tournaments", 7, PLCondition::TOURNAMENT}}
	};

	// Private match and Custom tournament
	const std::vector<int> EXCLUDED_PLAYLISTS = { 6, 22 };

	std::string sortedIds = "";
	// Members for scoreboard tracking logic.
	std::vector<std::pair<Pri, Pri>> comparisons;
	/**
	 * teamHistory records the last team that a player (represented by the
	 * string combining their name and uid) was seen on. This is necessary,
	 * because during ScoreboardSort any disconnected players will have a team
	 * number different from Blue or Orange, but we still need to know which team
	 * they show up on in the scoreboard display.
	 */
	std::unordered_map<std::string, int> teamHistory;
	ComputedScoreboardInfo computedInfo = { std::vector<Pri>(), 0, 0};  // Derived from comparisons and teamHistory.
	bool accumulateComparisons{};
#ifdef _DEBUG
	std::vector<std::string> sortednames;
#endif // _DEBUG


private:
	bool sortPris(Pri a, Pri b);
	// Decide which rank of a player to display and what that rank is
	DisplayRank displayRankOf(Pri pri, bool include_extras, bool include_tournaments, bool calculate_unranked);

	// Cache precomputed images of the ranks and their positions to be rendered
	void precomputeRankImages(
		Pri pri,
		DisplayRank displayRank,
		int oranges, int blues,
		int playlist,
		bool show_division, bool show_playlist, bool calculate_unranked
	);

	// Currently unused but might could be useful if initial scoreboard sorting is 
	// alphabetic or something similar
	// 
	// static bool compareName(int mmr1, std::string name1, int mmr2, std::string name2);
	// static std::string to_lower(std::string s);
};
