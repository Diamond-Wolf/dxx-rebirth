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
 * Header for movie.c
 *
 */

#pragma once

#ifdef __cplusplus
#include "d2x-rebirth/libmve/mvelib.h"

#define MOVIE_ABORT_ON  1
#define MOVIE_ABORT_OFF 0

#define MOVIE_NOT_PLAYED    0   // movie wasn't present
#define MOVIE_PLAYED_FULL   1   // movie was played all the way through
#define MOVIE_ABORTED       2   // movie started by was aborted

#ifdef OGL
#define MOVIE_WIDTH  (!GameArg.GfxSkipHiresMovie && grd_curscreen->get_screen_width() < 640 ? static_cast<uint16_t>(640) : grd_curscreen->get_screen_width())
#define MOVIE_HEIGHT (!GameArg.GfxSkipHiresMovie && grd_curscreen->get_screen_height() < 480 ? static_cast<uint16_t>(480) : grd_curscreen->get_screen_height())
#else
#define MOVIE_WIDTH  static_cast<uint16_t>(!GameArg.GfxSkipHiresMovie? 640 : 320)
#define MOVIE_HEIGHT static_cast<uint16_t>(!GameArg.GfxSkipHiresMovie? 480 : 200)
#endif

extern int PlayMovie(const char *subtitles, const char *filename, int allow_abort);
extern int PlayMovies(int num_files, const char *filename[], int graphmode, int allow_abort);
int InitRobotMovie(const char *filename, MVESTREAM_ptr_t &pMovie);
int RotateRobot(MVESTREAM_ptr_t &pMovie);
void DeInitRobotMovie(MVESTREAM_ptr_t &pMovie);

// find and initialize the movie libraries
void init_movies();

void init_extra_robot_movie(const char *filename);
void close_extra_robot_movie();

extern int MovieHires;      // specifies whether movies use low or high res

#endif
