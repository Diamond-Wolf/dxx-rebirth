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
 * Menu prototypes and variables
 *
 */

#ifndef _MENU_H
#define _MENU_H

#ifdef __cplusplus

extern int hide_menus(void);
extern void show_menus(void);

// called once at program startup to get the player's name
extern int RegisterPlayer();

// returns number of item chosen
extern int DoMenu();
extern void do_options_menu();
extern int select_demo(void);

#if defined(DXX_BUILD_DESCENT_I)
#define Menu_pcx_name (((SWIDTH>=640&&SHEIGHT>=480) && PHYSFSX_exists("menuh.pcx",1))?"menuh.pcx":"menu.pcx")
#define STARS_BACKGROUND (((SWIDTH>=640&&SHEIGHT>=480) && PHYSFSX_exists("starsb.pcx",1))?"starsb.pcx":"stars.pcx")
#elif defined(DXX_BUILD_DESCENT_II)
#define MENU_PCX_MAC_SHARE ("menub.pcx")
#define MENU_PCX_SHAREWARE ("menud.pcx")
#define MENU_PCX_OEM (HIRESMODE?"menuob.pcx":"menuo.pcx")
#define MENU_PCX_FULL (HIRESMODE?"menub.pcx":"menu.pcx")

// name of background bitmap
#define Menu_pcx_name (PHYSFSX_exists(MENU_PCX_FULL,1)?MENU_PCX_FULL:(PHYSFSX_exists(MENU_PCX_OEM,1)?MENU_PCX_OEM:PHYSFSX_exists(MENU_PCX_SHAREWARE,1)?MENU_PCX_SHAREWARE:MENU_PCX_MAC_SHARE))
#define STARS_BACKGROUND ((HIRESMODE && PHYSFSX_exists("starsb.pcx",1))?"starsb.pcx":PHYSFSX_exists("stars.pcx",1)?"stars.pcx":"starsb.pcx")
#endif

#endif

#endif /* _MENU_H */
