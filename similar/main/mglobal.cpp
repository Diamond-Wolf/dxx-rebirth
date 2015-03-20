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
 * Global variables for main directory
 *
 */

#include "maths.h"
#include "vecmat.h"
#include "inferno.h"
#include "segment.h"
#include "object.h"
#include "bm.h"
#include "3d.h"
#include "game.h"
#include "textures.h"

// Global array of vertices, common to one mine.
array<vertex, MAX_VERTICES> Vertices;
array<g3s_point, MAX_VERTICES> Segment_points;

fix FrameTime = 0x1000;	// Time since last frame, in seconds
fix64 GameTime64 = 0;			//	Time in game, in seconds

int d_tick_count = 0; // increments every 50ms
int d_tick_step = 0;  // true once every 50ms

//	This is the global mine which create_new_mine returns.
segment_array_t	Segments;
//lsegment	Lsegments[MAX_SEGMENTS];

// Number of vertices in current mine (ie, Vertices, pointed to by Vp)
unsigned Num_vertices;
unsigned Num_segments;

unsigned Highest_vertex_index;

//	Translate table to get opposite side of a face on a segment.
const array<char, MAX_SIDES_PER_SEGMENT> Side_opposite{{
	WRIGHT, WBOTTOM, WLEFT, WTOP, WFRONT, WBACK
}};

#define TOLOWER(c) ((((c)>='A') && ((c)<='Z'))?((c)+('a'-'A')):(c))

#ifdef PASSWORD
#define encrypt(a,b,c,d)	a ^ TOLOWER((((int) PASSWORD)>>24)&255), \
									b ^ TOLOWER((((int) PASSWORD)>>16)&255), \
									c ^ TOLOWER((((int) PASSWORD)>>8)&255), \
									d ^ TOLOWER((((int) PASSWORD))&255)
#else
#define encrypt(a,b,c,d) a,b,c,d
#endif

const array<array<sbyte, 4>, MAX_SIDES_PER_SEGMENT> Side_to_verts{{
			{ encrypt(7,6,2,3) },			// left
			{ encrypt(0,4,7,3) },			// top
			{ encrypt(0,1,5,4) },			// right
			{ encrypt(2,6,5,1) },			// bottom
			{ encrypt(4,5,6,7) },			// back
			{ encrypt(3,2,1,0) },			// front
}};

//	Note, this MUST be the same as Side_to_verts, it is an int for speed reasons.
const array<array<int, 4>, MAX_SIDES_PER_SEGMENT>  Side_to_verts_int{{
			{ encrypt(7,6,2,3) },			// left
			{ encrypt(0,4,7,3) },			// top
			{ encrypt(0,1,5,4) },			// right
			{ encrypt(2,6,5,1) },			// bottom
			{ encrypt(4,5,6,7) },			// back
			{ encrypt(3,2,1,0) },			// front
}};

// Texture map stuff

fix64	Next_laser_fire_time;			//	Time at which player can next fire his selected laser.
fix64	Next_missile_fire_time;			//	Time at which player can next fire his selected missile.
//--unused-- fix	Laser_delay_time = F1_0/6;		//	Delay between laser fires.

#define DEFAULT_DIFFICULTY		1

int	Difficulty_level=DEFAULT_DIFFICULTY;	//	Difficulty level in 0..NDL-1, 0 = easiest, NDL-1 = hardest

