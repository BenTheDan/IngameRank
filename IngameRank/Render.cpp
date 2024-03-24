#include "pch.h"
#include "IngameRank.h"

// Decide images to display and calculate their positions
void IngameRank::precomputeRankImages(
		Pri pri,
		DisplayRank displayRank, 
		int oranges, int blues, 
		int playlist, 
		bool show_division, bool show_playlist, bool calculate_unranked
	) {

	if (displayRank.skillRank.Tier < 0 || displayRank.skillRank.Tier > 23) {
		// If by any chance tier is wrong set it to unsynced so it doesn't crash when trying to get the images
		// though it really shouldn't be wrong, just a precaution
		displayRank.isSynced = false;
		displayRank.skillRank.Tier = 23;
		displayRank.skillRank.Division = -1;
	}

	float Y;
	bool do_show_division = show_division && displayRank.skillRank.Division > -1 && displayRank.skillRank.Tier < 22; // Wether to actually show the division
	if (pri.team == 0) {
		Y = sbPosInfo.blueLeaderPos.Y + sbPosInfo.playerSeparation * blues;//(computedInfo.bluePlayerCount - blues) + 9;
	}
	else {
		Y = sbPosInfo.orangeLeaderPos.Y + sbPosInfo.playerSeparation * (oranges);
	}
	float X = sbPosInfo.blueLeaderPos.X - 100.0f * do_show_division * sbPosInfo.scale * IMAGE_SCALE;

	LinearColor color = LinearColor{ (char)255, (char)255, (char)255, (char)255 };

	// When the player is a ghost player the PriWrapper should have a different team number than what we have saved
	// then the ghost players will have a dimmer rank shown for visual clarity
	if (pri.ghost_player) color = LinearColor{ (char)150, (char)150, (char)150, (char)255 };
	

	// Add tier image to be rendered
	toRender.push_back(image{ tiers[displayRank.skillRank.Tier], Vector2{int(std::roundf(X)), int(std::roundf(Y))}, sbPosInfo.scale * IMAGE_SCALE, color });
	// Add a little unranked icon if the tier was actually unranked, but calculated based on mmr
	if (displayRank.isUnranked && calculate_unranked) toRender.push_back(image{ tiers[0], Vector2{int(std::roundf(X)), int(std::roundf(Y))}, sbPosInfo.scale * IMAGE_SCALE * 0.5f, color });
	// Add division images to be rendered
	if (do_show_division) {
		std::shared_ptr<ImageWrapper> img = divisions[int((displayRank.skillRank.Tier - 1) / 3 + 1)];
		for (size_t i = 0; i < 4; i++)
		{
			toRender.push_back(image{ i <= displayRank.skillRank.Division ? img : divisions[0], {sbPosInfo.divisionPosX, int(std::roundf(Y + (3 - i) * 25 * sbPosInfo.scale * IMAGE_SCALE)) }, sbPosInfo.scale * IMAGE_SCALE, color });
		}
	}
	// Add playlist image to be rendered
	if (show_playlist && playlist == 0 && displayRank.isSynced) {
		if (PLAYLIST_NAMES.at(displayRank.playlist).index >= 0) // Make sure it's valid so again we don't crash
		toRender.push_back(image{ playlists[PLAYLIST_NAMES.at(displayRank.playlist).index], Vector2{int(std::roundf(X - 100.0f * sbPosInfo.scale * IMAGE_SCALE)), int(std::roundf(Y))}, sbPosInfo.scale * IMAGE_SCALE, color });
	}
}

void IngameRank::render(CanvasWrapper canvas) {
	// Images are "cached" in the toRender variable so we don't have to recalculate positions and images every frame just render the precalculated images
	
	for (image img : toRender)
	{
		canvas.SetColor(img.color);
		canvas.SetPosition(img.position);
		canvas.DrawTexture(img.img.get(), img.scale);

#ifdef _DEBUG
		//Draws a box around the rendered images
		int X1 = img.position.X;
		int X2 = X1 + int(img.img->GetSize().X * img.scale);
		int Y1 = img.position.Y;
		int Y2 = Y1 + int(img.img->GetSize().Y * img.scale);
		canvas.DrawLine(Vector2{ X1, Y1 }, Vector2{ X2, Y1 });
		canvas.DrawLine(Vector2{ X1, Y1 }, Vector2{ X1, Y2 });
		canvas.DrawLine(Vector2{ X1, Y2 }, Vector2{ X2, Y2 });
		canvas.DrawLine(Vector2{ X2, Y1 }, Vector2{ X2, Y2 });
#endif

	}
	canvas.SetColor((char)255, (char)255, (char)255, (char)255);

	if (!show_rank_on) {
		// If show player mmr is disabled in bakkesmod settings notify the player with a red message
		canvas.SetColor(255, 0, 0, 255);
		canvas.SetPosition(Vector2{ 0, 0 });
		canvas.DrawString("Turn on \"Ranked->Show player MMR on scoreboard\" and \"Ranked->Show MMR in casual playlists\" in the bakkesmod menu!", 1.5f, 1.5f);
	}

	renderPlaylist(canvas);
}

void IngameRank::renderPlaylist(CanvasWrapper canvas) {
	// Render the playlist selection text

	int playlist = display_playlist;
	std::string playlist_txt = "Playlist: " + PLAYLIST_NAMES.at(playlist).name;

	if (!!cvarManager)
	{
		CVarWrapper playlist_cvar = cvarManager->getCvar("ingamerank_playlist");
		if (!playlist_cvar.IsNull())
		{
			if (playlist_cvar.getIntValue() == -1) {
				playlist_txt = playlist_txt + " (Current)";
			}
		}
	}

	float opacity = 1.0f;
	if (!isSBOpen) {
		// Show for 4 seconds with fadeout if scoreboard is not open
		std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
		std::chrono::duration<double> elapsed = now - playlist_changed;
		double elapsed_seconds = elapsed.count();
		if (elapsed_seconds < 0.0) { playlist_changed = now; elapsed_seconds = 0.0; }
		else if (elapsed_seconds > 4.0) { gameWrapper->UnregisterDrawables(); return; }
		opacity = float(std::sqrt(4.0 - elapsed_seconds) / 2.0);
	}

	canvas.SetColor(LinearColor{ 255.0f, 255.0f, 255.0f, opacity * 255.0f });
	float posX = canvas_size.X - 75.0f * sbPosInfo.scale - 10;
	if (playlist > 0) {
		// Draw playlist image
		canvas.SetPosition(Vector2{ int(std::roundf(posX)), 10 });
		canvas.DrawTexture(playlists[PLAYLIST_NAMES.at(playlist).index].get(), 0.5f * sbPosInfo.scale);
	}

	// Draw text
	Vector2F string_size = canvas.GetStringSize(playlist_txt);
	canvas.SetPosition(Vector2{ int(std::roundf(posX - string_size.X * sbPosInfo.scale * 2.0f)), int(std::roundf(10 + 25.0f * sbPosInfo.scale - (string_size.Y / 2))) });
	canvas.DrawString(playlist_txt, sbPosInfo.scale * 2.0f, sbPosInfo.scale * 2.0f);
}