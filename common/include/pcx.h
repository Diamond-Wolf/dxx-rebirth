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
COPYRIGHT 1993-1998 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/
/*
 *
 * Routines to read/write pcx images.
 *
 */


#ifndef _PCX_H
#define _PCX_H

#include "pstypes.h"

#ifdef __cplusplus

struct grs_bitmap;
struct palette_array_t;

#define PCX_ERROR_NONE          0
#define PCX_ERROR_OPENING       1
#define PCX_ERROR_NO_HEADER     2
#define PCX_ERROR_WRONG_VERSION 3
#define PCX_ERROR_READING       4
#define PCX_ERROR_NO_PALETTE    5
#define PCX_ERROR_WRITING       6
#define PCX_ERROR_MEMORY        7

#if defined(DXX_BUILD_DESCENT_I)
namespace dsx {
// Load bitmap for little-known 'baldguy' cheat.
extern int bald_guy_load( const char * filename, grs_bitmap * bmp,int bitmap_type ,palette_array_t &palette );
}
#endif

namespace dcx {

// Reads filename into bitmap bmp, and fills in palette.  If bmp->bm_data==NULL,
// then bmp->bm_data is allocated and the w,h are filled.
// If palette==NULL the palette isn't read in.  Returns error code.

int pcx_read_bitmap(const char * filename, grs_bitmap &bmp,int bitmap_type, palette_array_t &palette );

// Writes the bitmap bmp to filename, using palette. Returns error code.

extern int pcx_write_bitmap( const char * filename, grs_bitmap * bmp, palette_array_t &palette );

extern const char *pcx_errormsg(int error_number);

}

#endif

#endif
