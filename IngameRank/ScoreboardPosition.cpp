#include "pch.h"

#include "ScoreboardPosition.h"

SbPosInfo getSbPosInfo(Vector2 canvas_size, float uiScale, bool mutators, int numBlues, int numOranges, bool isReplaying, ScoreboardOffsets sbo) {
	//-----Black Magic------------
	float scale;
	if (float(canvas_size.X) / float(canvas_size.Y) > 1.5f) scale = 0.507f * canvas_size.Y / sbo.SCOREBOARD_HEIGHT;
	else scale = 0.615f * canvas_size.X / sbo.SCOREBOARD_WIDTH;

	Vector2F center = Vector2F{ float(canvas_size.X) / 2, float(canvas_size.Y) / 2 + +sbo.Y_OFFCENTER_OFFSET * scale * uiScale };
	float mutators_center = canvas_size.X - 1005.0f * scale * uiScale;
	if (mutators_center < center.X && mutators) center.X = mutators_center;
	center.X -= isReplaying * sbo.SKIP_TICK_SHIFT * scale * uiScale;
	int team_difference = numBlues - numOranges;
	center.Y += sbo.IMBALANCE_SHIFT * (team_difference - ((numBlues == 0) != (numOranges == 0)) * (team_difference >= 0 ? 1 : -1)) * scale * uiScale;

	float tier_X = -sbo.SCOREBOARD_LEFT - sbo.IMAGE_WIDTH * IMAGE_SCALE;
	float tier_Y_blue = -sbo.BLUE_BOTTOM + (6 * (4 - numBlues)) - sbo.BANNER_DISTANCE * numBlues + 9;
	float tier_Y_orange = sbo.ORANGE_TOP;
	int div_X = int(std::roundf(center.X + (-sbo.SCOREBOARD_LEFT - 100.0f * IMAGE_SCALE) * scale * uiScale));

	tier_Y_blue = center.Y + tier_Y_blue * scale * uiScale;
	tier_Y_orange = center.Y + tier_Y_orange * scale * uiScale;
	tier_X = center.X + tier_X * scale * uiScale;

	SbPosInfo output;
	output.scale = scale * uiScale;
	output.blueLeaderPos = { tier_X, tier_Y_blue };
	output.orangeLeaderPos = { tier_X, tier_Y_orange };
	output.playerSeparation = sbo.BANNER_DISTANCE * scale * uiScale;
	output.divisionPosX = div_X;

	// based on 100x100 img
	//------End Black Magic---------
	return output;
}

ScoreboardOffsets ParseScoreboardOffsets(const std::string& content) {
	ScoreboardOffsets offsets;

	std::istringstream stream(content);
	std::string line;

	while (std::getline(stream, line)) {
		// give it a haircut idk
		line.erase(0, line.find_first_not_of(" \t"));
		line.erase(line.find_last_not_of(" \t") + 1);

		// this make it go like....nuh uhhhhhhhh
		if (line.empty() || line.starts_with("//")) continue;

		auto delimiterPos = line.find(' ');
		if (delimiterPos == std::string::npos) continue;

		std::string key = line.substr(0, delimiterPos);
		std::string value = line.substr(delimiterPos + 1);

		// default value to make sure it always has a value...make it extra kwispy
		int intValue = 0;
		try {
			intValue = std::stoi(value);
		}
		catch (const std::invalid_argument&) {
			intValue = 0;
		}

		auto it = keyMap.find(key);
		OffsetKey keyEnum = (it != keyMap.end()) ? it->second : UNKNOWN;

		switch (keyEnum) {
		case SCOREBOARD_LEFT:    offsets.SCOREBOARD_LEFT = intValue; break;
		case BLUE_BOTTOM:        offsets.BLUE_BOTTOM = intValue; break;
		case ORANGE_TOP:         offsets.ORANGE_TOP = intValue; break;
		case BANNER_DISTANCE:    offsets.BANNER_DISTANCE = intValue; break;
		case IMAGE_WIDTH:        offsets.IMAGE_WIDTH = intValue; break;
		case IMAGE_HEIGHT:       offsets.IMAGE_HEIGHT = intValue; break;
		case CENTER_X:           offsets.CENTER_X = intValue; break;
		case CENTER_Y:           offsets.CENTER_Y = intValue; break;
		case SCOREBOARD_HEIGHT:  offsets.SCOREBOARD_HEIGHT = intValue; break;
		case SCOREBOARD_WIDTH:   offsets.SCOREBOARD_WIDTH = intValue; break;
		case IMBALANCE_SHIFT:    offsets.IMBALANCE_SHIFT = intValue; break;
		case MUTATOR_SIZE:       offsets.MUTATOR_SIZE = intValue; break;
		case SKIP_TICK_SHIFT:    offsets.SKIP_TICK_SHIFT = intValue; break;
		case Y_OFFCENTER_OFFSET: offsets.Y_OFFCENTER_OFFSET = intValue; break;
		default: break;
		}
	}

	return offsets;
}