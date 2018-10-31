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

// mankrip - begin
#include "quakedef.h"

/*
2D files can be either WAD, LMP, PCX, TGA, MIP, WAL, SPR, SPR32

BUGS:
- frag counter still appears after loading a single player save during a deathmatch game
- newline on the wrong position for console text (load a save to see)
- ao pressionar F4 para voltar ao menu de opções a partir do Player Setup menu, as opções não são gravadas;
	só são gravadas ao se sair com Esc
- on-screen keyboard still active after typing commands, including 'disconnect' (feature?)
- call the on-screen keyboard while either on the console or in the Player Setup menu,
	press F3 to load a game, load a game and press T to chat. The on-screen keyboard is still active.

TO CHECK:
- Draw_Fill for clearing when letterboxing is active
- centerprinting when letterboxing is active

FIXED:
- console text is aligned to scr_2d_top instead of line zero
- Draw_Fill not clearing net icon right
- Draw_Fill not clearing pause plaque right
- Draw_Fill not clearing console notifications right
- Draw_Fill not clearing centerprints right
- Draw_Fill not clearing classic status bar right
- Draw_Fill always clears the whole screen during intermission with the console active
- Draw_Fill always clears the whole screen during deathmatch
- translucent console background not working
- mission packs' status bars
- Fightoon status bar
*/
int
	scr_2d_align_h // left, center, right
,	scr_2d_align_v // top, center, bottom
,	scr_2d_scale_h = 1
,	scr_2d_scale_v = 1
	// non-scaled:
,	scr_2d_left
,	scr_2d_right
,	scr_2d_top
,	scr_2d_bottom
,	scr_2d_width
,	scr_2d_height
,	scr_2d_offset_x
,	scr_2d_offset_y
,	scr_2d_padding_x
,	scr_2d_padding_y
	;
cvar_t
// screen positioning - begin
	scr_left	= {"scr_left"  , "0", true}
,	scr_right	= {"scr_right" , "0", true}
#ifdef _arch_dreamcast
//	scr_left	= {"scr_left"  , "4"  , true} // 12 pixels in 320x240 (2.5 = 8)
//,	scr_right	= {"scr_right" , "4"  , true} // 12 pixels in 320x240 (2.5 = 8)
,	scr_top		= {"scr_top"   , "5"  , true} // 12 pixels in 320x240
,	scr_bottom	= {"scr_bottom", "8.5", true} // 20 pixels in 320x240
#else
,	scr_top		= {"scr_top"   , "0", true}
,	scr_bottom	= {"scr_bottom", "0", true}
#endif
	;



void Draw_UpdateBorders (void)
{
	// não escalonada
//	scr_2d_left   = (scr_left  .value < 0.0f) ? 0 : ((scr_left.value   > 100.0f) ? vid.width  : (int) (3.2f * scr_left  .value));
//	scr_2d_right  = (scr_right .value < 0.0f) ? 0 : ((scr_right.value  > 100.0f) ? vid.width  : (int) (3.2f * scr_right .value));
//	scr_2d_top    = (scr_top   .value < 0.0f) ? 0 : ((scr_top.value    > 100.0f) ? vid.height : (int) (2.0f * scr_top   .value));
//	scr_2d_bottom = (scr_bottom.value < 0.0f) ? 0 : ((scr_bottom.value > 100.0f) ? vid.height : (int) (2.0f * scr_bottom.value));
	scr_2d_left   = (scr_left  .value < 0.0f) ? 0 : ((scr_left  .value > 100.0f) ? vid.width  : (int) ceil ( (float)vid.width  * scr_left  .value / 100.0f));
	scr_2d_right  = (scr_right .value < 0.0f) ? vid.width : ((scr_right .value > 100.0f) ? 0  : (int) ceil ( (float)vid.width  * (100.0f - scr_right .value) / 100.0f));
	scr_2d_top    = (scr_top   .value < 0.0f) ? 0 : ((scr_top   .value > 100.0f) ? vid.height : (int) ceil ( (float)vid.height * scr_top   .value / 100.0f));
	scr_2d_bottom = (scr_bottom.value < 0.0f) ? vid.height : ((scr_bottom.value > 100.0f) ? 0 : (int) ceil ( (float)vid.height * (100.0f - scr_bottom.value) / 100.0f));

	// enforce a minimum of 320x200 for the drawing area, by expanding the borders if needed
	scr_2d_width = scr_2d_right - scr_2d_left;
	scr_2d_height = scr_2d_bottom - scr_2d_top;
	if ( (scr_2d_right - scr_2d_left) < 320)
	{
		scr_2d_left -= (320 - scr_2d_width) / 2;
		scr_2d_right += (321 - scr_2d_width) / 2; // +1 for rounding up on division
	}
	if ( (scr_2d_bottom - scr_2d_top) < 200)
	{
		scr_2d_top -= (200 - scr_2d_height) / 2;
		scr_2d_bottom += (201 - scr_2d_height) / 2; // +1 for rounding up on division
	}
	scr_2d_width = scr_2d_right - scr_2d_left;
	scr_2d_height = scr_2d_bottom - scr_2d_top;

	// make sure the drawing area doesn't go offscreen
	if ( (scr_2d_top + scr_2d_height) > vid.height)
		scr_2d_top = vid.height - scr_2d_height;
	if (scr_2d_top < 0)
		scr_2d_top = 0;

//	if (scr_2d_bottom < (scr_2d_top + scr_2d_height))
		scr_2d_bottom = scr_2d_top + scr_2d_height;
	if (scr_2d_bottom > vid.height)
		scr_2d_bottom = vid.height;

	if ( (scr_2d_left + scr_2d_width) > vid.width)
		scr_2d_left = vid.width - scr_2d_width;
	if (scr_2d_left < 0)
		scr_2d_left = 0;

//	if (scr_2d_right < (scr_2d_left + scr_2d_width))
		scr_2d_right = scr_2d_left + scr_2d_width;
	if (scr_2d_right > vid.width)
		scr_2d_right = vid.width;

	scr_2d_width = scr_2d_right - scr_2d_left;
	scr_2d_height = scr_2d_bottom - scr_2d_top;

	// set up the scaling
	scr_2d_scale_h = scr_2d_width / 320;
	if (scr_2d_scale_h < 1)
		scr_2d_scale_h = 1;
	scr_2d_scale_v = scr_2d_height / 200;
	if (scr_2d_scale_v < 1)
		scr_2d_scale_v = 1;
}



void Draw_UpdateAlignment (int h, int v) // aligns the drawing area
{
	scr_2d_align_h = h;
	scr_2d_align_v = v;
	// calculate offsets
	if (scr_2d_align_h == 0) // left, no adjust
		scr_2d_offset_x = 0;
	else if (scr_2d_align_h == 1) // h-center
		scr_2d_offset_x = (scr_2d_width - 320 * scr_2d_scale_h) >> 1;
	else if (scr_2d_align_h == 2) // right
		scr_2d_offset_x = scr_2d_width - 320 * scr_2d_scale_h;
	scr_2d_padding_x = scr_2d_left + scr_2d_offset_x;

	if (scr_2d_align_v == 0) // top, no adjust
		scr_2d_offset_y = 0;
	else if (scr_2d_align_v == 1) // v-center
		scr_2d_offset_y = (scr_2d_height - 200 * scr_2d_scale_v) >> 1;
	else if (scr_2d_align_v == 2) // bottom
		scr_2d_offset_y = scr_2d_height - 200 * scr_2d_scale_v;
	scr_2d_padding_y = scr_2d_top + scr_2d_offset_y;
}



/*
at position x and y on the screen,
get a source image data with dimensions determined by sourcewidth & sourceheight,
skip ypadding lines and xpadding columns inside it,
get an area of w*h pixels from that point,
crop if necessary,
translate the color of each pixel and draw the result scaled
*/
void Draw2Dimage_ScaledMappedTranslatedTransparent (int x, int y, byte *source, int sourcewidth, int sourceheight, int xpadding, int ypadding, int w, int h, qboolean hflip, byte *translation, byte *color_blending_map, qboolean blendbackwards)//, vflip, rotate)
{
	byte
		* dest
	,	tbyte
		;
	unsigned short
		* pusdest
		;
	int
//		xmin
//	,	xmax
//	,	ymin
//	,	ymax
		v
	,	u
	,	vscale
	,	uscale
		;
	if (xpadding < 0)
		Sys_Error ("Draw2Dimage_ScaledMappedTranslatedTransparent: xpadding < 0");
	if (ypadding < 0)
		Sys_Error ("Draw2Dimage_ScaledMappedTranslatedTransparent: ypadding < 0");
	if (xpadding >= sourcewidth)
		Sys_Error ("Draw2Dimage_ScaledMappedTranslatedTransparent: xpadding >= sourcewidth");
	if (ypadding >= sourceheight)
		Sys_Error ("Draw2Dimage_ScaledMappedTranslatedTransparent: ypadding >= sourceheight");

	if (w > sourcewidth)
		Sys_Error ("Draw2Dimage_ScaledMappedTranslatedTransparent: w > sourcewidth");
	if (h > sourceheight)
		Sys_Error ("Draw2Dimage_ScaledMappedTranslatedTransparent: h > sourceheight");

	if (xpadding + w > sourcewidth)
		w = sourcewidth - xpadding;
	if (ypadding + h > sourceheight)
		h = sourceheight - ypadding;

	// return if offscreen
	if (x + w * scr_2d_scale_h < scr_2d_scale_h - 1)
		return;
	if (y + h * scr_2d_scale_v < scr_2d_scale_v - 1)
		return;
	if (x + scr_2d_scale_h > (signed)vid.width)
		return;
	if (y + scr_2d_scale_v > (signed)vid.height)
		return;

	// crop left
	if (scr_2d_padding_x + x * scr_2d_scale_h < 0)
	{
		int xoffset = scr_2d_padding_x / scr_2d_scale_h + x;
		w += xoffset;
		xpadding -= xoffset;
		x = -scr_2d_padding_x / scr_2d_scale_h;
	}
	// crop right
	if ( (unsigned) (scr_2d_padding_x + (x + w) * scr_2d_scale_h) > vid.width)
		w = ( (signed) (vid.width) - (scr_2d_padding_x + x * scr_2d_scale_h)) / scr_2d_scale_h;
	// crop top
	if (scr_2d_padding_y + y * scr_2d_scale_v < 0)
	{
		int yoffset = scr_2d_padding_y / scr_2d_scale_v + y;
		h += yoffset;
		ypadding -= yoffset;
		y = -scr_2d_padding_y / scr_2d_scale_v;
	}
	// crop bottom
	if ( (unsigned) (scr_2d_padding_y + (y + h) * scr_2d_scale_v) > vid.height)
		h = ( (signed) (vid.height) - (scr_2d_padding_y + y * scr_2d_scale_v)) / scr_2d_scale_v;

	if (!translation)
		translation = identityTable;

	source += (sourcewidth * ypadding) + xpadding;

	if (r_pixbytes == 1)
	{
		for (v = 0 ; v < h ; v++)
		{
			for (u = 0 ; u < w ; u++)
				if ( (tbyte = source[u]) != TRANSPARENT_COLOR)
				{
					dest = vid.buffer
					+ ( (scr_2d_padding_y + (y + v) * scr_2d_scale_v - 1) * vid.rowbytes) // v offset
					+ (scr_2d_padding_x + (x + (hflip ? w - u - 1 : u)) * scr_2d_scale_h - 1) // h offset
					;
					for (vscale = scr_2d_scale_v ; vscale ; vscale--)
						for (uscale = scr_2d_scale_h ; uscale ; uscale--)
							if (color_blending_map)
							{
								if (blendbackwards)
								{
									dest[
									vscale * vid.rowbytes // v offset
									+ uscale // h offset
									] =
									color_blending_map[

									dest[
									vscale * vid.rowbytes // v offset
									+ uscale // h offset
									]

									* 256

									+

									translation[tbyte]
									];
								}
								else
								{
									dest[
									vscale * vid.rowbytes // v offset
									+ uscale // h offset
									] =
									color_blending_map[

									dest[
									vscale * vid.rowbytes // v offset
									+ uscale // h offset
									]

									+

									translation[tbyte]

									* 256
									];
								}
							}
							else
							{
								dest[
								vscale * vid.rowbytes // v offset
								+ uscale // h offset
								] = translation[tbyte];
							}
				}

			source += sourcewidth;
		}
	}
	else
	{
	// FIXME: pretranslate at load time?
		pusdest = (unsigned short *)vid.buffer + y * (vid.rowbytes >> 1) + x;

		for (v = 0 ; v < h; v++)
		{
			for (u = 0 ; u < w ; u++)
				if ( (tbyte=source[u]) != TRANSPARENT_COLOR)
					pusdest[u] = d_8to16table[tbyte];

			pusdest += vid.rowbytes >> 1;
			source += sourcewidth;
		}
	}
}



void M_DrawTransPic (int x, int y, qpic_t *pic)
{
	if (pic)
		Draw2Dimage_ScaledMappedTranslatedTransparent (x, y, pic->data, pic->width, pic->height, 0, 0, pic->width, pic->height, false, identityTable, NULL, false);
}
void M_DrawTransPicMirror (int x, int y, qpic_t *pic)
{
	if (pic)
		Draw2Dimage_ScaledMappedTranslatedTransparent (x, y, pic->data, pic->width, pic->height, 0, 0, pic->width, pic->height, true , identityTable, NULL, false);
}



void M_DrawTransPicTranslate (int x, int y, qpic_t *pic)
{
	if (pic)
		Draw2Dimage_ScaledMappedTranslatedTransparent (x, y, pic->data, pic->width, pic->height, 0, 0, pic->width, pic->height, false, translationTable, NULL, false);
}
void M_DrawTransPicTranslateMirror (int x, int y, qpic_t *pic)
{
	if (pic)
		Draw2Dimage_ScaledMappedTranslatedTransparent (x, y, pic->data, pic->width, pic->height, 0, 0, pic->width, pic->height, true , translationTable, NULL, false);
}



extern byte		*draw_chars;				// 8*8 graphic characters

void M_DrawCharacter (int x, int y, int num)
{
	Draw2Dimage_ScaledMappedTranslatedTransparent (x, y, draw_chars + ( (num & 240) << 6) + ( (num & 15) << 3), 128, 128, 0, 0, 8, 8, false, identityTable, NULL, false);
}

void M_Print (int cx, int cy, char *str)
{
	while (*str)
	{
		M_DrawCharacter (cx, cy, (*str)+128);
		cx += 8;
		str++;
	}
}

void M_PrintWhite (int cx, int cy, char *str)
{
	while (*str)
	{
		M_DrawCharacter (cx, cy, *str);
		cx += 8;
		str++;
	}
}



void M_DrawTextBox (int x, int y, int width, int lines)
{
	qpic_t	*p;
	int		cx, cy;
	int		n;
	int		w = -width & 7; // mankrip
	int		h = -lines & 7; // mankrip

	// draw left side
	cx = x;
	cy = y;
	p = Draw_CachePic ("gfx/box_tl.lmp");
	M_DrawTransPic (cx, cy, p);
	p = Draw_CachePic ("gfx/box_ml.lmp");
	for (n = 0; n < lines; n+=8)//-------------
	{
		cy += 8;
		M_DrawTransPic (cx, cy, p);
	}
	p = Draw_CachePic ("gfx/box_bl.lmp");
	M_DrawTransPic (cx, cy+8 - h, p);//------------

	// draw middle
	cx += 8;
	while (width > 0)
	{
		cy = y;
		p = Draw_CachePic ("gfx/box_tm.lmp");
		M_DrawTransPic (cx, cy, p);
		p = Draw_CachePic ("gfx/box_mm.lmp");
		for (n = 0; n < lines; n+=8) //---------
		{
			cy += 8;
			if (n/8 == 1)//-----------
				p = Draw_CachePic ("gfx/box_mm2.lmp");
			M_DrawTransPic (cx, cy, p);
		}
		p = Draw_CachePic ("gfx/box_bm.lmp");
		M_DrawTransPic (cx, cy+8 - h, p);//-----------!!!!!!!!
		width -= 2*8;
		cx += 16;
	}

	// draw right side
	cy = y;
	p = Draw_CachePic ("gfx/box_tr.lmp");
	M_DrawTransPic (cx - w, cy, p); // mankrip - crosshair - edited
	p = Draw_CachePic ("gfx/box_mr.lmp");
	for (n = 0; n < lines; n+=8)//--------
	{
		cy += 8;
		M_DrawTransPic (cx - w, cy, p); // mankrip - crosshair - edited
	}
	p = Draw_CachePic ("gfx/box_br.lmp");
	M_DrawTransPic (cx - w, cy+8 - h, p); // mankrip - crosshair - edited
}

void M_Draw_Fill (int x, int y, int w, int h, byte c)
{
	Draw_Fill (scr_2d_padding_x + x * scr_2d_scale_h, scr_2d_padding_y + y * scr_2d_scale_v, w * scr_2d_scale_h, h * scr_2d_scale_v, c);
}
// mankrip - end
