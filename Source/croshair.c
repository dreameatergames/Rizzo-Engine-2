/*
Copyright (C) 2012 mankrip ( mankrip@gmail.com )

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

#include "quakedef.h"

cvar_t
	crosshair		= {"crosshair"		, "0", true}
,	crosshair_color	= {"crosshair_color", "1", true}
,	cl_crossx		= {"cl_crossx"		, "0", false}
,	cl_crossy		= {"cl_crossy"		, "0", false}
	;
byte
	crosshair_colormap[256]
,	crosshair_tex[5][11*11] = // [5][11][11]
	{
		{
			255,255,255,255,255,255,255,255,255,255,255
		,	255,255,255,255,255,255,255,255,255,255,255
		,	255,255,255,255,255, 21,255,255,255,255,255
		,	255,255,255,255,255, 26,255,255,255,255,255
		,	255,255,255,255,255, 31,255,255,255,255,255
		,	255,255, 21, 26, 31, 31, 31, 26, 21,255,255
		,	255,255,255,255,255, 31,255,255,255,255,255
		,	255,255,255,255,255, 26,255,255,255,255,255
		,	255,255,255,255,255, 21,255,255,255,255,255
		,	255,255,255,255,255,255,255,255,255,255,255
		,	255,255,255,255,255,255,255,255,255,255,255
		}
	,	{
			255,255,255,255,255,255,255,255,255,255,255
		,	255,255,255,255,255, 21,255,255,255,255,255
		,	255,255,255,255,255, 26,255,255,255,255,255
		,	255,255,255,255,255, 31,255,255,255,255,255
		,	255,255,255,255,255,255,255,255,255,255,255
		,	255, 21, 26, 31,255,255,255, 31, 26, 21,255
		,	255,255,255,255,255,255,255,255,255,255,255
		,	255,255,255,255,255, 31,255,255,255,255,255
		,	255,255,255,255,255, 26,255,255,255,255,255
		,	255,255,255,255,255, 21,255,255,255,255,255
		,	255,255,255,255,255,255,255,255,255,255,255
		}
	,	{
			255,255,255, 21, 26, 31, 26, 21,255,255,255
		,	255,255,255,255,255,255,255,255,255,255,255
		,	255,255,255,255,255,255,255,255,255,255,255
		,	 21,255,255,255,255,255,255,255,255,255, 21
		,	 26,255,255,255,255,255,255,255,255,255, 26
		,	 31,255,255,255,255, 26,255,255,255,255, 31
		,	 26,255,255,255,255,255,255,255,255,255, 26
		,	 21,255,255,255,255,255,255,255,255,255, 21
		,	255,255,255,255,255,255,255,255,255,255,255
		,	255,255,255,255,255,255,255,255,255,255,255
		,	255,255,255, 21, 26, 31, 26, 21,255,255,255
		}
	,	{
			255,255,255,255,255,255,255,255,255,255,255
		,	255, 31, 26, 21,255,255,255, 21, 26, 31,255
		,	255, 26,255,255,255,255,255,255,255, 26,255
		,	255, 21,255,255,255,255,255,255,255, 21,255
		,	255,255,255,255,255,255,255,255,255,255,255
		,	255,255,255,255,255,255,255,255,255,255,255
		,	255,255,255,255,255,255,255,255,255,255,255
		,	255, 21,255,255,255,255,255,255,255, 21,255
		,	255, 26,255,255,255,255,255,255,255, 26,255
		,	255, 31, 26, 21,255,255,255, 21, 26, 31,255
		,	255,255,255,255,255,255,255,255,255,255,255
		}
	,	{
			255,255,255,255,255,255,255,255,255,255,255
		,	255,255,255,255,255,255,255,255,255,255,255
		,	255,255,255,255,255,255,255,255,255,255,255
		,	255,255,255,255,255,255,255,255,255,255,255
		,	255,255,255,255,255,255,255,255,255,255,255
		,	255,255,255,255,255, 31,255,255,255,255,255
		,	255,255,255,255,255,255,255,255,255,255,255
		,	255,255,255,255,255,255,255,255,255,255,255
		,	255,255,255,255,255,255,255,255,255,255,255
		,	255,255,255,255,255,255,255,255,255,255,255
		,	255,255,255,255,255,255,255,255,255,255,255
		}
	}
,	* crosshair_img
	;



void Crosshair_Set (void)
{
	byte
		c = minmaxclamp (0, (int) crosshair_color.value, 14)//(256 - (int) r_fullbright_colors.value) >> 4 // don't use fullbright colors
		;
	if (crosshair.value)
	{
		BuildColorTranslationTable (c, 0, identityTable, crosshair_colormap);
		crosshair_img = crosshair_tex[minmaxclamp (0, (int) crosshair.value - 1, 4)];
	}
}



void Crosshair_Boot (void) // must not be called before Palette_Init, only after
{
	Cvar_RegisterVariableWithCallback (&crosshair		, Crosshair_Set);
	Cvar_RegisterVariableWithCallback (&crosshair_color	, Crosshair_Set);
	Cvar_RegisterVariableWithCallback (&cl_crossx		, Crosshair_Set);
	Cvar_RegisterVariableWithCallback (&cl_crossy		, Crosshair_Set);
	Crosshair_Set (); // r_fullbright_colors must have been registered before doing this
}



extern void Draw2Dimage_ScaledMappedTranslatedTransparent (int x, int y, byte *source, int sourcewidth, int sourceheight, int xpadding, int ypadding, int w, int h, qboolean hflip, byte *translation, byte *color_blending_map, qboolean blendbackwards);
int
	old_scr_2d_padding_x
,	old_scr_2d_padding_y
	;
void Crosshair_Draw (int x, int y, byte *translation) // draw the crosshair at a specific position on the screen
{
	if (crosshair.value < 0 || crosshair.value > 5)
		crosshair.value = 0;
	if (!crosshair.value)
		return;
	x /= scr_2d_scale_h;
	y /= scr_2d_scale_v;
	old_scr_2d_padding_x = scr_2d_padding_x;
	old_scr_2d_padding_y = scr_2d_padding_y;
	scr_2d_padding_x = 0;
	scr_2d_padding_y = 0;

	Draw2Dimage_ScaledMappedTranslatedTransparent (x, y, crosshair_img, 11, 11, 0, 0, 11, 11, false, translation, colorblendingmode[BLEND_ADDITIVE], false);

	scr_2d_padding_x = old_scr_2d_padding_x;
	scr_2d_padding_y = old_scr_2d_padding_y;
}



void Crosshair_Start (void) // draw the crosshair at the center of the screen
{
#ifdef GLQUAKE
#define scr_vrect r_refdef.vrect // bloody hack
#endif
	int x, y;
	if (crosshair.value < 0 || crosshair.value > 5)
		crosshair.value = 0;
	if (!crosshair.value)
		return;
	x = cl_crossx.value + scr_vrect.x + (scr_vrect.width  - 11 * scr_2d_scale_h) / 2; // - 6 * (vid.width/320);
	#if 1
	y = cl_crossy.value + scr_vrect.y + (scr_vrect.height - 11 * scr_2d_scale_v) / 2; // - 5 * (vid.height<480 ? vid.height/200 : vid.height/240);
	#else
	{
	extern cvar_t cl_nobob;
	y = cl_crossy.value + scr_vrect.y + (scr_vrect.height + (-11 + cl.viewheight + ! (cl_nobob.value || chase_active.value) * 2) * scr_2d_scale_v) / 2; // - 5 * (vid.height<480 ? vid.height/200 : vid.height/240);
	}
	#endif

	Crosshair_Draw (x, y, crosshair_colormap);

	#if 0
	byte color = (int)crosshair_color.value;
	if (!crosshair.value || color > 17)
		return;
	// custom colors
	if (color == 16) // sky blue
		Crosshair_Draw (x, y, 244);
	else if (color == 17) // red
		Crosshair_Draw (x, y, 251);
	// palette colors
	else if (color > 8 && color < 15)
		Crosshair_Draw (x, y, (color-1)*16);
	else if (color)
		Crosshair_Draw (x, y, color*16-1);
	else //if (!color) // black
		Crosshair_Draw (x, y, 0);
	#endif
}
// mankrip - crosshair - end
