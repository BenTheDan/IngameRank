#pragma once

#include "bakkesmod/wrappers/wrapperstructs.h"
#include <unordered_map>
#include <sstream>

//#define SCOREBOARD_LEFT 537
//#define BLUE_BOTTOM 77
//#define ORANGE_TOP 43
//#define BANNER_DISTANCE 57
//#define IMAGE_WIDTH 150
//#define IMAGE_HEIGHT 100
//#define CENTER_X 960
//#define CENTER_Y 540
//#define SCOREBOARD_HEIGHT 548
//#define SCOREBOARD_WIDTH 1033
//#define IMBALANCE_SHIFT 32
//#define MUTATOR_SIZE 478
//#define SKIP_TICK_SHIFT 67
//#define Y_OFFCENTER_OFFSET 32
#define IMAGE_SCALE 0.48f

// Scoreboard Position Info:
struct SbPosInfo {
	Vector2F blueLeaderPos;  // position of the first image for the top blue player
	Vector2F orangeLeaderPos;
	float playerSeparation;  // distance between profile pics on each team
	float scale;      // amount to scale each image
    int divisionPosX;
};

struct ScoreboardOffsets {
	int SCOREBOARD_LEFT = 537;
	int BLUE_BOTTOM = 67;
	int ORANGE_TOP = 43;
	int BANNER_DISTANCE = 57;
	int IMAGE_WIDTH = 150;
	int IMAGE_HEIGHT = 100;
	int CENTER_X = 960;
	int CENTER_Y = 540;
	int SCOREBOARD_HEIGHT = 548;
	int SCOREBOARD_WIDTH = 1033;
	int IMBALANCE_SHIFT = 32;
	int MUTATOR_SIZE = 478;
	int SKIP_TICK_SHIFT = 67;
	int Y_OFFCENTER_OFFSET = 32;
};

// Modified function signature to include platform
SbPosInfo getSbPosInfo(Vector2 canvas_size, float uiScale, bool mutators, int numBlues, int numOranges, bool isReplaying, ScoreboardOffsets sbo, OnlinePlatform platform);

ScoreboardOffsets ParseScoreboardOffsets(const std::string& content);

enum OffsetKey {
	SCOREBOARD_LEFT,
	BLUE_BOTTOM,
	ORANGE_TOP,
	BANNER_DISTANCE,
	IMAGE_WIDTH,
	IMAGE_HEIGHT,
	CENTER_X,
	CENTER_Y,
	SCOREBOARD_HEIGHT,
	SCOREBOARD_WIDTH,
	IMBALANCE_SHIFT,
	MUTATOR_SIZE,
	SKIP_TICK_SHIFT,
	Y_OFFCENTER_OFFSET,
	UNKNOWN
};

const std::unordered_map<std::string, OffsetKey> keyMap = {
	{"SCOREBOARD_LEFT", SCOREBOARD_LEFT},
	{"BLUE_BOTTOM", BLUE_BOTTOM},
	{"ORANGE_TOP", ORANGE_TOP},
	{"BANNER_DISTANCE", BANNER_DISTANCE},
	{"IMAGE_WIDTH", IMAGE_WIDTH},
	{"IMAGE_HEIGHT", IMAGE_HEIGHT},
	{"CENTER_X", CENTER_X},
	{"CENTER_Y", CENTER_Y},
	{"SCOREBOARD_HEIGHT", SCOREBOARD_HEIGHT},
	{"SCOREBOARD_WIDTH", SCOREBOARD_WIDTH},
	{"IMBALANCE_SHIFT", IMBALANCE_SHIFT},
	{"MUTATOR_SIZE", MUTATOR_SIZE},
	{"SKIP_TICK_SHIFT", SKIP_TICK_SHIFT},
	{"Y_OFFCENTER_OFFSET", Y_OFFCENTER_OFFSET},
};
