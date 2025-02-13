#include "pch.h"
#include "IngameRank.h"

// Calculate rank based on mmr if the api returns unranked
static Vector2 rankFromMMR(float mmr, int playlist) {
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

void IngameRank::updateRankFor(UniqueIDWrapper uid, bool callUpdateDisplay) {
	if (!*pluginActive) return;
#ifdef _DEBUG
	if (!gameWrapper->IsInGame() && !gameWrapper->IsInOnlineGame() || gameWrapper->IsInFreeplay() || gameWrapper->IsInReplay()) return;
#else
	if (!gameWrapper->IsInOnlineGame()) return;
#endif

	MMRWrapper mmrWrapper = gameWrapper->GetMMRWrapper();

	//int currentMMR = 0;
	//int currentPlaylist = mmrWrapper.GetCurrentPlaylist();
	//if (mmrWrapper.IsSynced(uid, currentPlaylist)) { currentMMR = mmrWrapper.GetPlayerMMR(uid, currentPlaylist); }
	//mmrs[uid.GetIdString()] = currentMMR;
	PRanks pRanks = {};

	for (auto& pid : PLAYLIST_NAMES)
	{
		if (pid.first <= 0) continue;

		bool isSynced = mmrWrapper.IsSynced(uid, pid.first);
		bool isUnranked = false;
		SkillRank sr;
		int mmr;

		if (isSynced) {
			//sr = mmrWrapper.GetPlayerRank(uid, pid.first);
			//mmr = mmrWrapper.GetPlayerMMR(uid, pid.first);
			sr = SkillRank{
				0,
				0,
				0
			};
			mmr = 600;

			if (sr.Tier == 0) {
				isUnranked = true;
				if (mmr != 600 || sr.MatchesPlayed > 0) { // Apparently if someone never ever played a playlist 600 (gold 3) is returned for the mmr so we don't want that
					Vector2 calculated = rankFromMMR(mmr, pid.first);
					if (calculated.X > -1) {
						sr.Tier = calculated.X;
						sr.Division = calculated.Y;
					}
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

#ifdef _DEBUG
	LOG("IngameRank rankUpdate for " + uid.GetIdString());
	for (auto rank : pRanks.ranks) {
		LOG(std::to_string(rank.playlist_id) + " " + std::to_string(rank.isSynced) + " " + std::to_string(rank.isUnranked) + " " + std::to_string(rank.mmr) + " " + std::to_string(rank.skillrank.Tier) + " " + std::to_string(rank.skillrank.Division) + " " + std::to_string(rank.skillrank.MatchesPlayed));
	}
#endif

	// Only update display when called from the mmrNotifier in order to avoid recursive function calls
	if (callUpdateDisplay) updateDisplay();
}

IngameRank::DisplayRank IngameRank::displayRankOf(Pri pri, bool include_extras, bool include_tournaments, bool calculate_unranked) {
	//-------Decide Rank To Display------------
	SkillRank sr;
	int playlist = display_playlist;
	int temp_playlist = playlist;
	bool isUnranked = false;
	bool isSynced = true;
	std::string idString = pri.uid.GetIdString();
	if (player_ranks.find(idString) == player_ranks.end()) {
		updateRankFor(pri.uid); 
	}

	if (playlist == 0) { //"Best"
		int best_pl = 0;
		SkillRank best_rank = { -1, 0, 0 };
		for (auto& pid: PLAYLIST_NAMES)
		{
			if (pid.first <= 0) continue;
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
		if (best_rank.Tier < 0) isSynced = false;
	}
	else { //Any other
		PlaylistRank current = player_ranks[idString].ranks[PLAYLIST_NAMES.at(playlist).index];
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

	DisplayRank output;
	output.skillRank = sr;
	output.isUnranked = isUnranked;
	output.isSynced = isSynced;
	output.playlist = temp_playlist;
	return output;
}
