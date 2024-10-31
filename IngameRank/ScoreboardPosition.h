#pragma once

#include "bakkesmod/wrappers/wrapperstructs.h"

#define SCOREBOARD_LEFT 537
#define BLUE_BOTTOM 77
#define ORANGE_TOP 43
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
#define Y_OFFCENTER_OFFSET 32
#define IMAGE_SCALE 0.48f

// Scoreboard Position Info: 
struct SbPosInfo {
	Vector2F blueLeaderPos;  // position of the first image for the top blue player
	Vector2F orangeLeaderPos;
	float playerSeparation;  // distance between profile pics on each team
	float scale;      // amount to scale each image
    int divisionPosX;
};

static SbPosInfo getSbPosInfo(Vector2 canvas_size, float uiScale, bool mutators, int numBlues, int numOranges, bool isReplaying) {
	//-----Black Magic------------
	float scale;
	if (float(canvas_size.X) / float(canvas_size.Y) > 1.5f) scale = 0.507f * canvas_size.Y / SCOREBOARD_HEIGHT;
	else scale = 0.615f * canvas_size.X / SCOREBOARD_WIDTH;

	Vector2F center = Vector2F{ float(canvas_size.X) / 2, float(canvas_size.Y) / 2 + +Y_OFFCENTER_OFFSET * scale * uiScale };
	float mutators_center = canvas_size.X - 1005.0f * scale * uiScale;
	if (mutators_center < center.X && mutators) center.X = mutators_center;
	center.X -= isReplaying * SKIP_TICK_SHIFT * scale * uiScale;
	int team_difference = numBlues - numOranges;
	center.Y += IMBALANCE_SHIFT * (team_difference - ((numBlues == 0) != (numOranges == 0)) * (team_difference >= 0 ? 1 : -1)) * scale * uiScale;

	float tier_X = -SCOREBOARD_LEFT - IMAGE_WIDTH * IMAGE_SCALE;
	float tier_Y_blue = -BLUE_BOTTOM + (6 * (4 - numBlues)) - BANNER_DISTANCE * numBlues + 9; 
	float tier_Y_orange = ORANGE_TOP;
	int div_X = int(std::roundf(center.X + (-SCOREBOARD_LEFT - 100.0f * IMAGE_SCALE) * scale * uiScale));

    tier_Y_blue = center.Y + tier_Y_blue * scale * uiScale;
    tier_Y_orange = center.Y + tier_Y_orange * scale * uiScale;
    tier_X = center.X + tier_X * scale * uiScale;

	SbPosInfo output;
	output.scale = scale * uiScale;
	output.blueLeaderPos = { tier_X, tier_Y_blue };
	output.orangeLeaderPos = { tier_X, tier_Y_orange };
	output.playerSeparation = BANNER_DISTANCE * scale * uiScale;
	output.divisionPosX = div_X;

	// based on 100x100 img
	//------End Black Magic---------
	return output;
}
