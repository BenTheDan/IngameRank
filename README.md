# IngameRank
Bakkesmod plugin to show ranks in-game.

Plugin ID: [282](https://bakkesplugins.com/plugins/view/282)

If you want to make a plugin appending the scoreboard in any way, you can find instructions on how to use my calculations in your own project in [**ScoreboardMath.md**](https://github.com/BenTheDan/IngameRank/blob/28b9d66b47d963ffb8155cbafea625e574384346/ScoreboardMath.md).


# Contributing

I'm no longer working on this project actively, but if you'd like to make your own changes, I'll be happy to review them and update the plugin.

## Known Issues
**Start of match sorting**
At the beginning of the match ranks might be next to the wrong players. This is because we don't know exactly how Rocket League sorts players when they all have 0 points. The issue is corrected once players start earning points.

**Joining players mess up scoreboard**
If a player joins or leaves while the scoreboard is open their ranks might get assigned to the wrong team and mess up the alignment. This is corrected once the scoreboard is closed and reopened.

## Patch history
**v1.2**

 - *Fixed* players staying on the scoreboard after leaving messing up the alignment
 - *Fixed* the slightly shifted scoreboard in Season 14 update causing misalignment
 - *Added* "Current" to the playlist selection
 - *Removed* "Auto playlist" option
 - *Updated* textures of playlists, tiers and divisions; new textures by Dooey123
