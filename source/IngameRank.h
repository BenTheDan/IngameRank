#pragma once
#pragma comment(lib, "pluginsdk.lib")
#include "bakkesmod/plugin/bakkesmodplugin.h"
#include <filesystem>
#include <chrono>
#include <algorithm>

#define SCOREBOARD_LEFT 537
#define BLUE_BOTTOM 77
#define ORANGE_TOP 32
#define BANNER_DISTANCE 57
#define IMAGE_WIDTH 150
#define IMAGE_HEIGHT 100
#define CENTER_X 960
#define CENTER_Y 540
#define SCOREBOARD_HEIGHT 548
#define SCOREBOARD_WIDTH 1033
#define IMBALANCE_SHIFT 32
#define MUTATOR_SIZE 478
#define SKIP_TICK_SHIFT 67

//#define _DEBUG

class IngameRank : public BakkesMod::Plugin::BakkesModPlugin
{
public:
	std::shared_ptr<bool> pluginActive = std::make_shared<bool>(true);
public:
	virtual void onLoad();
	virtual void onUnload();

	void openScoreboard(std::string eventName);
	void closeScoreboard(std::string eventName);
	void render(CanvasWrapper canvas);
	void renderPlaylist(CanvasWrapper canvas);
	void teamUpdate(std::string eventName);
	void griDestroyed(std::string eventName);
	std::unique_ptr<MMRNotifierToken> mmrToken;
private:
	std::shared_ptr<ImageWrapper> tiers[24];
	std::shared_ptr<ImageWrapper> divisions[8];
	std::shared_ptr<ImageWrapper> playlists[8];

	float image_scale = 1.0f;
	struct image {
		std::shared_ptr<ImageWrapper> img;
		Vector2 position;
		float scale;
	};
	struct pri {
		UniqueIDWrapper uid;
		int score;
		unsigned char team;
		bool isBot;
		std::string name;
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

	std::vector<pri> leaderboard;
	std::unordered_map<std::string, int> mmrs;
	int num_blues = 0;
	int num_oranges = 0;

	std::unordered_map<std::string, PRanks> player_ranks;

	std::vector<image> toRender;
	Vector2 rankFromMMR(float mmr, int playlist);
	void updateScores(bool keepOrder = false);
	void cyclePlaylist(std::vector<std::string> params);
	void updateRankFor(UniqueIDWrapper uid);
	bool compareName(int mmr1, std::string name1, int mmr2, std::string name2);
	std::string to_lower(std::string s);
	bool isSBOpen = false;
	bool mutators = false;
	bool isReplaying = false;
	float uiScale = 1.0f;
	Vector2 canvas_size;
	float scale = 1.0f;

	bool playlist_visible = false;
	std::chrono::system_clock::time_point playlist_changed;
	bool show_rank_on = false;

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
	std::map<int, Playlist> playlist_names = {
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

	std::vector<int> excluded_playlists = { 6, 22 };
};

