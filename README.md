# IngameRank
Bakkesmod plugin to show ranks in-game.

Plugin ID: [282](https://bakkesplugins.com/plugins/view/282)

If you want to make a plugin appending the scoreboard in any way, you can find instructions on how to use my calculations in your own project in [**ScoreboardMath.md**]().

## Experimental Steam Offsets & Troubleshooting

Some users, particularly on Steam, might experience issues where player ranks on the scoreboard are misaligned. We've introduced an experimental configuration file with adjusted offsets for Steam.

**If you are experiencing rank display issues on Steam, you can try the following manual workaround:**

1.  **Locate Configuration Files:**
    *   Open your BakkesMod data folder. You can find this by opening the BakkesMod console (usually F6) and typing `bm_datadir`.
    *   Navigate to the `data/assets/IngameRank/` directory within that folder.

2.  **Activate Experimental Steam Offsets:**
    *   In the `data/assets/IngameRank/` directory, you should find two files:
        *   `ScoreboardLookUp.txt` (the default configuration)
        *   `ScoreboardLookUp_Steam.txt` (the experimental Steam configuration)
    *   **Backup the default file:** Rename `ScoreboardLookUp.txt` to `ScoreboardLookUp_Default.txt` (or any other backup name).
    *   **Activate Steam file:** Rename `ScoreboardLookUp_Steam.txt` to `ScoreboardLookUp.txt`.
    *   Restart Rocket League or reload the IngameRank plugin for the changes to take effect.

3.  **How to Revert to Default Offsets:**
    *   Navigate back to the `data/assets/IngameRank/` directory.
    *   **Deactivate Steam file:** Rename `ScoreboardLookUp.txt` (which is currently the Steam-specific one) back to `ScoreboardLookUp_Steam.txt`.
    *   **Restore default file:** Rename `ScoreboardLookUp_Default.txt` (or your chosen backup name) back to `ScoreboardLookUp.txt`.
    *   Restart Rocket League or reload the plugin.

**Feedback Welcome:**
These Steam offsets are experimental. If you try them, please let us know if they improve, worsen, or change the rank alignment on the scoreboard for you. Specific feedback on how they are misaligned (e.g., "ranks are too high/low/left/right") is very helpful.

This manual process is a temporary workaround. We hope to implement a more automated solution in the future.

# Contributing

I'm no longer working on this project actively, but if you'd like to make your own changes, I'll be happy to review them and update the plugin.

## Patch history

**v1.2.5**

 - *Improved* wording and visibility of warning to turn on "show mmr" in options

**v1.2.4**

 - *Fixed* players showing as "gold 3" if they never played the playlist

**v1.2.3**

 - Misalignments can now be corrected without updating the plugin

**v1.2.2**

 - *Fixed* slight misalignment caused by an update

**v1.2.1**

 - *Fixed* players not being sorted correctly at the start of the match
 - *Fixed* players joining while the scoreboard is open messing up the alignment

**v1.2**

 - *Fixed* players staying on the scoreboard after leaving messing up the alignment
 - *Fixed* the slightly shifted scoreboard in Season 14 update causing misalignment
 - *Added* "Current" to the playlist selection
 - *Removed* "Auto playlist" option
 - *Updated* textures of playlists, tiers and divisions; new textures by Dooey123
