// --------------------------------------------------------
// | "Ghost Players" fix based on LikeBook's solution     |
// | from PlatformDisplay (plugin id: 376) by SoulDaMeep  |
// --------------------------------------------------------

#include "pch.h"
#include "IngameRank.h"
#include <unordered_set>

std::string nameAndId(PriWrapper pri) {
    return pri.GetPlayerName().ToString() + "|" + pri.GetUniqueIdWrapper().GetIdString();
}

std::string nameAndId(const IngameRank::Pri& p) {
    return p.name + "|" + p.uid.GetIdString();
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

	std::sort(seenPrisVector.begin(), seenPrisVector.end(), [](const Pri& a, const Pri& b) { return a.score > b.score; });
	computedInfo = ComputedScoreboardInfo{ seenPrisVector, numBlues, numOranges };
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