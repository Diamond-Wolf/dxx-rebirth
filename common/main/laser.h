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
 * Definitions for the laser code.
 *
 */

#pragma once

#include "maths.h"
#include "vecmat.h"

#ifdef __cplusplus
#include "segnum.h"
#include "fwd-object.h"
#include "weapon_id.h"

// These are new defines for the value of 'flags' passed to do_laser_firing.
// The purpose is to collect other flags like QUAD_LASER and Spreadfire_toggle
// into a single 8-bit quantity so it can be easily used in network mode.

#define LASER_QUAD                  1
#define LASER_SPREADFIRE_TOGGLED    2

#define MAX_LASER_LEVEL         LASER_LEVEL_4   // Note, laser levels are numbered from 0.

#if defined(DXX_BUILD_DESCENT_I)
#define DXX_MAXIMUM_LASER_LEVEL	LASER_LEVEL_4
#elif defined(DXX_BUILD_DESCENT_II)
#define DXX_MAXIMUM_LASER_LEVEL	MAX_SUPER_LASER_LEVEL
#endif

#define MAX_LASER_BITMAPS   6

// For muzzle firing casting light.
#define MUZZLE_QUEUE_MAX    8

// Constants & functions governing homing missile behavior.
extern void calc_d_homer_tick();
#define HOMING_MAX_TRACKABLE_DOT        (3*F1_0/4) // was (7*F1_0/8) in original Descent 2
#define HOMING_MIN_TRACKABLE_DOT        (3*(F1_0 - HOMING_MAX_TRACKABLE_DOT)/4 + HOMING_MAX_TRACKABLE_DOT)
#define HOMING_FLY_STRAIGHT_TIME        (F1_0/8)
#define HOMING_TURN_TIME                (F1_0/30)

#if defined(DXX_BUILD_DESCENT_I) || defined(DXX_BUILD_DESCENT_II)
void Laser_render(vobjptr_t obj);
objptridx_t Laser_player_fire(vobjptridx_t obj, weapon_id_type laser_type, int gun_num, int make_sound, vms_vector shot_orientation);
void Laser_do_weapon_sequence(vobjptridx_t obj);
void Flare_create(vobjptridx_t obj);
bool laser_are_related(vcobjptridx_t o1, vcobjptridx_t o2);

extern int do_laser_firing_player(void);
extern void do_missile_firing(int drop_bomb);
extern void net_missile_firing(int player, int weapon, int flags);
extern objnum_t Network_laser_track;

objptridx_t Laser_create_new(const vms_vector &direction, const vms_vector &position, segnum_t segnum, vobjptridx_t parent, weapon_id_type type, int make_sound);

// Fires a laser-type weapon (a Primary weapon)
// Fires from object objnum, weapon type weapon_id.
// Assumes that it is firing from a player object, so it knows which
// gun to fire from.
// Returns the number of shots actually fired, which will typically be
// 1, but could be higher for low frame rates when rapidfire weapons,
// such as vulcan or plasma are fired.
int do_laser_firing(vobjptridx_t objnum, int weapon_id, int level, int flags, int nfires, vms_vector shot_orientation);

// Easier to call than Laser_create_new because it determines the
// segment containing the firing point and deals with it being stuck
// in an object or through a wall.
// Fires a laser of type "weapon_type" from an object (parent) in the
// direction "direction" from the position "position"
// Returns object number of laser fired or -1 if not possible to fire
// laser.
objptridx_t Laser_create_new_easy(const vms_vector &direction, const vms_vector &position, vobjptridx_t parent, weapon_id_type weapon_type, int make_sound);

#if defined(DXX_BUILD_DESCENT_II)
// give up control of the guided missile
void release_guided_missile(int player_num);

// Omega cannon stuff.
#define MAX_OMEGA_CHARGE    (F1_0)  //  Maximum charge level for omega cannonw
extern fix Omega_charge;
extern int Smartmines_dropped;
// NOTE: OMEGA_CHARGE_SCALE moved to laser.c to avoid long rebuilds if changed
int ok_to_do_omega_damage(vcobjptr_t weapon);
void create_robot_smart_children(vobjptridx_t objp, uint_fast32_t count);
#endif

void create_weapon_smart_children(vobjptridx_t objp);
int object_to_object_visibility(vcobjptridx_t obj1, vcobjptr_t obj2, int trans_type);
#endif

extern int Missile_gun;
extern int Proximity_dropped;

struct muzzle_info
{
	fix64       create_time;
	segnum_t       segnum;
	vms_vector  pos;
};

extern array<muzzle_info, MUZZLE_QUEUE_MAX> Muzzle_data;

// Omega cannon stuff.
#define MAX_OMEGA_CHARGE    (F1_0)  //  Maximum charge level for omega cannonw
void omega_charge_frame(void);

#if defined(DXX_BUILD_DESCENT_I) || defined(DXX_BUILD_DESCENT_II)
static inline int is_proximity_bomb_or_smart_mine(weapon_id_type id)
{
#if defined(DXX_BUILD_DESCENT_II)
	if (id == weapon_id_type::SUPERPROX_ID)
		return 1;
#endif
	return id == weapon_id_type::PROXIMITY_ID;
}

static inline int is_proximity_bomb_or_smart_mine_or_placed_mine(weapon_id_type id)
{
#if defined(DXX_BUILD_DESCENT_II)
	if (id == weapon_id_type::SUPERPROX_ID || id == weapon_id_type::PMINE_ID)
		return 1;
#endif
	return id == weapon_id_type::PROXIMITY_ID;
}
#endif

#endif
