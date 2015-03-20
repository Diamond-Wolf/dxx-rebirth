/*
 * Portions of this file are copyright Rebirth contributors and licensed as
 * described in COPYING.txt.
 * Portions of this file are copyright Parallax Software and licensed
 * according to the Parallax license below.
 * See COPYING.txt for license details.

THE COMPUTER CODE CONTAINED HEREIN IS THE SOLE PROPERTY OF PARALLAX
SOFTWARE CORPORATION ("PARALLAX").  PARALLAX, IN DISTRIBUTING THE CODE TO
END-USERS, AND SUBJECT TO ALL OF THE TERMS AND CONDITIONS HEREIN, GRANTS A
ROYALTY-FREE, PERPETUAL LICENSE TO SUCH END-USERS FOR USE BY SUCH END-USERS
IN USING, DISPLAYING,  AND CREATING DERIVATIVE WORKS THEREOF, SO LONG AS
SUCH USE, DISPLAY OR CREATION IS FOR NON-COMMERCIAL, ROYALTY OR REVENUE
FREE PURPOSES.  IN NO EVENT SHALL THE END-USER USE THE COMPUTER CODE
CONTAINED HEREIN FOR REVENUE-BEARING PURPOSES.  THE END-USER UNDERSTANDS
AND AGREES TO THE TERMS HEREIN AND ACCEPTS THE SAME BY USE OF THIS FILE.
COPYRIGHT 1993-1999 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

/*
 *
 * contains routine(s) to read in the configuration file which contains
 * game configuration stuff like detail level, sound card, etc
 *
 */

#include <memory>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "config.h"
#include "pstypes.h"
#include "game.h"
#include "songs.h"
#include "kconfig.h"
#include "palette.h"
#include "args.h"
#include "player.h"
#include "mission.h"
#include "u_mem.h"
#include "physfsx.h"
#include "nvparse.h"

#include "compiler-make_unique.h"

struct Cfg GameCfg;

#define DigiVolumeStr "DigiVolume"
#define MusicVolumeStr "MusicVolume"
#define ReverseStereoStr "ReverseStereo"
#define OrigTrackOrderStr "OrigTrackOrder"
#define MusicTypeStr "MusicType"
#define CMLevelMusicPlayOrderStr "CMLevelMusicPlayOrder"
#define CMLevelMusicTrack0Str "CMLevelMusicTrack0"
#define CMLevelMusicTrack1Str "CMLevelMusicTrack1"
#define CMLevelMusicPathStr "CMLevelMusicPath"
#define CMMiscMusic0Str "CMMiscMusic0"
#define CMMiscMusic1Str "CMMiscMusic1"
#define CMMiscMusic2Str "CMMiscMusic2"
#define CMMiscMusic3Str "CMMiscMusic3"
#define CMMiscMusic4Str "CMMiscMusic4"
#define GammaLevelStr "GammaLevel"
#define LastPlayerStr "LastPlayer"
#define LastMissionStr "LastMission"
#define ResolutionXStr "ResolutionX"
#define ResolutionYStr "ResolutionY"
#define AspectXStr "AspectX"
#define AspectYStr "AspectY"
#define WindowModeStr "WindowMode"
#define TexFiltStr "TexFilt"
#if defined(DXX_BUILD_DESCENT_II)
#define MovieTexFiltStr "MovieTexFilt"
#define MovieSubtitlesStr "MovieSubtitles"
#endif
#define VSyncStr "VSync"
#define MultisampleStr "Multisample"
#define FPSIndicatorStr "FPSIndicator"
#define GrabinputStr "GrabInput"

int ReadConfigFile()
{
	// set defaults
	GameCfg.DigiVolume = 8;
	GameCfg.MusicVolume = 8;
	GameCfg.ReverseStereo = 0;
	GameCfg.OrigTrackOrder = 0;
#if defined(__APPLE__) && defined(__MACH__)
	GameCfg.MusicType = MUSIC_TYPE_REDBOOK;
#else
	GameCfg.MusicType = MUSIC_TYPE_BUILTIN;
#endif
	GameCfg.CMLevelMusicPlayOrder = MUSIC_CM_PLAYORDER_CONT;
	GameCfg.CMLevelMusicTrack[0] = -1;
	GameCfg.CMLevelMusicTrack[1] = -1;
	GameCfg.CMLevelMusicPath = {};
	GameCfg.CMMiscMusic = {};
#if defined(__APPLE__) && defined(__MACH__)
	GameCfg.OrigTrackOrder = 1;
#if defined(DXX_BUILD_DESCENT_I)
	GameCfg.CMLevelMusicPlayOrder = MUSIC_CM_PLAYORDER_LEVEL;
	GameCfg.CMLevelMusicPath = "descent.m3u";
	snprintf(GameCfg.CMMiscMusic[SONG_TITLE].data(), GameCfg.CMMiscMusic[SONG_TITLE].size(), "%s%s", PHYSFS_getUserDir(), "Music/iTunes/iTunes Music/Insanity/Descent/02 Primitive Rage.mp3");
	snprintf(GameCfg.CMMiscMusic[SONG_CREDITS].data(), GameCfg.CMMiscMusic[SONG_CREDITS].size(), "%s%s", PHYSFS_getUserDir(), "Music/iTunes/iTunes Music/Insanity/Descent/05 The Darkness Of Space.mp3");
#elif defined(DXX_BUILD_DESCENT_II)
	GameCfg.CMLevelMusicPath = "descent2.m3u";
	snprintf(GameCfg.CMMiscMusic[SONG_TITLE].data(), GameCfg.CMMiscMusic[SONG_TITLE].size(), "%s%s", PHYSFS_getUserDir(), "Music/iTunes/iTunes Music/Redbook Soundtrack/Descent II, Macintosh CD-ROM/02 Title.mp3");
	snprintf(GameCfg.CMMiscMusic[SONG_CREDITS].data(), GameCfg.CMMiscMusic[SONG_CREDITS].size(), "%s%s", PHYSFS_getUserDir(), "Music/iTunes/iTunes Music/Redbook Soundtrack/Descent II, Macintosh CD-ROM/03 Crawl.mp3");
#endif
	snprintf(GameCfg.CMMiscMusic[SONG_BRIEFING].data(), GameCfg.CMMiscMusic[SONG_BRIEFING].size(), "%s%s", PHYSFS_getUserDir(), "Music/iTunes/iTunes Music/Insanity/Descent/03 Outerlimits.mp3");
	snprintf(GameCfg.CMMiscMusic[SONG_ENDLEVEL].data(), GameCfg.CMMiscMusic[SONG_ENDLEVEL].size(), "%s%s", PHYSFS_getUserDir(), "Music/iTunes/iTunes Music/Insanity/Descent/04 Close Call.mp3");
	snprintf(GameCfg.CMMiscMusic[SONG_ENDGAME].data(), GameCfg.CMMiscMusic[SONG_ENDGAME].size(), "%s%s", PHYSFS_getUserDir(), "Music/iTunes/iTunes Music/Insanity/Descent/14 Insanity.mp3");
#endif
	GameCfg.GammaLevel = 0;
	GameCfg.LastPlayer.fill(0);
	memset(GameCfg.LastMission,0,MISSION_NAME_LEN+1);
	GameCfg.ResolutionX = 640;
	GameCfg.ResolutionY = 480;
	GameCfg.AspectX = 3;
	GameCfg.AspectY = 4;
	GameCfg.WindowMode = 0;
	GameCfg.TexFilt = 0;
#if defined(DXX_BUILD_DESCENT_II)
	GameCfg.MovieTexFilt = 0;
	GameCfg.MovieSubtitles = 0;
#endif
	GameCfg.VSync = 0;
	GameCfg.Multisample = 0;
	GameCfg.FPSIndicator = 0;
	GameCfg.Grabinput = 1;


	auto infile = PHYSFSX_openReadBuffered("descent.cfg");
	if (!infile)
	{
		return 1;
	}

	// to be fully safe, assume the whole cfg consists of one big line
	for (PHYSFSX_gets_line_t<0> line(PHYSFS_fileLength(infile) + 1); !PHYSFS_eof(infile);)
	{
		PHYSFSX_fgets(line, infile);
		const auto lb = line.begin();
		const auto eol = std::find(lb, line.end(), 0);
		if (eol == line.end())
			continue;
		auto eq = std::find(lb, eol, '=');
		if (eq == eol)
			continue;
		auto value = std::next(eq);
		if (cmp(lb, eq, DigiVolumeStr))
			convert_integer(GameCfg.DigiVolume, value);
		else if (cmp(lb, eq, MusicVolumeStr))
			convert_integer(GameCfg.MusicVolume, value);
		else if (cmp(lb, eq, ReverseStereoStr))
			convert_integer(GameCfg.ReverseStereo, value);
		else if (cmp(lb, eq, OrigTrackOrderStr))
			convert_integer(GameCfg.OrigTrackOrder, value);
		else if (cmp(lb, eq, MusicTypeStr))
			convert_integer(GameCfg.MusicType, value);
		else if (cmp(lb, eq, CMLevelMusicPlayOrderStr))
			convert_integer(GameCfg.CMLevelMusicPlayOrder, value);
		else if (cmp(lb, eq, CMLevelMusicTrack0Str))
			convert_integer(GameCfg.CMLevelMusicTrack[0], value);
		else if (cmp(lb, eq, CMLevelMusicTrack1Str))
			convert_integer(GameCfg.CMLevelMusicTrack[1], value);
		else if (cmp(lb, eq, CMLevelMusicPathStr))
			convert_string(GameCfg.CMLevelMusicPath, value, eol);
		else if (cmp(lb, eq, CMMiscMusic0Str))
			convert_string(GameCfg.CMMiscMusic[SONG_TITLE], value, eol);
		else if (cmp(lb, eq, CMMiscMusic1Str))
			convert_string(GameCfg.CMMiscMusic[SONG_BRIEFING], value, eol);
		else if (cmp(lb, eq, CMMiscMusic2Str))
			convert_string(GameCfg.CMMiscMusic[SONG_ENDLEVEL], value, eol);
		else if (cmp(lb, eq, CMMiscMusic3Str))
			convert_string(GameCfg.CMMiscMusic[SONG_ENDGAME], value, eol);
		else if (cmp(lb, eq, CMMiscMusic4Str))
			convert_string(GameCfg.CMMiscMusic[SONG_CREDITS], value, eol);
		else if (cmp(lb, eq, GammaLevelStr))
		{
			convert_integer(GameCfg.GammaLevel, value);
			gr_palette_set_gamma( GameCfg.GammaLevel );
		}
		else if (cmp(lb, eq, LastPlayerStr))
			GameCfg.LastPlayer.copy_lower(value, std::distance(value, eol));
		else if (cmp(lb, eq, LastMissionStr))
			convert_string(GameCfg.LastMission, value, eol);
		else if (cmp(lb, eq, ResolutionXStr))
			convert_integer(GameCfg.ResolutionX, value);
		else if (cmp(lb, eq, ResolutionYStr))
			convert_integer(GameCfg.ResolutionY, value);
		else if (cmp(lb, eq, AspectXStr))
			convert_integer(GameCfg.AspectX, value);
		else if (cmp(lb, eq, AspectYStr))
			convert_integer(GameCfg.AspectY, value);
		else if (cmp(lb, eq, WindowModeStr))
			convert_integer(GameCfg.WindowMode, value);
		else if (cmp(lb, eq, TexFiltStr))
			convert_integer(GameCfg.TexFilt, value);
#if defined(DXX_BUILD_DESCENT_II)
		else if (cmp(lb, eq, MovieTexFiltStr))
			convert_integer(GameCfg.MovieTexFilt, value);
		else if (cmp(lb, eq, MovieSubtitlesStr))
			convert_integer(GameCfg.MovieSubtitles, value);
#endif
		else if (cmp(lb, eq, VSyncStr))
			convert_integer(GameCfg.VSync, value);
		else if (cmp(lb, eq, MultisampleStr))
			convert_integer(GameCfg.Multisample, value);
		else if (cmp(lb, eq, FPSIndicatorStr))
			convert_integer(GameCfg.FPSIndicator, value);
		else if (cmp(lb, eq, GrabinputStr))
			convert_integer(GameCfg.Grabinput, value);
	}
	if ( GameCfg.DigiVolume > 8 ) GameCfg.DigiVolume = 8;
	if ( GameCfg.MusicVolume > 8 ) GameCfg.MusicVolume = 8;

	if (GameCfg.ResolutionX >= 320 && GameCfg.ResolutionY >= 200)
		Game_screen_mode = SM(GameCfg.ResolutionX,GameCfg.ResolutionY);

	return 0;
}

int WriteConfigFile()
{
	GameCfg.GammaLevel = gr_palette_get_gamma();

	auto infile = PHYSFSX_openWriteBuffered("descent.cfg");
	if (!infile)
	{
		return 1;
	}
	PHYSFSX_printf(infile, "%s=%d\n", DigiVolumeStr, GameCfg.DigiVolume);
	PHYSFSX_printf(infile, "%s=%d\n", MusicVolumeStr, GameCfg.MusicVolume);
	PHYSFSX_printf(infile, "%s=%d\n", ReverseStereoStr, GameCfg.ReverseStereo);
	PHYSFSX_printf(infile, "%s=%d\n", OrigTrackOrderStr, GameCfg.OrigTrackOrder);
	PHYSFSX_printf(infile, "%s=%d\n", MusicTypeStr, GameCfg.MusicType);
	PHYSFSX_printf(infile, "%s=%d\n", CMLevelMusicPlayOrderStr, GameCfg.CMLevelMusicPlayOrder);
	PHYSFSX_printf(infile, "%s=%d\n", CMLevelMusicTrack0Str, GameCfg.CMLevelMusicTrack[0]);
	PHYSFSX_printf(infile, "%s=%d\n", CMLevelMusicTrack1Str, GameCfg.CMLevelMusicTrack[1]);
	PHYSFSX_printf(infile, "%s=%s\n", CMLevelMusicPathStr, GameCfg.CMLevelMusicPath.data());
	PHYSFSX_printf(infile, "%s=%s\n", CMMiscMusic0Str, GameCfg.CMMiscMusic[SONG_TITLE].data());
	PHYSFSX_printf(infile, "%s=%s\n", CMMiscMusic1Str, GameCfg.CMMiscMusic[SONG_BRIEFING].data());
	PHYSFSX_printf(infile, "%s=%s\n", CMMiscMusic2Str, GameCfg.CMMiscMusic[SONG_ENDLEVEL].data());
	PHYSFSX_printf(infile, "%s=%s\n", CMMiscMusic3Str, GameCfg.CMMiscMusic[SONG_ENDGAME].data());
	PHYSFSX_printf(infile, "%s=%s\n", CMMiscMusic4Str, GameCfg.CMMiscMusic[SONG_CREDITS].data());
	PHYSFSX_printf(infile, "%s=%d\n", GammaLevelStr, GameCfg.GammaLevel);
	PHYSFSX_printf(infile, "%s=%s\n", LastPlayerStr, static_cast<const char *>(Players[Player_num].callsign));
	PHYSFSX_printf(infile, "%s=%s\n", LastMissionStr, GameCfg.LastMission);
	PHYSFSX_printf(infile, "%s=%i\n", ResolutionXStr, SM_W(Game_screen_mode));
	PHYSFSX_printf(infile, "%s=%i\n", ResolutionYStr, SM_H(Game_screen_mode));
	PHYSFSX_printf(infile, "%s=%i\n", AspectXStr, GameCfg.AspectX);
	PHYSFSX_printf(infile, "%s=%i\n", AspectYStr, GameCfg.AspectY);
	PHYSFSX_printf(infile, "%s=%i\n", WindowModeStr, GameCfg.WindowMode);
	PHYSFSX_printf(infile, "%s=%i\n", TexFiltStr, GameCfg.TexFilt);
#if defined(DXX_BUILD_DESCENT_II)
	PHYSFSX_printf(infile, "%s=%i\n", MovieTexFiltStr, GameCfg.MovieTexFilt);
	PHYSFSX_printf(infile, "%s=%i\n", MovieSubtitlesStr, GameCfg.MovieSubtitles);
#endif
	PHYSFSX_printf(infile, "%s=%i\n", VSyncStr, GameCfg.VSync);
	PHYSFSX_printf(infile, "%s=%i\n", MultisampleStr, GameCfg.Multisample);
	PHYSFSX_printf(infile, "%s=%i\n", FPSIndicatorStr, GameCfg.FPSIndicator);
	PHYSFSX_printf(infile, "%s=%i\n", GrabinputStr, GameCfg.Grabinput);
	return 0;
}
