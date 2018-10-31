/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// screen.c -- master for refresh, status bar, console, chat, notify, etc

#include "quakedef.h"
#include "r_local.h"
#include "ptc_file.h" // mankrip

// only the refresh window will be updated unless these variables are flagged
int			scr_copytop;
int			scr_copyeverything;

float		scr_con_current;
float		scr_conlines;		// lines of console to display

float		oldfov; // edited
cvar_t		scr_viewsize = {"viewsize","100", true};
cvar_t		scr_fov = {"fov","90"};	// 10 - 170
cvar_t		scr_conspeed = {"scr_conspeed","300"};
cvar_t		scr_centertime = {"scr_centertime","2"};
cvar_t		scr_showram = {"showram","1"};
cvar_t		scr_showturtle = {"showturtle","0"};
cvar_t		scr_showpause = {"showpause","1"};
// mankrip - begin
cvar_t		scr_printspeed = {"scr_printspeed","16"}; // 8 // edited
cvar_t		scr_fading = {"scr_fading","1"};
cvar_t		screenshot_number = {"screenshot_number","0", true};
// mankrip - end

qboolean	scr_initialized;		// ready to draw

qpic_t		*scr_ram;
qpic_t		*scr_net;
//qpic_t		*scr_turtle; // mankrip - removed

int			scr_fullupdate;

int			clearconsole;
int			clearnotify;

viddef_t	vid;				// global video state

vrect_t		*pconupdate;
vrect_t		scr_vrect;

qboolean	scr_disabled_for_loading;
qboolean	scr_drawloading;
float		scr_disabled_time;
qboolean	scr_skipupdate;

qboolean	block_drawing;

void SCR_ScreenShot_f (void);

/*
===============================================================================

CENTER PRINTING

===============================================================================
*/

char		scr_centerstring[1024];
float		scr_centertime_start;	// for slow victory printing
float		scr_centertime_off;
int			scr_center_lines;
int			scr_erase_lines;
int			scr_erase_center;

/*
==============
SCR_CenterPrint

Called for important messages that should stay in the center of the screen
for a few moments
==============
*/
void SCR_CenterPrint (char *str)
{
	strncpy (scr_centerstring, str, sizeof(scr_centerstring)-1);
	scr_centertime_off = scr_centertime.value;
	scr_centertime_start = cl.time;

// count the number of lines for centering
	scr_center_lines = 1;
	while (*str)
	{
		if (*str == '\n')
			scr_center_lines++;
		str++;
	}
}

void SCR_EraseCenterString (void)
{
	#if 0 // mankrip
	int		y;
	int		h; // mankrip
	#endif // mankrip

	if (scr_erase_center++ > vid.numpages)
	{
		scr_erase_lines = 0;
		return;
	}

	#if 0 // mankrip
	// mankrip - svc_letterbox - begin
	if (cl.letterbox)
		y = scr_vrect.y + scr_vrect.height + 4;
	else
	// mankrip - svc_letterbox - end
	if (scr_center_lines <= 4)
		y = vid.height*0.35;
	else
		y = screen_top + 44;//28+16;//48; // mankrip - edited
	#endif // mankrip

	scr_copytop = 1;
	// mankrip - begin
	#if 0
	// limit the height to prevent crashes with huge centerprints
	h = 8*scr_erase_lines;
	if (y+h > vid.height)
		h = vid.height - y;
	Draw_Fill (0, y,vid.width, h, 0);
	#endif
	// mankrip - end
}

void SCR_DrawCenterString (void)
{
	char	*start;
	int		l;
	int		j;
	int		x, y;
	int		remaining;

// the finale prints the characters one at a time
	if (cl.intermission)
		remaining = scr_printspeed.value * (cl.time - scr_centertime_start);
	else
		remaining = 9999;

	scr_erase_center = 0;
	start = scr_centerstring;

	Draw_UpdateAlignment (1, 1);
	// mankrip - svc_letterbox - begin
	if (cl.letterbox)
	{
		y = (scr_vrect.y + scr_vrect.height + 4) / scr_2d_scale_v;
		Draw_UpdateAlignment (1, 0);
	}
	else
	// mankrip - svc_letterbox - end
	if (scr_center_lines <= 4)
		y = 200*0.35; // mankrip - edited
	else
		y = 44;//28+16;//48; // mankrip - edited

	do
	{
	// scan the width of the line
		for (l=0 ; l<40 ; l++)
			if (start[l] == '\n' || !start[l])
				break;
		x = (320 - l*8)/2;
		for (j=0 ; j<l ; j++, x+=8)
		{
			M_DrawCharacter (x, y, start[j]);
			if (!remaining--)
				return;
		}

		y += 8;

		while (*start && *start != '\n')
			start++;

		if (!*start)
			break;
		start++;		// skip the \n
	} while (1);
}

void SCR_CheckDrawCenterString (void)
{
	scr_copytop = 1;
	if (scr_center_lines > scr_erase_lines)
		scr_erase_lines = scr_center_lines;

// 2001-10-20 TIMESCALE extension by Tomaz/Maddes  start
//	scr_centertime_off -= host_frametime;
	scr_centertime_off -= host_cpu_frametime;
// 2001-10-20 TIMESCALE extension by Tomaz/Maddes  end

	if (scr_centertime_off <= 0 && !cl.intermission)
		return;
	if (key_dest != key_game)
		return;

	SCR_DrawCenterString ();
}

//=============================================================================

/*
====================
CalcFov
====================
*/
float CalcFov (float fov_x, float width, float height)
{
        float   a;
        float   x;

        if (fov_x < 1 || fov_x > 179)
                Sys_Error ("Bad fov: %f", fov_x);

        x = width/tan(fov_x/360*M_PI);

        a = atan (height/x);

        a = a*360/M_PI;

        return a;
}

/*
=================
SCR_CalcRefdef

Must be called whenever vid changes
Internal use only
=================
*/
static void SCR_CalcRefdef (void)
{
	vrect_t		vrect;
//	float		size; // mankrip - removed

	scr_fullupdate = 0;		// force a background redraw
	vid.recalc_refdef = 0;

// force the status bar to redraw
	Sbar_Changed ();

//========================================

	r_refdef.fov_x = scr_fov.value;
	r_refdef.fov_y = CalcFov (r_refdef.fov_x, r_refdef.vrect.width, r_refdef.vrect.height);

	fovscale = 90.0f/scr_fov.value; // mankrip - FOV-based scaling
/*/// mankrip - removed - begin
// intermission is always full screen
	if (cl.intermission)
		size = 120;
	else
		size = scr_viewsize.value;

	if (size >= 120)
		sb_lines = 0;		// no status bar at all
	else if (size >= 110)
		sb_lines = 24;		// no inventory
	else
		sb_lines = 24+16+8;
/*/// mankrip - removed - end

// these calculations mirror those in R_Init() for r_refdef, but take no
// account of water warping
	vrect.x = 0;
	vrect.y = 0;
	vrect.width = vid.width;
	vrect.height = vid.height;

	R_SetVrect (&vrect, &scr_vrect, sb_lines);

// guard against going from one mode to another that's less than half the
// vertical resolution
	if (scr_con_current > vid.height)
		scr_con_current = vid.height;

// notify the refresh of the change
	R_ViewChanged (&vrect, sb_lines, vid.aspect);
}


/*
=================
SCR_SizeUp_f

Keybinding command
=================
*/
void SCR_SizeUp_f (void)
{
	Cvar_SetValue ("viewsize",scr_viewsize.value+5); // mankrip - edited
}


/*
=================
SCR_SizeDown_f

Keybinding command
=================
*/
void SCR_SizeDown_f (void)
{
	Cvar_SetValue ("viewsize",scr_viewsize.value-5); // mankrip - edited
}

//============================================================================
/*
==================
SCR_Adjust
==================
*/
void SCR_AdjustFOV (void)
{
// bound field of view
	if (scr_fov.value < 10)
		Cvar_Set ("fov","10");
	if (scr_fov.value > 170)
		Cvar_Set ("fov","170");

	if (oldfov != scr_fov.value)
	{
		oldfov = scr_fov.value;
		vid.recalc_refdef = true;
	}
}
void SCR_Adjust (void)
{
	static float	oldscr_viewsize;
	static float	oldlcd_x;

	if (oldlcd_x != lcd_x.value)
	{
		oldlcd_x = lcd_x.value;
		vid.recalc_refdef = true;
	}

// bound viewsize
	// mankrip - edited - begin
	if (scr_viewsize.value < 50)	// 30
		Cvar_Set ("viewsize","50");	// 30
	else if (scr_viewsize.value > 100)	// 120
		Cvar_Set ("viewsize","100");// 120
	// mankrip - edited - end
	if (scr_viewsize.value != oldscr_viewsize)
	{
		oldscr_viewsize = scr_viewsize.value;
		vid.recalc_refdef = 1;
	}

//
// check for vid changes
//
	// mankrip - screen positioning - begin
	{
		int i = (int)((float)(vid.width) * (scr_left.value/100.0));
		if (scr_left.value > 100)//100
			i = vid.width;
		if (screen_left != i)
		{
			screen_left = i;
			vid.recalc_refdef = true;
		}

		i = (int)((float)(vid.width) * (scr_right.value/100.0));
		if (scr_right.value > 100)//100
			i = vid.width;
		if (screen_right != i)
		{
			screen_right = i;
			vid.recalc_refdef = true;
		}

		i = (int)((float)(vid.height) * (scr_top.value/100.0));
		if (scr_top.value > 100)//100
			i = vid.height;
		if (screen_top != i)
		{
			screen_top = i;
			vid.recalc_refdef = true;
		}

		i = (int)((float)(vid.height) * (scr_bottom.value/100.0));
		if (scr_bottom.value > 100)//100
			i = vid.height;
		if (screen_bottom != i)
		{
			screen_bottom = i;
			vid.recalc_refdef = true;
		}
	}
//	if (scr_con_current)
//		vid.recalc_refdef = true;
	// mankrip - screen positioning - end
/*	// mankrip - r_letterbox - start
	{
	//	static float old_r_letterbox;
		extern cvar_t r_letterbox;
		if (r_letterbox.value < 0)
			r_letterbox.value = 0;
		else if (r_letterbox.value > 100)
			r_letterbox.value = 100;
	//	if (old_r_letterbox != r_letterbox.value)
		{
	//		old_r_letterbox = r_letterbox.value;
			cl.letterbox = r_letterbox.value/100.0; // svc_letterbox
			vid.recalc_refdef = true;
		}
	}
*/	// mankrip - r_letterbox - end
}
/*
==================
SCR_Init
==================
*/
void SCR_Init (void)
{
	Cvar_RegisterVariableWithCallback (&scr_fov, SCR_AdjustFOV); // mankrip
	Cvar_RegisterVariableWithCallback (&scr_viewsize, SCR_Adjust); // mankrip
//	Cvar_RegisterVariable (&scr_fov);
//	Cvar_RegisterVariable (&scr_viewsize);
	Cvar_RegisterVariable (&scr_conspeed);
	Cvar_RegisterVariable (&scr_showram);
	Cvar_RegisterVariable (&scr_showturtle);
	Cvar_RegisterVariable (&scr_showpause);
	Cvar_RegisterVariable (&scr_centertime);
	Cvar_RegisterVariable (&scr_printspeed);
	Cvar_RegisterVariable (&scr_fading); // mankrip
	Cvar_RegisterVariable (&screenshot_number); // mankrip
	// mankrip - screen positioning - begin
	Cvar_RegisterVariableWithCallback (&scr_left, SCR_Adjust);
	Cvar_RegisterVariableWithCallback (&scr_right, SCR_Adjust);
	Cvar_RegisterVariableWithCallback (&scr_top, SCR_Adjust);
	Cvar_RegisterVariableWithCallback (&scr_bottom, SCR_Adjust);
	// mankrip - screen positioning - end

//
// register our commands
//
	Cmd_AddCommand ("screenshot",SCR_ScreenShot_f);
	Cmd_AddCommand ("sizeup",SCR_SizeUp_f);
	Cmd_AddCommand ("sizedown",SCR_SizeDown_f);

	scr_ram = Draw_PicFromWad ("ram");
	scr_net = Draw_PicFromWad ("net");
//	scr_turtle = Draw_PicFromWad ("turtle"); // mankrip - removed

	scr_initialized = true;
}



/*
==============
SCR_DrawRam
==============
*/
void SCR_DrawRam (void)
{
	if (!scr_showram.value)
		return;

	if (!r_cache_thrash)
		return;

	Draw_UpdateAlignment (0, 0); // mankrip
	M_DrawTransPic (32, 0, scr_ram); // mankrip - edited
}

/*
==============
SCR_DrawNet
==============
*/
void SCR_DrawNet (void)
{
	if (realtime - cl.last_received_message < 0.3)
		return;
	if (cls.demoplayback)
		return;

	Draw_UpdateAlignment (0, 0); // mankrip
	M_DrawTransPic (64, 0, scr_net); // mankrip - edited
}

// 2001-11-31 FPS display by QuakeForge/Muff  start
/*
==============
SCR_DrawFPS
==============
*/
//muff - hacked out of SourceForge implementation + modified
void SCR_DrawFPS (void)
{
	static double lastframetime;
	double t;
	static int lastfps;
	int x;//, y; // mankrip - edited
	char st[80];

	t = Sys_FloatTime ();
	if ( (t - lastframetime) >= 1.0 /* 0.5 */)
	{
		lastfps = fps_count;
		fps_count = 0;
		lastframetime = t + 1.0 /* 0.5 */ - (t - lastframetime);
	}

	sprintf (st, "%3d FPS", lastfps /* *2 */);

	x = (320 - strlen (st) * 8);// / 2; // mankrip - edited
	// mankrip - begin
	Draw_UpdateAlignment (2, 0);
	#if 0
	if (scr_vrect.y > screen_top)
		M_Draw_Fill (320 - strlen(st) * 8, 0, strlen (st) * 8, 8, 0);
	#endif
	// mankrip - end
	Draw_String (x, 0, st); // x, y, st // mankrip - edited
}
// 2001-11-31 FPS display by QuakeForge/Muff  end

/*
==============
DrawPause
==============
*/
void SCR_DrawPause (void)
{
	// mankrip - begin
	if (cl.paused)
		if (scr_showpause.value) // turn off for screenshots
		{
			extern cvar_t sbar;
			if (sbar.value == 5)
			{
				qpic_t	*pic;
				pic = Draw_CachePic ("gfx/pause.lmp");
				Draw_UpdateAlignment (1, 2);
				M_DrawTransPic (160 - pic->width / 2, 200 - pic->height, pic);
			}
			else
			{
				Draw_UpdateAlignment (1, 0);
				M_DrawPlaque ("gfx/pause.lmp", false);
			}
		}
	// mankrip - end
}



/*
==============
SCR_DrawLoading
==============
*/
void SCR_DrawLoading (void)
{
	qpic_t	*pic;

	if (!scr_drawloading)
		return;

	pic = Draw_CachePic ("gfx/loading.lmp");
	Draw_UpdateAlignment (1, 1); // mankrip
	M_DrawTransPic ( (320 - pic->width) / 2, (200 - pic->height - sb_lines / scr_2d_scale_v) / 2, pic);
}



//=============================================================================


/*
==================
SCR_SetUpToDrawConsole
==================
*/
void SCR_SetUpToDrawConsole (void)
{
//	extern cvar_t	con_alpha; // mankrip - transparent console
	Con_CheckResize ();

	if (scr_drawloading)
		return;		// never a console with loading plaque

// decide on the height of the console
	con_forcedup = !cl.worldmodel || cls.signon != SIGNONS;

	if (con_forcedup)
	{
		scr_conlines = vid.height;		// full screen
		scr_con_current = scr_conlines;
	}
	else if (key_dest == key_console)
		scr_conlines = vid.height/2;	// half screen
	else
		scr_conlines = 0;				// none visible

	if (scr_conlines < scr_con_current)
	{
// 2001-10-20 TIMESCALE extension by Tomaz/Maddes  start
//		scr_con_current -= scr_conspeed.value*host_frametime;
		scr_con_current -= scr_conspeed.value * host_cpu_frametime * ( (float) vid.height / 400.0f);// 200.0f); // mankrip - slower console speed to compensate for framerate and resolution independency - edited
// 2001-10-20 TIMESCALE extension by Tomaz/Maddes  end
		if (scr_conlines > scr_con_current)
			scr_con_current = scr_conlines;

	}
	else if (scr_conlines > scr_con_current)
	{
// 2001-10-20 TIMESCALE extension by Tomaz/Maddes  start
//		scr_con_current += scr_conspeed.value*host_frametime;
		scr_con_current += scr_conspeed.value * host_cpu_frametime * ( (float) vid.height / 400.0f);// 200.0f); // mankrip - slower console speed to compensate for framerate and resolution independency - edited
// 2001-10-20 TIMESCALE extension by Tomaz/Maddes  end
		if (scr_conlines < scr_con_current)
			scr_con_current = scr_conlines;
	}

	if (clearconsole++ < vid.numpages)
	{
		scr_copytop = 1;
		#if 0 // mankrip
		if (con_alpha.value == 1.0) // mankrip - transparent console
			Draw_Fill (0, (int)scr_con_current, vid.width, vid.height-(int)scr_con_current, 0); // mankrip - edited
		else // mankrip - transparent console
			Draw_Fill (0, 0, vid.width, vid.height, 0); // mankrip - transparent console
		#endif // mankrip
		Sbar_Changed ();
	}
	else if (clearnotify++ < vid.numpages && scr_vrect.y > screen_top) // mankrip - edited
	{
		scr_copytop = 1;
		#if 0 // mankrip
		Draw_Fill (0, screen_top, vid.width, con_notifylines, 0); // mankrip - edited
		#endif // mankrip
	}
	else
		con_notifylines = 0;
}

/*
==================
SCR_DrawConsole
==================
*/
void SCR_DrawConsole (void)
{
	if (scr_con_current)
	{
		scr_copyeverything = 1;
		Con_DrawConsole (scr_con_current, true);
		clearconsole = 0;
	}
	else
	{
		if (key_dest == key_game || key_dest == key_message)
			Con_DrawNotify ();	// only draw notify in game
	}
}


/*
==============================================================================

						SCREEN SHOTS

==============================================================================
*/

/*
==================
SCR_ScreenShot_f
==================
*/
void SCR_ScreenShot_f (void)
{
#ifndef _arch_dreamcast // mankrip
	int     i, last;
	char		pcxname[80];
	char		checkname[MAX_OSPATH];

//
// find a file name to save it to
//
	// mankrip - begin
	strcpy(pcxname,"scr_0000.pcx");

	last = minmaxclamp (0, (int) screenshot_number.value, 9999);
	for (i=9999 ; i>=last ; i--)
	{
		pcxname[4] =  i/1000		+ '0';
		pcxname[5] = (i/100	)%10	+ '0';
		pcxname[6] = (i/10	)%10	+ '0';
		pcxname[7] = (i/1	)%10	+ '0';
	// mankrip - end
		sprintf (checkname, "%s/%s", com_gamedir, pcxname);
		if (Sys_FileTime(checkname) != -1) // mankrip - edited
		{
		//	i--;
			break;	// file does exist
		}
	}
	// the counter isn't decremented after finding the file, but it is if the file doesn't exist
	// however, if the file doesn't exist (pcxname) isn't updated on the last cycle, so it won't match (i)
	// therefore, we must always increment (i) here
	if (++i > 9999) // mankrip - edited
	{
		Con_Printf ("SCR_ScreenShot_f: Couldn't create a PCX file\n");
		return;
 	}
	// mankrip - begin
	pcxname[4] =  i/1000		+ '0';
	pcxname[5] = (i/100	)%10	+ '0';
	pcxname[6] = (i/10	)%10	+ '0';
	pcxname[7] = (i/1	)%10	+ '0';
 	Cvar_SetValue ("screenshot_number", (float) i+1);
	// mankrip - end

//
// save the pcx file
//
	D_EnableBackBufferAccess ();	// enable direct drawing of console to back
									//  buffer

	WritePCXfile (pcxname, vid.buffer, vid.width, vid.height, vid.rowbytes,
				  shifted_basepal); // mankrip

	D_DisableBackBufferAccess ();	// for adapters that can't stay mapped in
									//  for linear writes all the time

	Con_Printf ("Wrote %s\n", pcxname);
#endif // mankrip
}


//=============================================================================


/*
===============
SCR_BeginLoadingPlaque

================
*/
void SCR_BeginLoadingPlaque (void)
{
	S_StopAllSounds (true);

	if (cls.state != ca_connected)
		return;
	if (cls.signon != SIGNONS)
		return;

	Vibration_Stop (0); // mankrip
	Vibration_Stop (1); // mankrip
// redraw with no console and the loading plaque
	Con_ClearNotify ();
	scr_centertime_off = 0;
	scr_con_current = 0;

	scr_drawloading = true;
	scr_fullupdate = 0;
	Sbar_Changed ();
	SCR_UpdateScreen ();
	scr_drawloading = false;

	scr_disabled_for_loading = true;
	scr_disabled_time = realtime;
	scr_fullupdate = 0;
}

/*
===============
SCR_EndLoadingPlaque

================
*/
void SCR_EndLoadingPlaque (void)
{
	scr_disabled_for_loading = false;
	scr_fullupdate = 0;
	Con_ClearNotify ();
}

//=============================================================================

char	*scr_notifystring;

void SCR_DrawNotifyString (void)
{
	char	*start;
	int		l;
	int		j;
	int		x, y;

	start = scr_notifystring;

	y = 200*0.35; // mankrip - edited
	Draw_UpdateAlignment (0, 0); // mankrip

	do
	{
	// scan the width of the line
		for (l=0 ; l<40 ; l++)
			if (start[l] == '\n' || !start[l])
				break;
		x = (320 - l*8)/2; // mankrip - edited
		for (j=0 ; j<l ; j++, x+=8)
			M_DrawCharacter (x, y, start[j]); // mankrip - edited

		y += 8;

		while (*start && *start != '\n')
			start++;

		if (!*start)
			break;
		start++;		// skip the \n
	} while (1);
}

//=============================================================================

/*
===============
SCR_BringDownConsole

Brings the console down and fades the palettes back to normal
================
*/
void SCR_BringDownConsole (void)
{
	int		i;

	scr_centertime_off = 0;

	for (i=0 ; i<20 && scr_conlines != scr_con_current ; i++)
		SCR_UpdateScreen ();

	cl.cshifts[0].percent = 0;		// no area contents palette on next frame
	VID_SetPalette (host_basepal);
}


/*
==================
SCR_UpdateScreen

This is called every frame, and can also be called explicitly to flush
text to the screen.

WARNING: be very careful calling this from elsewhere, because the refresh
needs almost the entire 256k of stack space!
==================
*/
void SCR_UpdateScreen (void)
{
	vrect_t		vrect;

	if (scr_skipupdate || block_drawing)
		return;

	scr_copytop = 0;
	scr_copyeverything = 0;

	if (scr_disabled_for_loading)
	{
		if (realtime - scr_disabled_time > 60)
		{
			scr_disabled_for_loading = false;
			Con_Printf ("load failed.\n");
		}
		else
			return;
	}

	if (cls.state == ca_dedicated)
		return;				// stdout only

	if (!scr_initialized || !con_initialized)
		return;				// not initialized yet

	// mankrip - removi código daqui

	// mankrip - start
	{
		static byte old_sb_lines;
		if (sb_lines != old_sb_lines)
		{
			old_sb_lines = sb_lines;
			vid.recalc_refdef = true;
		}
	}
	// mankrip - end

	if (vid.recalc_refdef)
	{
	// something changed, so reorder the screen
		SCR_CalcRefdef ();
		Draw_UpdateBorders (); // mankrip
		D_Particles_Recalc (); // mankrip
	}

//
// do 3D refresh drawing, and then update the screen
//
	D_EnableBackBufferAccess ();	// of all overlay stuff if drawing directly

	if (scr_fullupdate++ < vid.numpages)
	{	// clear the entire screen
		scr_copyeverything = 1;
//		Draw_Fill (0,0,vid.width,vid.height, 0); // mankrip - edited
	//	Sbar_Changed ();
	}
	#if 0
	if (scr_copyeverything)
	{
		Draw_Fill (							   0,							   0, vid.width									 ,									 scr_vrect.y, 0); // top
		Draw_Fill (							   0, scr_vrect.y					,								  scr_vrect.x,								scr_vrect.height, 0); // left
		Draw_Fill (scr_vrect.x + scr_vrect.width, scr_vrect.y					, vid.width - (scr_vrect.x + scr_vrect.width),								scr_vrect.height, 0); // right
		Draw_Fill (							   0, scr_vrect.y + scr_vrect.height, vid.width									 , vid.height - (scr_vrect.y + scr_vrect.height), 0); // bottom
	}
	else if (scr_copytop)
	#endif
	{
		// not edited yet, so same as above
		Draw_Fill (							   0,							   0, vid.width									 ,									 scr_vrect.y, 0); // top
		Draw_Fill (							   0, scr_vrect.y					,								  scr_vrect.x,								scr_vrect.height, 0); // left
		Draw_Fill (scr_vrect.x + scr_vrect.width, scr_vrect.y					, vid.width - (scr_vrect.x + scr_vrect.width),								scr_vrect.height, 0); // right
		Draw_Fill (							   0, scr_vrect.y + scr_vrect.height, vid.width									 , vid.height - (scr_vrect.y + scr_vrect.height), 0); // bottom
	}

	pconupdate = NULL;


	SCR_SetUpToDrawConsole ();
	SCR_EraseCenterString ();

	D_DisableBackBufferAccess ();	// for adapters that can't stay mapped in
									//  for linear writes all the time

	VID_LockBuffer ();

	V_RenderView ();

	VID_UnlockBuffer ();

	D_EnableBackBufferAccess ();	// of all overlay stuff if drawing directly

	if (scr_drawloading)
	{
		SCR_DrawLoading ();
//		Sbar_Draw (); // mankrip - removed
	}
	else if (cl.intermission == 1 && key_dest == key_game)
	{
		Sbar_IntermissionOverlay ();
	}
	else if (cl.intermission == 2 && key_dest == key_game)
	{
		Sbar_FinaleOverlay ();
		SCR_CheckDrawCenterString ();
	}
	else if (cl.intermission == 3 && key_dest == key_game)
	{
		SCR_CheckDrawCenterString ();
	}
	else
	{
		SCR_DrawRam ();
		SCR_DrawNet ();
	//	SCR_DrawTurtle (); // mankrip - removed
		SCR_DrawPause ();
		Sbar_Draw (); // mankrip - moved here, so centerprints don't get occluded by the crosshair
		SCR_CheckDrawCenterString ();
		SCR_DrawConsole ();
		M_Draw ();
		if (cl_showfps.value) SCR_DrawFPS ();	// 2001-11-31 FPS display by QuakeForge/Muff
	}

	D_DisableBackBufferAccess ();	// for adapters that can't stay mapped in
									//  for linear writes all the time
	if (pconupdate)
	{
		D_UpdateRects (pconupdate);
	}

	V_UpdatePalette ();

//
// update one of three areas
//
	if (scr_copyeverything)
	{
		vrect.x = 0;
		vrect.y = 0;
		vrect.width = vid.width;
		vrect.height = vid.height;
		vrect.pnext = 0;

		VID_Update (&vrect);
	}
	else if (scr_copytop)
	{
		vrect.x = 0;
		vrect.y = 0;
		vrect.width = vid.width;
		vrect.height = vid.height - sb_lines - screen_bottom; // mankrip - edited
		vrect.pnext = 0;

		VID_Update (&vrect);
	}
	else
	{
		vrect.x = scr_vrect.x;
		vrect.y = scr_vrect.y;
		vrect.width = scr_vrect.width;
		vrect.height = scr_vrect.height;
		vrect.pnext = 0;

		VID_Update (&vrect);
	}
}


/*
==================
SCR_UpdateWholeScreen
==================
*/
void SCR_UpdateWholeScreen (void)
{
	scr_fullupdate = 0;
	SCR_UpdateScreen ();
}
