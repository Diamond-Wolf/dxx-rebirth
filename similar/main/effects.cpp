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
 * Special effects, such as rotating fans, electrical walls, and
 * other cool animations.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "gr.h"
#include "inferno.h"
#include "game.h"
#include "vclip.h"
#include "effects.h"
#include "bm.h"
#include "u_mem.h"
#include "textures.h"
#include "physfs-serial.h"
#include "cntrlcen.h"
#include "segment.h"
#include "dxxerror.h"

#include "compiler-range_for.h"
#include "partial_range.h"

unsigned Num_effects;
array<eclip, MAX_EFFECTS> Effects;

void init_special_effects()
{
	range_for (eclip &ec, partial_range(Effects, Num_effects))
		ec.time_left = ec.vc.frame_time;
}

void reset_special_effects()
{
	range_for (eclip &ec, partial_range(Effects, Num_effects))
	{
		ec.segnum = segment_none;					//clear any active one-shots
		ec.flags &= ~(EF_STOPPED|EF_ONE_SHOT);		//restart any stopped effects

		//reset bitmap, which could have been changed by a crit_clip
		if (ec.changing_wall_texture != -1)
			Textures[ec.changing_wall_texture] = ec.vc.frames[ec.frame_count];

		if (ec.changing_object_texture != -1)
			ObjBitmaps[ec.changing_object_texture] = ec.vc.frames[ec.frame_count];

	}
}

void do_special_effects()
{
	range_for (eclip &ec, partial_range(Effects, Num_effects))
	{
		if ((ec.changing_wall_texture == -1) && (ec.changing_object_texture==-1) )
			continue;

		if (ec.flags & EF_STOPPED)
			continue;

		ec.time_left -= FrameTime;

		while (ec.time_left < 0) {

			ec.time_left += ec.vc.frame_time;
			
			ec.frame_count++;
			if (ec.frame_count >= ec.vc.num_frames) {
				if (ec.flags & EF_ONE_SHOT) {
					Assert(ec.segnum!=segment_none);
					Assert(ec.sidenum>=0 && ec.sidenum<6);
					Assert(ec.dest_bm_num!=0 && Segments[ec.segnum].sides[ec.sidenum].tmap_num2!=0);
					Segments[ec.segnum].sides[ec.sidenum].tmap_num2 = ec.dest_bm_num | (Segments[ec.segnum].sides[ec.sidenum].tmap_num2&0xc000);		//replace with destoyed
					ec.flags &= ~EF_ONE_SHOT;
					ec.segnum = segment_none;		//done with this
				}

				ec.frame_count = 0;
			}
		}

		if (ec.flags & EF_CRITICAL)
			continue;

		if (ec.crit_clip!=-1 && Control_center_destroyed) {
			int n = ec.crit_clip;

			if (ec.changing_wall_texture != -1)
				Textures[ec.changing_wall_texture] = Effects[n].vc.frames[Effects[n].frame_count];

			if (ec.changing_object_texture != -1)
				ObjBitmaps[ec.changing_object_texture] = Effects[n].vc.frames[Effects[n].frame_count];

		}
		else	{
			if (ec.changing_wall_texture != -1)
				Textures[ec.changing_wall_texture] = ec.vc.frames[ec.frame_count];
	
			if (ec.changing_object_texture != -1)
				ObjBitmaps[ec.changing_object_texture] = ec.vc.frames[ec.frame_count];
		}

	}
}

void restore_effect_bitmap_icons()
{
	range_for (eclip &ec, partial_range(Effects, Num_effects))
	{
		if (! (ec.flags & EF_CRITICAL))	{
			if (ec.changing_wall_texture != -1)
				Textures[ec.changing_wall_texture] = ec.vc.frames[0];
	
			if (ec.changing_object_texture != -1)
				ObjBitmaps[ec.changing_object_texture] = ec.vc.frames[0];
		}
	}
}

//stop an effect from animating.  Show first frame.
void stop_effect(int effect_num)
{
	eclip *ec = &Effects[effect_num];
	
	//Assert(ec->bm_ptr != -1);

	ec->flags |= EF_STOPPED;

	ec->frame_count = 0;
	//*ec->bm_ptr = &GameBitmaps[ec->vc.frames[0].index];

	if (ec->changing_wall_texture != -1)
		Textures[ec->changing_wall_texture] = ec->vc.frames[0];
	
	if (ec->changing_object_texture != -1)
		ObjBitmaps[ec->changing_object_texture] = ec->vc.frames[0];

}

//restart a stopped effect
void restart_effect(int effect_num)
{
	Effects[effect_num].flags &= ~EF_STOPPED;

	//Assert(Effects[effect_num].bm_ptr != -1);
}

DEFINE_VCLIP_SERIAL_UDT();
DEFINE_SERIAL_UDT_TO_MESSAGE(eclip, ec, (ec.vc, ec.time_left, ec.frame_count, ec.changing_wall_texture, ec.changing_object_texture, ec.flags, ec.crit_clip, ec.dest_bm_num, ec.dest_vclip, ec.dest_eclip, ec.dest_size, ec.sound_num, ec.segnum, serial::pad<2>(), ec.sidenum));
ASSERT_SERIAL_UDT_MESSAGE_SIZE(eclip, 130);

/*
 * reads n eclip structs from a PHYSFS_file
 */
void eclip_read(PHYSFS_file *fp, eclip &ec)
{
	PHYSFSX_serialize_read(fp, ec);
}

#if 0
void eclip_write(PHYSFS_file *fp, const eclip &ec)
{
	PHYSFSX_serialize_write(fp, ec);
}
#endif
