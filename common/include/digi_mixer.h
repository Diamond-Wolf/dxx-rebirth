/*
 * This file is part of the DXX-Rebirth project <http://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
#ifndef __DIGI_MIXER__
#define __DIGI_MIXER__

#include "maths.h"

#ifdef __cplusplus

int digi_mixer_init();
void digi_mixer_close();
int digi_mixer_start_sound(short, fix, int, int, int, int, sound_object *);
void digi_mixer_set_channel_volume(int, int);
void digi_mixer_set_channel_pan(int, int);
void digi_mixer_stop_sound(int);
void digi_mixer_end_sound(int);
int digi_mixer_is_channel_playing(int);
void digi_mixer_reset();
void digi_mixer_stop_all_channels();
void digi_mixer_set_digi_volume(int);
void digi_mixer_debug();

#endif

#endif
