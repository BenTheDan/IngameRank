// --------------------------------------------------------
// | "Ghost Players" fix based on LikeBook's solution     |
// | from PlatformDisplay (plugin id: 376) by SoulDaMeep  |
// --------------------------------------------------------

#include "pch.h"
#include "IngameRank.h"
#include <unordered_set>

// Get the sorted ids list from the return value of "Function TAGame.GFxData_Scoreboard_TA.UpdateSortedPlayerIDs"
void IngameRank::getSortedIds(ActorWrapper caller) {

	auto* scoreboard = reinterpret_cast<ScoreboardObj*>(caller.memory_address);
	if (scoreboard->sorted_names == nullptr) return;
	auto sorted_names = std::wstring(scoreboard->sorted_names);

	std::string str;
	// Turn wstring into string so we can later use string.find
	std::transform(sorted_names.begin(), sorted_names.end(), std::back_inserter(str), [](wchar_t c) {
		return (char)c;
		});
	sortedIds = str;
}

std::string nameAndId(PriWrapper pri) {
    return pri.GetPlayerName().ToString() + "|" + pri.GetUniqueIdWrapper().GetIdString();
}

std::string nameAndId(const IngameRank::Pri& p) {
    return p.name + "|" + p.uid.GetIdString();
}

bool IngameRank::sortPris(Pri a, Pri b) {
	std::string id_a = a.uid.GetIdString();
	std::string id_b = b.uid.GetIdString();

	// Bots have their id in sortedIds in the format of Bot_Name
	if (a.isBot) id_a = "Bot_" + a.name;
	if (b.isBot) id_b = "Bot_" + b.name;

	// Use the sorted ids string to find out which pri is higher on the scoreboard
	// Needed when the players all have a score of 0
	size_t index_a = sortedIds.find(id_a);
	size_t index_b = sortedIds.find(id_b);
	if (index_a != std::string::npos && index_b != std::string::npos) {
		return index_a < index_b;
	}
	// Fallback mechanism if sorted ids doesn't contain the ids
	else {
		return a.score > b.score;
	}
}

void IngameRank::ComputeScoreboardInfo() {
	if (!accumulateComparisons) {
		return;
	}
	accumulateComparisons = false;

	auto hash = [](const Pri& p) { return std::hash<std::string>{}(nameAndId(p)); };
	auto keyEqual = [](const Pri& lhs, const Pri& rhs) { return nameAndId(lhs) == nameAndId(rhs); };
	std::unordered_set<Pri, decltype(hash), decltype(keyEqual)> seenPris{ 10, hash, keyEqual };
	std::unordered_set<std::string> ghostPlayerIds{};
	for (const auto& comparison : comparisons) {
		seenPris.insert(comparison.first);
		seenPris.insert(comparison.second);
		if (comparison.first.ghost_player) ghostPlayerIds.insert(nameAndId(comparison.first));
		if (comparison.second.ghost_player) ghostPlayerIds.insert(nameAndId(comparison.second));
	}
	std::vector<Pri> seenPrisVector;
	int numBlues{};
	int numOranges{};
	for (auto pri : seenPris) {
		pri.team = teamHistory[nameAndId(pri)];
		if (ghostPlayerIds.find(nameAndId(pri)) != ghostPlayerIds.end()) pri.ghost_player = true;
		if (pri.team == 0) {
			numBlues++;
		}
		else {
			numOranges++;
		}
		seenPrisVector.push_back(pri);
	}

	std::sort(seenPrisVector.begin(), seenPrisVector.end(), [this](const Pri& a, const Pri& b) { return sortPris(a, b); });
	computedInfo = ComputedScoreboardInfo{ seenPrisVector, numBlues, numOranges };
	updateDisplay();
}

void IngameRank::RecordScoreboardComparison(ActorWrapper gameEvent, void* params, std::string eventName) {
	if (!accumulateComparisons) {
		accumulateComparisons = true;
		comparisons.clear();
	}
	SSParams* p = static_cast<SSParams*>(params);
	if (!p) { return; }
	PriWrapper a(p->PRI_A);
	PriWrapper b(p->PRI_B);
	comparisons.push_back({ a, b });
	auto teamNumA = a.GetTeamNum2();
	if (teamNumA <= 1) {
		teamHistory[nameAndId(a)] = teamNumA;
	}

	auto teamNumB = b.GetTeamNum2();
	if (teamNumB <= 1) {
		teamHistory[nameAndId(b)] = teamNumB;
	}
}