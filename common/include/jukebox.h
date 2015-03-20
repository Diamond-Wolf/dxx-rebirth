/*
 * This file is part of the DXX-Rebirth project <http://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
#ifndef __JUKEBOX_H__
#define __JUKEBOX_H__

#include "physfsx.h"

#ifdef __cplusplus

extern const array<file_extension_t, 6> jukebox_exts;

void jukebox_unload();
void jukebox_load();
int jukebox_play();
char *jukebox_current();
int jukebox_is_loaded();
int jukebox_is_playing();
int jukebox_numtracks();
void jukebox_list();

#endif

#endif
