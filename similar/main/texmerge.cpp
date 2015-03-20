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
 * Routines to cache merged textures.
 *
 */


#include "gr.h"
#include "dxxerror.h"
#include "game.h"
#include "textures.h"
#include "rle.h"
#include "timer.h"
#include "piggy.h"
#include "texmerge.h"
#include "piggy.h"

#include "compiler-range_for.h"
#include "partial_range.h"

#ifdef OGL
#include "ogl_init.h"
#define MAX_NUM_CACHE_BITMAPS 200
#else
#define MAX_NUM_CACHE_BITMAPS 50
#endif

//static grs_bitmap * cache_bitmaps[MAX_NUM_CACHE_BITMAPS];                     

struct TEXTURE_CACHE {
	grs_bitmap_ptr bitmap;
	grs_bitmap * bottom_bmp;
	grs_bitmap * top_bmp;
	int 		orient;
	fix64		last_time_used;
};

static array<TEXTURE_CACHE, MAX_NUM_CACHE_BITMAPS> Cache;

static unsigned num_cache_entries;

static int cache_hits = 0;
static int cache_misses = 0;

static void merge_textures_super_xparent(int type, const grs_bitmap &bottom_bmp, const grs_bitmap &top_bmp,
											 ubyte *dest_data);
static void merge_textures_new(int type, const grs_bitmap &bottom_bmp, const grs_bitmap &top_bmp,
								ubyte *dest_data);

//----------------------------------------------------------------------

int texmerge_init(int num_cached_textures)
{
	if ( num_cached_textures <= MAX_NUM_CACHE_BITMAPS )
		num_cache_entries = num_cached_textures;
	else
		num_cache_entries = MAX_NUM_CACHE_BITMAPS;
	
	range_for (auto &i, partial_range(Cache, num_cache_entries))
	{
		i.bitmap = NULL;
		i.last_time_used = -1;
		i.top_bmp = NULL;
		i.bottom_bmp = NULL;
		i.orient = -1;
	}

	return 1;
}

void texmerge_flush()
{
	range_for (auto &i, partial_range(Cache, num_cache_entries))
	{
		i.last_time_used = -1;
		i.top_bmp = NULL;
		i.bottom_bmp = NULL;
		i.orient = -1;
	}
}


//-------------------------------------------------------------------------
void texmerge_close()
{
	range_for (auto &i, partial_range(Cache, num_cache_entries))
	{
		i.bitmap.reset();
	}
}

//--unused-- int info_printed = 0;

grs_bitmap &texmerge_get_cached_bitmap(unsigned tmap_bottom, unsigned tmap_top)
{
	grs_bitmap *bitmap_top, *bitmap_bottom;
	int orient;
	int lowest_time_used;

	bitmap_top = &GameBitmaps[Textures[tmap_top&0x3FFF].index];
	bitmap_bottom = &GameBitmaps[Textures[tmap_bottom].index];
	
	orient = ((tmap_top&0xC000)>>14) & 3;

	lowest_time_used = Cache[0].last_time_used;
	auto least_recently_used = &Cache.front();
	range_for (auto &i, partial_range(Cache, num_cache_entries))
	{
		if ( (i.last_time_used > -1) && (i.top_bmp==bitmap_top) && (i.bottom_bmp==bitmap_bottom) && (i.orient==orient ))	{
			cache_hits++;
			i.last_time_used = timer_query();
			return *i.bitmap.get();
		}	
		if ( i.last_time_used < lowest_time_used )	{
			lowest_time_used = i.last_time_used;
			least_recently_used = &i;
		}
	}

	//---- Page out the LRU bitmap;
	cache_misses++;

	// Make sure the bitmaps are paged in...
	piggy_page_flushed = 0;

	PIGGY_PAGE_IN(Textures[tmap_top&0x3FFF]);
	PIGGY_PAGE_IN(Textures[tmap_bottom]);
	if (piggy_page_flushed)	{
		// If cache got flushed, re-read 'em.
		piggy_page_flushed = 0;
		PIGGY_PAGE_IN(Textures[tmap_top&0x3FFF]);
		PIGGY_PAGE_IN(Textures[tmap_bottom]);
	}
	Assert( piggy_page_flushed == 0 );
	if (bitmap_bottom->bm_w != bitmap_bottom->bm_h || bitmap_top->bm_w != bitmap_top->bm_h)
		Error("Texture width != texture height!\n");
	if (bitmap_bottom->bm_w != bitmap_top->bm_w || bitmap_bottom->bm_h != bitmap_top->bm_h)
		Error("Top and Bottom textures have different size!\n");

	least_recently_used->bitmap = gr_create_bitmap(bitmap_bottom->bm_w,  bitmap_bottom->bm_h);
#ifdef OGL
	ogl_freebmtexture(*least_recently_used->bitmap.get());
#endif

	if (bitmap_top->bm_flags & BM_FLAG_SUPER_TRANSPARENT)	{
		merge_textures_super_xparent( orient, *bitmap_bottom, *bitmap_top, least_recently_used->bitmap->get_bitmap_data() );
		gr_set_bitmap_flags(*least_recently_used->bitmap.get(), BM_FLAG_TRANSPARENT);
		least_recently_used->bitmap->avg_color = bitmap_top->avg_color;
	} else	{
		merge_textures_new( orient, *bitmap_bottom, *bitmap_top, least_recently_used->bitmap->get_bitmap_data() );
		least_recently_used->bitmap->bm_flags = bitmap_bottom->bm_flags & (~BM_FLAG_RLE);
		least_recently_used->bitmap->avg_color = bitmap_bottom->avg_color;
	}

	least_recently_used->top_bmp = bitmap_top;
	least_recently_used->bottom_bmp = bitmap_bottom;
	least_recently_used->last_time_used = timer_query();
	least_recently_used->orient = orient;
	return *least_recently_used->bitmap.get();
}

void merge_textures_new( int type, const grs_bitmap &rbottom_bmp, const grs_bitmap &rtop_bmp, ubyte * dest_data )
{
	ubyte c = 0;
	int wh;
	auto top_bmp = rle_expand_texture(rtop_bmp);
	auto bottom_bmp = rle_expand_texture(rbottom_bmp);

	const auto &top_data = top_bmp->bm_data;
	const auto &bottom_data = bottom_bmp->bm_data;
	wh = bottom_bmp->bm_w;

	switch( type )	{
		case 0:
			// Normal
			for (int y=0; y<wh; y++ )
				for (int x=0; x<wh; x++ ) {
					c = top_data[ wh*y+x ];
					if (c==TRANSPARENCY_COLOR)
						c = bottom_data[ wh*y+x ];
					*dest_data++ = c;
				}
			break;
		case 1:
			for (int y=0; y<wh; y++ )
				for (int x=0; x<wh; x++ )
				{
					c = top_data[ wh*x+((wh-1)-y) ];
					if (c==TRANSPARENCY_COLOR)
						c = bottom_data[ wh*y+x ];
					*dest_data++ = c;
				}
			break;
		case 2:
			for (int y=0; y<wh; y++ )
				for (int x=0; x<wh; x++ )
				{
					c = top_data[ wh*((wh-1)-y)+((wh-1)-x) ];
					if (c==TRANSPARENCY_COLOR)
						c = bottom_data[ wh*y+x ];
					*dest_data++ = c;
				}
			break;
		case 3:
			for (int y=0; y<wh; y++ )
				for (int x=0; x<wh; x++ )
				{
					c = top_data[ wh*((wh-1)-x)+y  ];
					if (c==TRANSPARENCY_COLOR)
						c = bottom_data[ wh*y+x ];
					*dest_data++ = c;
				}
			break;
	}
}

void merge_textures_super_xparent( int type, const grs_bitmap &rbottom_bmp, const grs_bitmap &rtop_bmp, ubyte * dest_data )
{
	ubyte c = 0;
	int wh;
	auto top_bmp = rle_expand_texture(rtop_bmp);
	auto bottom_bmp = rle_expand_texture(rbottom_bmp);

	const auto &top_data = top_bmp->bm_data;
	const auto &bottom_data = bottom_bmp->bm_data;
	wh = bottom_bmp->bm_w;

	switch( type )
	{
		case 0:
			// Normal
			for (int y=0; y<wh; y++ )
				for (int x=0; x<wh; x++ )
				{
					c = top_data[ wh*y+x ];
					if (c==TRANSPARENCY_COLOR)
						c = bottom_data[ wh*y+x ];
					else if (c==254)
						c = TRANSPARENCY_COLOR;
					*dest_data++ = c;
				}
			break;
		case 1:
			// 
			for (int y=0; y<wh; y++ )
				for (int x=0; x<wh; x++ )
				{
					c = top_data[ wh*x+((wh-1)-y) ];
					if (c==TRANSPARENCY_COLOR)
						c = bottom_data[ wh*y+x ];
					else if (c==254)
						c = TRANSPARENCY_COLOR;
					*dest_data++ = c;
				}
			break;
		case 2:
			// Normal
			for (int y=0; y<wh; y++ )
				for (int x=0; x<wh; x++ )
				{
					c = top_data[ wh*((wh-1)-y)+((wh-1)-x) ];
					if (c==TRANSPARENCY_COLOR)
						c = bottom_data[ wh*y+x ];
					else if (c==254)
						c = TRANSPARENCY_COLOR;
					*dest_data++ = c;
				}
			break;
		case 3:
			// Normal
			for (int y=0; y<wh; y++ )
				for (int x=0; x<wh; x++ )
				{
					c = top_data[ wh*((wh-1)-x)+y  ];
					if (c==TRANSPARENCY_COLOR)
						c = bottom_data[ wh*y+x ];
					else if (c==254)
						c = TRANSPARENCY_COLOR;
					*dest_data++ = c;
				}
			break;
	}
}
