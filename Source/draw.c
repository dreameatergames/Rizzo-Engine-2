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

// draw.c -- this is the only file outside the refresh that touches the
// vid buffer

#include "quakedef.h"

typedef struct {
	vrect_t	rect;
	int		width;
	int		height;
	byte	*ptexbytes;
	int		rowbytes;
} rectdesc_t;

static rectdesc_t	r_rectdesc;

byte		*draw_chars;				// 8*8 graphic characters
qpic_t		*draw_disc;
qpic_t		*draw_backtile;

//=============================================================================
/* Support Routines */

typedef struct cachepic_s
{
	char		name[MAX_QPATH];
	cache_user_t	cache;
} cachepic_t;

#define	MAX_CACHED_PICS		128
cachepic_t	menu_cachepics[MAX_CACHED_PICS];
int			menu_numcachepics;


qpic_t	*Draw_PicFromWad (char *name)
{
	return W_GetLumpName (name);
}

// mankrip - begin
extern int		LongSwap (int l);
extern short	ShortSwap (short l);
extern byte		BestColor (int r, int g, int b, int start, int stop);
qpic_t	*Draw_DecodePic (qpic_t *pic)
{
	satpic_t *p = (satpic_t *) pic;
	if (LongSwap (p->width) == 320 && LongSwap (p->height) == 240)
	{
		int i, r, g, b;
		byte c[256], *source;
		source = p->data;
		p->width	= LongSwap (p->width);
		p->height	= LongSwap (p->height);

		for (i=0 ; i<256 ; i++)
		{
			p->palette[i] = ShortSwap (p->palette[i]);
			r = ( ( (int) (p->palette[i]>>0) & 31) + 1) * 8 - 1;
			g = ( ( (int) (p->palette[i]>>5) & 31) + 1) * 8 - 1;
		//	g = ( ( (int) (p->palette[i]>>5) & 63) + 1) * 4 - 1;
			b = ( ( (int) (p->palette[i]>>10) & 31) + 1) * 8 - 1;
			c[i] = BestColor (r, g, b, 0, 255);
		}
		for (i=0 ; i < p->width * p->height ; i++)
			source[i] = c[source[i]];
	}
	if (p->width == 320 && p->height == 240)
		return (qpic_t *) ( (byte *) p + 512);
	return pic;
}
// mankrip - end
/*
================
Draw_CachePic
================
*/
qpic_t	*Draw_CachePic (char *path)
{
	cachepic_t	*pic;
	int			i;
	qpic_t		*dat;

	for (pic=menu_cachepics, i=0 ; i<menu_numcachepics ; pic++, i++)
		if (!strcmp (path, pic->name))
			break;

	if (i == menu_numcachepics)
	{
		if (menu_numcachepics == MAX_CACHED_PICS)
			Sys_Error ("menu_numcachepics == MAX_CACHED_PICS");
		menu_numcachepics++;
		strcpy (pic->name, path);
	}

	dat = Cache_Check (&pic->cache);

	if (dat)
	//	return Draw_DecodePic (dat); // mankrip
		return dat;

//
// load the pic from disk
//
	COM_LoadCacheFile (path, &pic->cache);

	dat = (qpic_t *)pic->cache.data;
	if (!dat)
	{
	//	Sys_Error ("Draw_CachePic: failed to load %s", path);
		Con_DPrintf ("Draw_CachePic: failed to load %s", path);
		return NULL;
	}

	SwapPic (dat);
	RemapBackwards_Pixels (dat->data, dat->width * dat->height);

//	return Draw_DecodePic (dat); // mankrip
	return dat;
}



/*
===============
Draw_Init
===============
*/
void Draw_Init (void)
{
//	int		i; // removed

	draw_chars = W_GetLumpName ("conchars");
	draw_disc = W_GetLumpName ("disc");
	draw_backtile = W_GetLumpName ("backtile");

	r_rectdesc.width = draw_backtile->width;
	r_rectdesc.height = draw_backtile->height;
	r_rectdesc.ptexbytes = draw_backtile->data;
	r_rectdesc.rowbytes = draw_backtile->width;
}



//=============================================================================

//								TEXT CHARACTERS

//=============================================================================




/*
================
Draw_Character

Draws one 8*8 graphics character with 0 being transparent.
It can be clipped to the top of the screen to allow the console to be
smoothly scrolled off.
================
*/
#if 0
void Draw_CharacterShadow (int x, int y, int num) // mankrip
{
	byte			*dest;
	byte			*source;
	unsigned short	*pusdest;
	int				drawline;
	int				drawcol, draw_col; // mankrip
	int				row, col;

	num &= 255;

	if (y <= -8)
		return;			// totally off screen
	// mankrip - begin
	// if the character is totally off of the screen, don't print it
	else if (y >= (int)vid.height)
		return;
	if (x <= -8)
		return;
	else if (x >= (int)vid.width)
		return;
	// mankrip - end

	row = num>>4;
	col = num&15;
	source = draw_chars + (row<<10) + (col<<3);

	if (y < 0)
	{	// clipped
		drawline = 8 + y;
		source -= 128*y;
		y = 0;
	}
	else if (y+8 >= vid.height)			// mankrip
		drawline = (int)vid.height - y;	// mankrip
	else
		drawline = 8;

	// mankrip - begin
	if (x < 0)
	{
		draw_col = drawcol = 8 + x;
		source -= x;
		x = 0;
	}
	else if (x+8 >= vid.width)
		draw_col = drawcol = (int)vid.width - x;
	else
		draw_col = drawcol = 8;
	// mankrip - end


	if (r_pixbytes == 1)
	{
		dest = vid.conbuffer + y*vid.conrowbytes + x;

		if (drawcol == 8) // mankrip
			while (drawline--)
			{
				if (source[0])
					dest[0] = 0;//alphamap[dest[drawcol]];//(byte)vid.colormap[dest[0] + ((32 + 16) * 256)]; // mankrip
				if (source[1])
					dest[1] = 0;//alphamap[dest[drawcol]];//(byte)vid.colormap[dest[1] + ((32 + 16) * 256)]; // mankrip
				if (source[2])
					dest[2] = 0;//alphamap[dest[drawcol]];//(byte)vid.colormap[dest[2] + ((32 + 16) * 256)]; // mankrip
				if (source[3])
					dest[3] = 0;//alphamap[dest[drawcol]];//(byte)vid.colormap[dest[3] + ((32 + 16) * 256)]; // mankrip
				if (source[4])
					dest[4] = 0;//alphamap[dest[drawcol]];//(byte)vid.colormap[dest[4] + ((32 + 16) * 256)]; // mankrip
				if (source[5])
					dest[5] = 0;//alphamap[dest[drawcol]];//(byte)vid.colormap[dest[5] + ((32 + 16) * 256)]; // mankrip
				if (source[6])
					dest[6] = 0;//alphamap[dest[drawcol]];//(byte)vid.colormap[dest[6] + ((32 + 16) * 256)]; // mankrip
				if (source[7])
					dest[7] = 0;//alphamap[dest[drawcol]];//(byte)vid.colormap[dest[7] + ((32 + 16) * 256)]; // mankrip
				source += 128;
				dest += vid.conrowbytes;
			}
		// mankrip - begin
		else
			while (drawline--)
			{
				while (drawcol--)
				{
					if (source[drawcol])
						dest[drawcol] = 0;//alphamap[dest[drawcol]];//(byte)vid.colormap[dest[drawcol] + ((32 + 16) * 256)]; // mankrip
				}
				drawcol = draw_col;
				source += 128;
				dest += vid.conrowbytes;
			}
		// mankrip - end
	}
	else
	{
		// FIXME: pre-expand to native format?
		pusdest = (unsigned short *)((byte *)vid.conbuffer + y*vid.conrowbytes + (x<<1));

		if (drawcol == 8) // mankrip
			while (drawline--)
			{
				if (source[0])
					pusdest[0] = d_8to16table[source[0]];
				if (source[1])
					pusdest[1] = d_8to16table[source[1]];
				if (source[2])
					pusdest[2] = d_8to16table[source[2]];
				if (source[3])
					pusdest[3] = d_8to16table[source[3]];
				if (source[4])
					pusdest[4] = d_8to16table[source[4]];
				if (source[5])
					pusdest[5] = d_8to16table[source[5]];
				if (source[6])
					pusdest[6] = d_8to16table[source[6]];
				if (source[7])
					pusdest[7] = d_8to16table[source[7]];

				source += 128;
				pusdest += (vid.conrowbytes >> 1);
			}
		// mankrip - begin
		else
			while (drawline--)
			{
				while (drawcol--)
				{
					if (source[drawcol])
						pusdest[drawcol] = d_8to16table[source[drawcol]];
				}
				drawcol = draw_col;
				source += 128;
				pusdest += (vid.conrowbytes >> 1);
			}
		// mankrip - end
	}
}
#endif
void Draw_Character (int x, int y, int num)
{
	byte			*dest;
	byte			*source;
	unsigned short	*pusdest;
	int				drawline;
	int				drawcol, draw_col; // mankrip
	int				row, col;

	num &= 255;

	if (y <= -8)
		return;			// totally off screen
	// mankrip - begin
	// if the character is totally off of the screen, don't print it
	else if (y >= (int)vid.height)
		return;
	if (x <= -8)
		return;
	else if (x >= (int)vid.width)
		return;
	// mankrip - end

#if 0
	if (developer.value)
		Draw_CharacterShadow (x+1, y+1, num); // mankrip
#endif

	row = num>>4;
	col = num&15;
	source = draw_chars + (row<<10) + (col<<3);

	if (y < 0)
	{	// clipped
		drawline = 8 + y;
		source -= 128*y;
		y = 0;
	}
	else if (y+8 >= vid.height)			// mankrip
		drawline = (int)vid.height - y;	// mankrip
	else
		drawline = 8;

	// mankrip - begin
	if (x < 0)
	{
		draw_col = drawcol = 8 + x;
		source -= x;
		x = 0;
	}
	else if (x+8 >= vid.width)
		draw_col = drawcol = (int)vid.width - x;
	else
		draw_col = drawcol = 8;
	// mankrip - end


	if (r_pixbytes == 1)
	{
		dest = vid.conbuffer + y*vid.conrowbytes + x;

		if (drawcol == 8) // mankrip
			while (drawline--)
			{
				if (source[0])
					dest[0] = source[0];
				if (source[1])
					dest[1] = source[1];
				if (source[2])
					dest[2] = source[2];
				if (source[3])
					dest[3] = source[3];
				if (source[4])
					dest[4] = source[4];
				if (source[5])
					dest[5] = source[5];
				if (source[6])
					dest[6] = source[6];
				if (source[7])
					dest[7] = source[7];
				source += 128;
				dest += vid.conrowbytes;
			}
		// mankrip - begin
		else
			while (drawline--)
			{
				while (drawcol--)
				{
					if (source[drawcol])
						dest[drawcol] = source[drawcol];
				}
				drawcol = draw_col;
				source += 128;
				dest += vid.conrowbytes;
			}
		// mankrip - end
	}
	else
	{
		// FIXME: pre-expand to native format?
		pusdest = (unsigned short *)((byte *)vid.conbuffer + y*vid.conrowbytes + (x<<1));

		if (drawcol == 8) // mankrip
			while (drawline--)
			{
				if (source[0])
					pusdest[0] = d_8to16table[source[0]];
				if (source[1])
					pusdest[1] = d_8to16table[source[1]];
				if (source[2])
					pusdest[2] = d_8to16table[source[2]];
				if (source[3])
					pusdest[3] = d_8to16table[source[3]];
				if (source[4])
					pusdest[4] = d_8to16table[source[4]];
				if (source[5])
					pusdest[5] = d_8to16table[source[5]];
				if (source[6])
					pusdest[6] = d_8to16table[source[6]];
				if (source[7])
					pusdest[7] = d_8to16table[source[7]];

				source += 128;
				pusdest += (vid.conrowbytes >> 1);
			}
		// mankrip - begin
		else
			while (drawline--)
			{
				while (drawcol--)
				{
					if (source[drawcol])
						pusdest[drawcol] = d_8to16table[source[drawcol]];
				}
				drawcol = draw_col;
				source += 128;
				pusdest += (vid.conrowbytes >> 1);
			}
		// mankrip - end
	}
}

#if 0
void Draw_CharToConback (int num, byte *dest)
{
	int		row, col;
	byte	*source;
	int		drawline;
	int		x;

	row = num>>4;
	col = num&15;
	source = draw_chars + (row<<10) + (col<<3);

	drawline = 8;

	while (drawline--)
	{
		for (x=0 ; x<8 ; x++)
			if (source[x])
				dest[x] = 0x60 + source[x];
		source += 128;
		dest += 320;
	}

}
#endif

/*
================
Draw_String
================
*/
void Draw_String (int x, int y, char *str)
{
	while (*str)
	{
		M_DrawCharacter (x, y, *str);
		str++;
		x += 8;
	}
}

#if 0
/*
================
Draw_DebugChar

Draws a single character directly to the upper right corner of the screen.
This is for debugging lockups by drawing different chars in different parts
of the code.
================
*/
void Draw_DebugChar (char num)
{
	byte			*dest;
	byte			*source;
	int				drawline;
	extern byte		*draw_chars;
	int				row, col;

	if (!vid.direct)
		return;		// don't have direct FB access, so no debugchars...

	drawline = 8;

	row = num>>4;
	col = num&15;
	source = draw_chars + (row<<10) + (col<<3);

	dest = vid.direct + 312;

	while (drawline--)
	{
		dest[0] = source[0];
		dest[1] = source[1];
		dest[2] = source[2];
		dest[3] = source[3];
		dest[4] = source[4];
		dest[5] = source[5];
		dest[6] = source[6];
		dest[7] = source[7];
		source += 128;
		dest += 320;
	}
}
#endif



//=============================================================================

//								2D SPRITES

//=============================================================================



/*
=============
Draw_TransPic
=============
*/
void Draw_TransPic (int x, int y, qpic_t *pic)
{
	byte	*dest, *source, tbyte;
	unsigned short	*pusdest;
	int				v, u;

	if (!pic) return; // mankrip

	if (x < 0 || (unsigned)(x + pic->width) > vid.width ||
		y < 0 || (unsigned)(y + pic->height) > vid.height)
		Sys_Error ("Draw_TransPic: bad coordinates");

	source = pic->data;

	if (r_pixbytes == 1)
	{
		dest = vid.buffer + y * vid.rowbytes + x;

		if (pic->width & 7)
		{	// general
			for (v=0 ; v<pic->height ; v++)
			{
				for (u=0 ; u<pic->width ; u++)
					if ( (tbyte=source[u]) != TRANSPARENT_COLOR)
						dest[u] = tbyte;

				dest += vid.rowbytes;
				source += pic->width;
			}
		}
		else
		{	// unwound
			for (v=0 ; v<pic->height ; v++)
			{
				for (u=0 ; u<pic->width ; u+=8)
				{
					if ( (tbyte=source[u]) != TRANSPARENT_COLOR)
						dest[u] = tbyte;
					if ( (tbyte=source[u+1]) != TRANSPARENT_COLOR)
						dest[u+1] = tbyte;
					if ( (tbyte=source[u+2]) != TRANSPARENT_COLOR)
						dest[u+2] = tbyte;
					if ( (tbyte=source[u+3]) != TRANSPARENT_COLOR)
						dest[u+3] = tbyte;
					if ( (tbyte=source[u+4]) != TRANSPARENT_COLOR)
						dest[u+4] = tbyte;
					if ( (tbyte=source[u+5]) != TRANSPARENT_COLOR)
						dest[u+5] = tbyte;
					if ( (tbyte=source[u+6]) != TRANSPARENT_COLOR)
						dest[u+6] = tbyte;
					if ( (tbyte=source[u+7]) != TRANSPARENT_COLOR)
						dest[u+7] = tbyte;
				}
				dest += vid.rowbytes;
				source += pic->width;
			}
		}
	}
	else
	{
	// FIXME: pretranslate at load time?
		pusdest = (unsigned short *)vid.buffer + y * (vid.rowbytes >> 1) + x;

		for (v=0 ; v<pic->height ; v++)
		{
			for (u=0 ; u<pic->width ; u++)
				if ( (tbyte=source[u]) != TRANSPARENT_COLOR)
					pusdest[u] = d_8to16table[tbyte];

			pusdest += vid.rowbytes >> 1;
			source += pic->width;
		}
	}
}
// mankrip - begin
// Fightoon additions
void Draw_TransPicMirror (int x, int y, qpic_t *pic)
{
	byte	*dest, *source, tbyte;
//	unsigned short	*pusdest;
	int				v, u;

	if (!pic) return; // mankrip

	if (x < 0 || (unsigned)(x + pic->width) > vid.width ||
		y < 0 || (unsigned)(y + pic->height) > vid.height)
		Sys_Error (va("Draw_TransPic: bad coordinates (%i, %i)", x, y));

	source = pic->data;

	if (r_pixbytes == 1)
	{
		dest = vid.buffer + y * vid.rowbytes + x;

		if (pic->width & 7)
		{	// general
			for (v=0 ; v<pic->height ; v++)
			{
				for (u=0 ; u<pic->width ; u++)
					if ( (tbyte=source[u]) != TRANSPARENT_COLOR)
						dest[pic->width-1-u] = tbyte;

				dest += vid.rowbytes;
				source += pic->width;
			}
		}
		else
		{	// unwound
			for (v=0 ; v<pic->height ; v++)
			{
				for (u=0 ; u<pic->width ; u+=8)
				{
					if ( (tbyte=source[pic->width-u-1]) != TRANSPARENT_COLOR)
						dest[u] = tbyte;
					if ( (tbyte=source[pic->width-u-2]) != TRANSPARENT_COLOR)
						dest[u+1] = tbyte;
					if ( (tbyte=source[pic->width-u-3]) != TRANSPARENT_COLOR)
						dest[u+2] = tbyte;
					if ( (tbyte=source[pic->width-u-4]) != TRANSPARENT_COLOR)
						dest[u+3] = tbyte;
					if ( (tbyte=source[pic->width-u-5]) != TRANSPARENT_COLOR)
						dest[u+4] = tbyte;
					if ( (tbyte=source[pic->width-u-6]) != TRANSPARENT_COLOR)
						dest[u+5] = tbyte;
					if ( (tbyte=source[pic->width-u-7]) != TRANSPARENT_COLOR)
						dest[u+6] = tbyte;
					if ( (tbyte=source[pic->width-u-8]) != TRANSPARENT_COLOR)
						dest[u+7] = tbyte;
				}
				dest += vid.rowbytes;
				source += pic->width;
			}
		}
	}
}
// mankrip - end


/*
=============
Draw_TransPicTranslate
=============
*/
void Draw_TransPicTranslate (int x, int y, qpic_t *pic, byte *translation)
{
	byte	*dest, *source, tbyte;
	unsigned short	*pusdest;
	int				v, u;

	if (!pic) return; // mankrip

	if (x < 0 || (unsigned)(x + pic->width) > vid.width || y < 0 ||
		 (unsigned)(y + pic->height) > vid.height)
	{
		Sys_Error ("Draw_TransPic: bad coordinates");
	}

	source = pic->data;

	if (r_pixbytes == 1)
	{
		dest = vid.buffer + y * vid.rowbytes + x;

		if (pic->width & 7)
		{	// general
			for (v=0 ; v<pic->height ; v++)
			{
				for (u=0 ; u<pic->width ; u++)
					if ( (tbyte=source[u]) != TRANSPARENT_COLOR)
						dest[u] = translation[tbyte];

				dest += vid.rowbytes;
				source += pic->width;
			}
		}
		else
		{	// unwound
			for (v=0 ; v<pic->height ; v++)
			{
				for (u=0 ; u<pic->width ; u+=8)
				{
					if ( (tbyte=source[u]) != TRANSPARENT_COLOR)
						dest[u] = translation[tbyte];
					if ( (tbyte=source[u+1]) != TRANSPARENT_COLOR)
						dest[u+1] = translation[tbyte];
					if ( (tbyte=source[u+2]) != TRANSPARENT_COLOR)
						dest[u+2] = translation[tbyte];
					if ( (tbyte=source[u+3]) != TRANSPARENT_COLOR)
						dest[u+3] = translation[tbyte];
					if ( (tbyte=source[u+4]) != TRANSPARENT_COLOR)
						dest[u+4] = translation[tbyte];
					if ( (tbyte=source[u+5]) != TRANSPARENT_COLOR)
						dest[u+5] = translation[tbyte];
					if ( (tbyte=source[u+6]) != TRANSPARENT_COLOR)
						dest[u+6] = translation[tbyte];
					if ( (tbyte=source[u+7]) != TRANSPARENT_COLOR)
						dest[u+7] = translation[tbyte];
				}
				dest += vid.rowbytes;
				source += pic->width;
			}
		}
	}
	else
	{
	// FIXME: pretranslate at load time?
		pusdest = (unsigned short *)vid.buffer + y * (vid.rowbytes >> 1) + x;

		for (v=0 ; v<pic->height ; v++)
		{
			for (u=0 ; u<pic->width ; u++)
			{
				tbyte = source[u];

				if (tbyte != TRANSPARENT_COLOR)
				{
					pusdest[u] = d_8to16table[tbyte];
				}
			}

			pusdest += vid.rowbytes >> 1;
			source += pic->width;
		}
	}
}

/*
================
Draw_ConsoleBackground

================
*/
void Draw_ConsoleBackgroundLine (byte *src, byte *dest)
{
	memcpy (dest, src, vid.conwidth);
}
void Draw_ConsoleBackgroundLineStretched (byte *src, byte *dest, int fstep)
{
	int
		x = 0
	,	f = 0
		;

	for ( ; x<vid.conwidth ; x+=4)
	{
		dest[x  ] = src[f>>16]; f += fstep;
		dest[x+1] = src[f>>16]; f += fstep;
		dest[x+2] = src[f>>16]; f += fstep;
		dest[x+3] = src[f>>16]; f += fstep;
	}
}
void Draw_ConsoleBackgroundLineStretched_Blend (byte *src, byte *dest, int fstep)
{
	int
		x = 0
	,	f = 0
		;

	#if 1
	colorblendingmap = colorblendingmode[BLEND_WEIGHTEDMEAN];
	#else
	colorblendingmap = colorblendingmode[BLEND_SMOKE];
	#endif

	for ( ; x<vid.conwidth ; x+=4)
	{
		dest[x  ] = colorblendingmap[dest[x  ] + src[f>>16] * 256]; f += fstep;
		dest[x+1] = colorblendingmap[dest[x+1] + src[f>>16] * 256]; f += fstep;
		dest[x+2] = colorblendingmap[dest[x+2] + src[f>>16] * 256]; f += fstep;
		dest[x+3] = colorblendingmap[dest[x+3] + src[f>>16] * 256]; f += fstep;
	}
}
void Draw_ConsoleBackgroundLineStretched_BlendBackwards (byte *src, byte *dest, int fstep)
{
	int
		x = 0
	,	f = 0
		;

	#if 1
	colorblendingmap = colorblendingmode[BLEND_WEIGHTEDMEAN];
	#else
	colorblendingmap = colorblendingmode[BLEND_SMOKE];
	#endif

	for ( ; x<vid.conwidth ; x+=4)
	{
		dest[x  ] = colorblendingmap[dest[x  ] * 256 + src[f>>16]]; f += fstep;
		dest[x+1] = colorblendingmap[dest[x+1] * 256 + src[f>>16]]; f += fstep;
		dest[x+2] = colorblendingmap[dest[x+2] * 256 + src[f>>16]]; f += fstep;
		dest[x+3] = colorblendingmap[dest[x+3] * 256 + src[f>>16]]; f += fstep;
	}
}
void Old_Draw_ConsoleBackground (int lines)
{
	int				x, y, v;
	byte			*src, *dest;
	unsigned short	*pusdest;
	int				f, fstep;
	qpic_t			*conback;
	extern cvar_t	con_alpha; // mankrip - transparent console

	if (!con_alpha.value && !con_forcedup) // mankrip - transparent console
		return; // mankrip - transparent console
	conback = Draw_CachePic ("gfx/conback.lmp");
// draw the pic
	fstep = conback->width*0x10000/vid.conwidth; // mankrip - hi-res console background - edited
	if (r_pixbytes == 1)
	{
		dest = vid.conbuffer;

		for (y=0 ; y<lines ; y++, dest += vid.conrowbytes)
		{
			// mankrip - hi-res console background - edited - begin
			v = (vid.conheight - lines + y)*conback->height/vid.conheight;
			src = conback->data + v*conback->width;

			if (con_alpha.value < 0.5f && !con_forcedup)
				Draw_ConsoleBackgroundLineStretched_BlendBackwards (src, dest, fstep);
			else if (con_alpha.value < 1.0f && !con_forcedup)
				Draw_ConsoleBackgroundLineStretched_Blend (src, dest, fstep);
			else if (vid.conwidth == conback->width)
			// mankrip - hi-res console background - edited - end
				memcpy (dest, src, vid.conwidth);
			else
			{
				f = 0;
				for (x=0 ; x<vid.conwidth ; x+=4)
				{
					dest[x  ] = src[f>>16]; f += fstep;
					dest[x+1] = src[f>>16]; f += fstep;
					dest[x+2] = src[f>>16]; f += fstep;
					dest[x+3] = src[f>>16]; f += fstep;
				}
			}
		}
	}
	else
	{
		pusdest = (unsigned short *)vid.conbuffer;

		for (y=0 ; y<lines ; y++, pusdest += (vid.conrowbytes >> 1))
		{
		// FIXME: pre-expand to native format?
		// FIXME: does the endian switching go away in production?
			v = (vid.conheight - lines + y)*conback->height/vid.conheight; // mankrip - hi-res console background - edited
			src = conback->data + v*conback->width; // mankrip - hi-res console background - edited
			f = 0;
		//	fstep = 320*0x10000/vid.conwidth; // removed
			for (x=0 ; x<vid.conwidth ; x+=4)
			{
				pusdest[x] = d_8to16table[src[f>>16]];
				f += fstep;
				pusdest[x+1] = d_8to16table[src[f>>16]];
				f += fstep;
				pusdest[x+2] = d_8to16table[src[f>>16]];
				f += fstep;
				pusdest[x+3] = d_8to16table[src[f>>16]];
				f += fstep;
			}
		}
	}
}
/* // mankrip
//	char			ver[100]; // removed
// hack the version number directly into the pic
#ifdef _WIN32
	sprintf (ver, "(WinQuake) %4.2f", (float)VERSION);
	dest = conback->data + 320*186 + 320 - 11 - 8*strlen(ver);
#elif defined(X11)
	sprintf (ver, "(X11 Quake %2.2f) %4.2f", (float)X11_VERSION, (float)VERSION);
	dest = conback->data + 320*186 + 320 - 11 - 8*strlen(ver);
#elif defined(__linux__)
	sprintf (ver, "(Linux Quake %2.2f) %4.2f", (float)LINUX_VERSION, (float)VERSION);
	dest = conback->data + 320*186 + 320 - 11 - 8*strlen(ver);
#else
	dest = conback->data + 320 - 43 + 320*186;
	sprintf (ver, "%4.2f", VERSION);
#endif

	for (x=0 ; x<strlen(ver) ; x++)
		Draw_CharToConback (ver[x], dest+(x<<3));
*/ // mankrip
#if 0
float ordered_dither_threshold [2][4][4] =
{
	{	//		positive,		negative,		positive,		negative
		{ 1.0f / 16.0f,  9.0f / -16.0f,  3.0f / 16.0f, 11.0f / -16.0f}
	,	{13.0f / 16.0f,  5.0f / -16.0f, 15.0f / 16.0f,  7.0f / -16.0f}
	,	{ 4.0f / 16.0f, 12.0f / -16.0f,  2.0f / 16.0f, 10.0f / -16.0f}
	,	{16.0f / 16.0f,  8.0f / -16.0f, 14.0f / 16.0f,  6.0f / -16.0f}
	}
,	{
		{ 1.0f /  16.0f,  9.0f /  16.0f,  3.0f /  16.0f, 11.0f /  16.0f}
	,	{13.0f / -16.0f,  5.0f / -16.0f, 15.0f / -16.0f,  7.0f / -16.0f} // negative
	,	{ 4.0f /  16.0f, 12.0f /  16.0f,  2.0f /  16.0f, 10.0f /  16.0f}
	,	{16.0f / -16.0f,  8.0f / -16.0f, 14.0f / -16.0f,  6.0f / -16.0f} // negative
	}
};
#if 1
#define EXTRA_ORDERED_DITHER
#endif
void Draw2D_Stretched (byte * src, int srcwidth, int srcheight, int x, int y, int w, int h, int offsetx, int offsety, float scalex, float scaley, byte alpha)
{
	// texture offsets are relative to the destination, not to the source
	if (alpha > 0 || con_forcedup) // mankrip - transparent console
	{
		float
			ustep = 1.0f / scalex
		,	vstep = 1.0f / scaley
		#ifdef EXTRA_ORDERED_DITHER
			// why 0.2? this algorithm isn't stable, it's too smooth at 1280x960 and above
		,	uscale = (((float) w / (float) srcwidth ) - 1.0f) * 0.25f //0.5f
		,	vscale = (((float) h / (float) srcheight) - 1.0f) * 0.25f //0.5f
		#endif
		,	u = ustep * (float) offsetx
		,	v = vstep * (float) offsety
		,	threshold
		,	ustep02 = ustep *  2.0f
		,	ustep03 = ustep *  3.0f
		,	ustep04 = ustep *  4.0f
		,	ustep05 = ustep *  5.0f
		,	ustep06 = ustep *  6.0f
		,	ustep07 = ustep *  7.0f
		,	ustep08 = ustep *  8.0f
		,	ustep09 = ustep *  9.0f
		,	ustep10 = ustep * 10.0f
		,	ustep11 = ustep * 11.0f
		,	ustep12 = ustep * 12.0f
		,	ustep13 = ustep * 13.0f
		,	ustep14 = ustep * 14.0f
		,	ustep15 = ustep * 15.0f
		,	ustep16 = ustep * 16.0f
		,	* dither_threshold_x
		,	* dither_threshold_y
			;
		int
			ditherwidth = w - (int) (1.0f / ustep)
		,	groupwidth = ditherwidth / ( (developer.value == 2.0f) ? 16 : 1) // TO DO: remove developer check
		,	ditherheight = h - (int) (1.0f / vstep)
			;
		if (uscale < 0.0f) uscale = 0.0f;
		if (vscale < 0.0f) vscale = 0.0f;
/*
		float
			xremainder = scalex - (float) floor ( (double) scalex) // can be zero!
		,	xremaindermultiplier = (xremainder) ? (1.0f / xremainder) : 1.0f
		,	yremainder = scaley - (float) floor ( (double) scaley) // can be zero!
		,	yremaindermultiplier = (yremainder) ? (1.0f / yremainder) : 1.0f
			;
		int
			matrixscalex = (int) (xremaindermultiplier * scalex * 4.0f)
		,	matrixscaley = (int) (yremaindermultiplier * scaley * 4.0f)
			;

		int dithermatrix[matrixscalex][matrixscaley];
*/
		if (r_pixbytes == 1)
		{
			byte
				* dest = vid.conbuffer
				;
			#if 0
			if (alpha < 128 && !con_forcedup)
				for ( ; y < lines ; y++, dest += vid.conrowbytes, v += vstep)
				{
					src = conback->data + v * srcwidth;
					for (f = 0, x = 0 ; x < vid.conwidth ; x++, f += fstep)
					{
						dest[x] = colorblendingmap[src[f >> 16] + dest[x] * 256];
					}
				}
			else if (alpha < 255 && !con_forcedup)
				for ( ; y < lines ; y++, dest += vid.conrowbytes, v += vstep)
				{
					src = conback->data + v * srcwidth;
					for (f = 0, x = 0 ; x < vid.conwidth ; x++, f += fstep)
					{
						dest[x] = colorblendingmap[dest[x] + src[f >> 16] * 256];
					}
				}
			else
			#endif
			{
				#ifdef EXTRA_ORDERED_DITHERzzzzzzz
				for ( ; y < ditherheight ; y++, v += vstep, dest += vid.conrowbytes, uscale *= -1.0f, vscale = 0.5f) // TO DO: replace vid.conrowbytes
				#else
				for ( ; y < ditherheight ; y++, v += vstep, dest += vid.conrowbytes) // TO DO: replace vid.conrowbytes
				#endif
					if (scalex == 1.0f && scaley == 1.0f)
						memcpy (dest, src + (int)v * srcwidth, w);
					else
					{
						dither_threshold_x = ordered_dither_threshold[0][y % 4];
						dither_threshold_y = ordered_dither_threshold[1][y % 4];
						for (u = 0, x = 0 ; x < groupwidth ; x += 16, u += ustep16)
						{
							#ifdef EXTRA_ORDERED_DITHER
							dest[  x      ] = src[ (int) (v + *dither_threshold_x	 * vscale) * srcwidth + (int) (u           + *dither_threshold_y    * uscale)];
							dest[ (x +  1)] = src[ (int) (v +  dither_threshold_x[1] * vscale) * srcwidth + (int) (u + ustep   +  dither_threshold_y[1] * uscale)];
							dest[ (x +  2)] = src[ (int) (v +  dither_threshold_x[2] * vscale) * srcwidth + (int) (u + ustep02 +  dither_threshold_y[2] * uscale)];
							dest[ (x +  3)] = src[ (int) (v +  dither_threshold_x[3] * vscale) * srcwidth + (int) (u + ustep03 +  dither_threshold_y[3] * uscale)];

							dest[ (x +  4)] = src[ (int) (v + *dither_threshold_x	 * vscale) * srcwidth + (int) (u + ustep04 + *dither_threshold_y    * uscale)];
							dest[ (x +  5)] = src[ (int) (v +  dither_threshold_x[1] * vscale) * srcwidth + (int) (u + ustep05 +  dither_threshold_y[1] * uscale)];
							dest[ (x +  6)] = src[ (int) (v +  dither_threshold_x[2] * vscale) * srcwidth + (int) (u + ustep06 +  dither_threshold_y[2] * uscale)];
							dest[ (x +  7)] = src[ (int) (v +  dither_threshold_x[3] * vscale) * srcwidth + (int) (u + ustep07 +  dither_threshold_y[3] * uscale)];

							dest[ (x +  8)] = src[ (int) (v + *dither_threshold_x	 * vscale) * srcwidth + (int) (u + ustep08 + *dither_threshold_y    * uscale)];
							dest[ (x +  9)] = src[ (int) (v +  dither_threshold_x[1] * vscale) * srcwidth + (int) (u + ustep09 +  dither_threshold_y[1] * uscale)];
							dest[ (x + 10)] = src[ (int) (v +  dither_threshold_x[2] * vscale) * srcwidth + (int) (u + ustep10 +  dither_threshold_y[2] * uscale)];
							dest[ (x + 11)] = src[ (int) (v +  dither_threshold_x[3] * vscale) * srcwidth + (int) (u + ustep11 +  dither_threshold_y[3] * uscale)];

							dest[ (x + 12)] = src[ (int) (v + *dither_threshold_x	 * vscale) * srcwidth + (int) (u + ustep12 + *dither_threshold_y    * uscale)];
							dest[ (x + 13)] = src[ (int) (v +  dither_threshold_x[1] * vscale) * srcwidth + (int) (u + ustep13 +  dither_threshold_y[1] * uscale)];
							dest[ (x + 14)] = src[ (int) (v +  dither_threshold_x[2] * vscale) * srcwidth + (int) (u + ustep14 +  dither_threshold_y[2] * uscale)];
							dest[ (x + 15)] = src[ (int) (v +  dither_threshold_x[3] * vscale) * srcwidth + (int) (u + ustep15 +  dither_threshold_y[3] * uscale)];

							/*
							threshold =  dither_threshold[2]; dest[ (x +  2)] = src[ (int) (v + threshold *  0.5f) * srcwidth + (int) (u + ustep02 + threshold * uscale)];
							threshold =  dither_threshold[3]; dest[ (x +  3)] = src[ (int) (v + threshold * -0.5f) * srcwidth + (int) (u + ustep03 + threshold * uscale)];
							threshold = *dither_threshold	; dest[ (x +  4)] = src[ (int) (v + threshold *  0.5f) * srcwidth + (int) (u + ustep04 + threshold * uscale)];
							threshold =  dither_threshold[1]; dest[ (x +  5)] = src[ (int) (v + threshold * -0.5f) * srcwidth + (int) (u + ustep05 + threshold * uscale)];
							threshold =  dither_threshold[2]; dest[ (x +  6)] = src[ (int) (v + threshold *  0.5f) * srcwidth + (int) (u + ustep06 + threshold * uscale)];
							threshold =  dither_threshold[3]; dest[ (x +  7)] = src[ (int) (v + threshold * -0.5f) * srcwidth + (int) (u + ustep07 + threshold * uscale)];
							threshold = *dither_threshold	; dest[ (x +  8)] = src[ (int) (v + threshold *  0.5f) * srcwidth + (int) (u + ustep08 + threshold * uscale)];
							threshold =  dither_threshold[1]; dest[ (x +  9)] = src[ (int) (v + threshold * -0.5f) * srcwidth + (int) (u + ustep09 + threshold * uscale)];
							threshold =  dither_threshold[2]; dest[ (x + 10)] = src[ (int) (v + threshold *  0.5f) * srcwidth + (int) (u + ustep10 + threshold * uscale)];
							threshold =  dither_threshold[3]; dest[ (x + 11)] = src[ (int) (v + threshold * -0.5f) * srcwidth + (int) (u + ustep11 + threshold * uscale)];
							threshold = *dither_threshold	; dest[ (x + 12)] = src[ (int) (v + threshold *  0.5f) * srcwidth + (int) (u + ustep12 + threshold * uscale)];
							threshold =  dither_threshold[1]; dest[ (x + 13)] = src[ (int) (v + threshold * -0.5f) * srcwidth + (int) (u + ustep13 + threshold * uscale)];
							threshold =  dither_threshold[2]; dest[ (x + 14)] = src[ (int) (v + threshold *  0.5f) * srcwidth + (int) (u + ustep14 + threshold * uscale)];
							threshold =  dither_threshold[3]; dest[ (x + 15)] = src[ (int) (v + threshold * -0.5f) * srcwidth + (int) (u + ustep15 + threshold * uscale)];
							*/
							#else
							threshold = dither_threshold[  x       % 4]; dest[  x      ] = src[ (int) (v + threshold) * srcwidth + (int) (u           + threshold)];
							threshold = dither_threshold[ (x +  1) % 4]; dest[ (x +  1)] = src[ (int) (v + threshold) * srcwidth + (int) (u + ustep   + threshold)];
							threshold = dither_threshold[ (x +  2) % 4]; dest[ (x +  2)] = src[ (int) (v + threshold) * srcwidth + (int) (u + ustep02 + threshold)];
							threshold = dither_threshold[ (x +  3) % 4]; dest[ (x +  3)] = src[ (int) (v + threshold) * srcwidth + (int) (u + ustep03 + threshold)];
							threshold = dither_threshold[ (x +  4) % 4]; dest[ (x +  4)] = src[ (int) (v + threshold) * srcwidth + (int) (u + ustep04 + threshold)];
							threshold = dither_threshold[ (x +  5) % 4]; dest[ (x +  5)] = src[ (int) (v + threshold) * srcwidth + (int) (u + ustep05 + threshold)];
							threshold = dither_threshold[ (x +  6) % 4]; dest[ (x +  6)] = src[ (int) (v + threshold) * srcwidth + (int) (u + ustep06 + threshold)];
							threshold = dither_threshold[ (x +  7) % 4]; dest[ (x +  7)] = src[ (int) (v + threshold) * srcwidth + (int) (u + ustep07 + threshold)];
							threshold = dither_threshold[ (x +  8) % 4]; dest[ (x +  8)] = src[ (int) (v + threshold) * srcwidth + (int) (u + ustep08 + threshold)];
							threshold = dither_threshold[ (x +  9) % 4]; dest[ (x +  9)] = src[ (int) (v + threshold) * srcwidth + (int) (u + ustep09 + threshold)];
							threshold = dither_threshold[ (x + 10) % 4]; dest[ (x + 10)] = src[ (int) (v + threshold) * srcwidth + (int) (u + ustep10 + threshold)];
							threshold = dither_threshold[ (x + 11) % 4]; dest[ (x + 11)] = src[ (int) (v + threshold) * srcwidth + (int) (u + ustep11 + threshold)];
							threshold = dither_threshold[ (x + 12) % 4]; dest[ (x + 12)] = src[ (int) (v + threshold) * srcwidth + (int) (u + ustep12 + threshold)];
							threshold = dither_threshold[ (x + 13) % 4]; dest[ (x + 13)] = src[ (int) (v + threshold) * srcwidth + (int) (u + ustep13 + threshold)];
							threshold = dither_threshold[ (x + 14) % 4]; dest[ (x + 14)] = src[ (int) (v + threshold) * srcwidth + (int) (u + ustep14 + threshold)];
							threshold = dither_threshold[ (x + 15) % 4]; dest[ (x + 15)] = src[ (int) (v + threshold) * srcwidth + (int) (u + ustep15 + threshold)];
							#endif
						}
						#ifdef EXTRA_ORDERED_DITHERzzzzzzzzzz
						for ( ; x < ditherwidth ; x++, u += ustep, vscale *= -1.0f)
						#else
						for ( ; x < ditherwidth ; x++, u += ustep)
						#endif
						{
						//	threshold = dither_threshold[x % 4];
							dest[x] = src[
							#ifdef EXTRA_ORDERED_DITHERzzzzzzzzz
							// maybe for scale > 2.0f
								(int) (v + threshold * vscale) * srcwidth // change to below for horizontal smoothness
							+	(int) (u + threshold * uscale) // change to below for vertical smoothness
							#else
								(int) (v + dither_threshold_x[x % 4]) * srcwidth
							+	(int) (u + dither_threshold_y[x % 4])
							#endif
							];
						}
						#ifdef EXTRA_ORDERED_DITHERzzzzzzzzz
						for ( ; x < w ; x++, u += ustep, vscale *= -1.0f)
						#else
						for ( ; x < w ; x++, u += ustep)
						#endif
							dest[x] = src[
								(int) (v + dither_threshold_x[x % 4]) * srcwidth
							+	(int) u
							];
					}
				#ifdef EXTRA_ORDERED_DITHERzzzzzzzzz
				for ( ; y < h ; y++, v += vstep, dest += vid.conrowbytes, uscale *= -1.0f) // TO DO: replace vid.conrowbytes
				#else
				for ( ; y < h ; y++, v += vstep, dest += vid.conrowbytes) // TO DO: replace vid.conrowbytes
				#endif
					if (scalex == 1.0f && scaley == 1.0f)
						memcpy (dest, src + (int)v * srcwidth, w);
					else
					{
						dither_threshold_y = ordered_dither_threshold[1][y % 4];
						#ifdef EXTRA_ORDERED_DITHERzzzzzzzzzzz
						for (u = 0, x = 0 ; x < ditherwidth ; x++, u += ustep, vscale *= -1.0f)
						#else
						for (u = 0, x = 0 ; x < ditherwidth ; x++, u += ustep)
						#endif
							dest[x] = src[
								(int) v * srcwidth
							+	(int) (u + dither_threshold_y[x % 4])
							];
						#ifdef EXTRA_ORDERED_DITHERzzzzzzzz
						for ( ; x < w ; x++, u += ustep, vscale *= -1.0f)
						#else
						for ( ; x < w ; x++, u += ustep)
						#endif
							dest[x] = src[
								(int) v * srcwidth
							+	(int) u
							];
					}
			}
		}
		#if 0
		else
		{
			int
				f
			,	fstep = conback->width * 0x10000 / vid.conwidth
				;
			unsigned short
				* pusdest = (unsigned short *)vid.conbuffer
				;
			for (y=0 ; y<lines ; y++, pusdest += (vid.conrowbytes >> 1))
			{
				// FIXME: pre-expand to native format?
				// FIXME: does the endian switching go away in production?
				v = (vid.conheight - lines + y)*conback->height/vid.conheight; // mankrip - hi-res console background - edited
				src = conback->data + (int) v * conback->width; // mankrip - hi-res console background - edited
				for (f = 0, x = 0 ; x < vid.conwidth ; x+=4)
				{
					pusdest[x  ] = d_8to16table[src[f>>16]];
					f += fstep;
					pusdest[x+1] = d_8to16table[src[f>>16]];
					f += fstep;
					pusdest[x+2] = d_8to16table[src[f>>16]];
					f += fstep;
					pusdest[x+3] = d_8to16table[src[f>>16]];
					f += fstep;
				}
			}
		}
		#endif
	}
}
#endif
void Draw_ConsoleBackground (int lines)
{
	#if 1
	Old_Draw_ConsoleBackground (lines);
	#else
	if (developer.value == 1.0f)
		Old_Draw_ConsoleBackground (lines);
	else
	{
		extern cvar_t
			con_alpha
			;
		qpic_t
			* conback = Draw_CachePic ("gfx/conback.lmp")
			;
		if (conback)
			Draw2D_Stretched (conback->data, conback->width, conback->height
			, 0, 0, vid.conwidth, lines, 0, vid.conheight - lines, (float)vid.conwidth / (float)conback->width, (float)vid.conheight / (float)conback->height, (byte) (con_alpha.value * 255.0f));
	}
	#endif
}


/*
==============
R_DrawRect8
==============
*/
void R_DrawRect8 (vrect_t *prect, int rowbytes, byte *psrc,
	int transparent)
{
	byte	t;
	int		i, j, srcdelta, destdelta;
	byte	*pdest;

	pdest = vid.buffer + (prect->y * vid.rowbytes) + prect->x;

	srcdelta = rowbytes - prect->width;
	destdelta = vid.rowbytes - prect->width;

	if (transparent)
	{
		for (i=0 ; i<prect->height ; i++)
		{
			for (j=0 ; j<prect->width ; j++)
			{
				t = *psrc;
				if (t != TRANSPARENT_COLOR)
				{
					*pdest = t;
				}

				psrc++;
				pdest++;
			}

			psrc += srcdelta;
			pdest += destdelta;
		}
	}
	else
	{
		for (i=0 ; i<prect->height ; i++)
		{
			memcpy (pdest, psrc, prect->width);
			psrc += rowbytes;
			pdest += vid.rowbytes;
		}
	}
}


/*
==============
R_DrawRect16
==============
*/
void R_DrawRect16 (vrect_t *prect, int rowbytes, byte *psrc,
	int transparent)
{
	byte			t;
	int				i, j, srcdelta, destdelta;
	unsigned short	*pdest;

// FIXME: would it be better to pre-expand native-format versions?

	pdest = (unsigned short *)vid.buffer +
			(prect->y * (vid.rowbytes >> 1)) + prect->x;

	srcdelta = rowbytes - prect->width;
	destdelta = (vid.rowbytes >> 1) - prect->width;

	if (transparent)
	{
		for (i=0 ; i<prect->height ; i++)
		{
			for (j=0 ; j<prect->width ; j++)
			{
				t = *psrc;
				if (t != TRANSPARENT_COLOR)
				{
					*pdest = d_8to16table[t];
				}

				psrc++;
				pdest++;
			}

			psrc += srcdelta;
			pdest += destdelta;
		}
	}
	else
	{
		for (i=0 ; i<prect->height ; i++)
		{
			for (j=0 ; j<prect->width ; j++)
			{
				*pdest = d_8to16table[*psrc];
				psrc++;
				pdest++;
			}

			psrc += srcdelta;
			pdest += destdelta;
		}
	}
}


/*
=============
Draw_TileClear

This repeats a 64*64 tile graphic to fill the screen around a sized down
refresh window.
=============
*/
void Draw_TileClear (int x, int y, int w, int h)
{
	int				width, height, tileoffsetx, tileoffsety;
	byte			*psrc;
	vrect_t			vr;

	r_rectdesc.rect.x = x;
	r_rectdesc.rect.y = y;
	r_rectdesc.rect.width = w;
	r_rectdesc.rect.height = h;

	vr.y = r_rectdesc.rect.y;
	height = r_rectdesc.rect.height;

	tileoffsety = vr.y % r_rectdesc.height;

	while (height > 0)
	{
		vr.x = r_rectdesc.rect.x;
		width = r_rectdesc.rect.width;

		if (tileoffsety != 0)
			vr.height = r_rectdesc.height - tileoffsety;
		else
			vr.height = r_rectdesc.height;

		if (vr.height > height)
			vr.height = height;

		tileoffsetx = vr.x % r_rectdesc.width;

		while (width > 0)
		{
			if (tileoffsetx != 0)
				vr.width = r_rectdesc.width - tileoffsetx;
			else
				vr.width = r_rectdesc.width;

			if (vr.width > width)
				vr.width = width;

			psrc = r_rectdesc.ptexbytes +
					(tileoffsety * r_rectdesc.rowbytes) + tileoffsetx;

			if (r_pixbytes == 1)
			{
				R_DrawRect8 (&vr, r_rectdesc.rowbytes, psrc, 0);
			}
			else
			{
				R_DrawRect16 (&vr, r_rectdesc.rowbytes, psrc, 0);
			}

			vr.x += vr.width;
			width -= vr.width;
			tileoffsetx = 0;	// only the left tile can be left-clipped
		}

		vr.y += vr.height;
		height -= vr.height;
		tileoffsety = 0;		// only the top tile can be top-clipped
	}
}
//=============================================================================

//=============================================================================

/*
================
Draw_BeginDisc

Draws the little blue disc in the corner of the screen.
Call before beginning any disc IO.
================
*/
void Draw_BeginDisc (void)
{

	D_BeginDirectRect (vid.width - 24, 0, draw_disc->data, 24, 24);
}


/*
================
Draw_EndDisc

Erases the disc icon.
Call after completing any disc IO
================
*/
void Draw_EndDisc (void)
{

	D_EndDirectRect (vid.width - 24, 0, 24, 24);
}



//=============================================================================

//								PRIMITIVES

//=============================================================================



/*
=============
Draw_Fill

Fills a box of pixels with a single color
=============
*/
void Draw_Fill (int x, int y, int w, int h, byte c)
{
	// mankrip - begin
	// return if nothing to draw
	if (! (w && h))
		return;
	// return if offscreen
	if (x >= vid.width)
		return;
	if (x + w <= 0)
		return;

	if (y >= vid.height)
		return;
	if (y + h <= 0)
		return;
	// adjust for negative dimensions
	if (w < 0)
	{
		x += w;
		w = -w;
	}
	if (h < 0)
	{
		y += h;
		h = -h;
	}
	// crop if partially offscreen
	if (x < 0)
	{
		w += x;
		x = 0;
	}
	if (x + w > vid.width)
		w = vid.width - x;

	if (y < 0)
	{
		h += y;
		y = 0;
	}
	if (y + h > vid.height)
		h = vid.height - y;
	// mankrip - end

	if (r_pixbytes == 1)
	{
		// mankrip - begin
		byte
			* dest = vid.buffer + y * vid.rowbytes + x
			;
		int
			count = w >> 4
		,	spancount = w % 16
		,	rowpadding = vid.rowbytes - w
		,	u
			;

		if (spancount)
		{
			for ( ; h ; h--, dest += rowpadding)
			{
				for (u = count ; u ; u--)
				{
					dest += 16;
					dest[-16] = c;
					dest[-15] = c;
					dest[-14] = c;
					dest[-13] = c;
					dest[-12] = c;
					dest[-11] = c;
					dest[-10] = c;
					dest[ -9] = c;
					dest[ -8] = c;
					dest[ -7] = c;
					dest[ -6] = c;
					dest[ -5] = c;
					dest[ -4] = c;
					dest[ -3] = c;
					dest[ -2] = c;
					dest[ -1] = c;
				}
				dest += spancount;
				switch (spancount)
				{
					case 16: dest[-16] = c;
					case 15: dest[-15] = c;
					case 14: dest[-14] = c;
					case 13: dest[-13] = c;
					case 12: dest[-12] = c;
					case 11: dest[-11] = c;
					case 10: dest[-10] = c;
					case  9: dest[ -9] = c;
					case  8: dest[ -8] = c;
					case  7: dest[ -7] = c;
					case  6: dest[ -6] = c;
					case  5: dest[ -5] = c;
					case  4: dest[ -4] = c;
					case  3: dest[ -3] = c;
					case  2: dest[ -2] = c;
					case  1: dest[ -1] = c;
					break;
				}
			}
		}
		else
		{
			for ( ; h ; h--, dest += rowpadding)
				for (u = count ; u ; u--)
				{
					dest += 16;
					dest[-16] = c;
					dest[-15] = c;
					dest[-14] = c;
					dest[-13] = c;
					dest[-12] = c;
					dest[-11] = c;
					dest[-10] = c;
					dest[ -9] = c;
					dest[ -8] = c;
					dest[ -7] = c;
					dest[ -6] = c;
					dest[ -5] = c;
					dest[ -4] = c;
					dest[ -3] = c;
					dest[ -2] = c;
					dest[ -1] = c;
				}
		}
		// mankrip - end
	}
	else
	{
		int				u, v;
		unsigned short	*pusdest;
		unsigned		uc;
		uc = d_8to16table[c];

		pusdest = (unsigned short *)vid.buffer + y * (vid.rowbytes >> 1) + x;
		for (v=0 ; v<h ; v++, pusdest += (vid.rowbytes >> 1))
			for (u=0 ; u<w ; u++)
				pusdest[u] = uc;
	}
}

/*
================
Draw_FadeScreen

================
*/
void Draw_FadeScreen (void)
{
	#if 0
	int level = 20;
	#endif
//	byte *alphamap = mapForAlpha (int BLEND_WEIGHTEDMEAN, float alpha);
	// mankrip - begin
	if (scr_fading.value < 0.0)
		return;
	else
	{
	// mankrip - end
		int			x,y;
		byte		*pbuf;

		VID_UnlockBuffer ();
		S_ExtraUpdate ();
		VID_LockBuffer ();

		// mankrip - begin
		#if 0
		#if 0
		if (scr_fading.value >= 8.0 && scr_fading.value < 14.0) // backwards range
		{
			byte scolor = (byte) (scr_fading.value * 16.0);
			for (y=0 ; y<vid.height ; y++)
			{
				pbuf = (byte *)(vid.buffer + vid.rowbytes*y);
				for (x=0 ; x<vid.width ; x++, pbuf++)
					*pbuf = scolor + (byte) ( (*pbuf>127 && *pbuf<224) ? (*pbuf%16) : (15 - *pbuf%16) );
			}
		}
		else
		#endif
		if (scr_fading.value < 16.0)
		{
			byte scolor = (byte) scr_fading.value * 16;
			for (y=0 ; y<vid.height ; y++)
			{
				pbuf = (byte *)(vid.buffer + vid.rowbytes*y);
				for (x=0 ; x<vid.width ; x++, pbuf++)
				//	*pbuf = scolor + (byte) ( (*pbuf>127 && *pbuf<224) ? (15 - *pbuf%16) : (*pbuf%16) );
					*pbuf = scolor + *pbuf%16;
			}
		}
		// use alphamap instead of vid.colormap, to ensure the fullbright colors will be darkened
		else if (scr_fading.value < 17.0f)
		{
			for (y=0 ; y<vid.height ; y++)
			{
				pbuf = (byte *)(vid.buffer + vid.rowbytes*y);
				for (x=0 ; x<vid.width ; x++, pbuf++)
					*pbuf = colorblendingmap[*pbuf];
			}
		// mankrip - end
			#if 0
			for (y=0 ; y<vid.height ; y++)
			{
				pbuf = (byte *)(vid.buffer + vid.rowbytes*y);
				for (x=0 ; x<vid.width ; x++, pbuf++)
					*pbuf = (byte)vid.colormap[*pbuf + ((32 + level) * 256)]; // edited
			}
			#endif
		} // mankrip
		else //if (scr_fading.value < 18.0f) // mankrip
			// stippled
			for (y=0 ; y<vid.height ; y++)
			{
				int	t;
				pbuf = (byte *)(vid.buffer + vid.rowbytes*y);
				t = (y & 1) << 1;
				for (x=0 ; x<vid.width ; x++)
				{
					if ((x & 3) != t)
						pbuf[x] = 0;
				}
			}
		#else
		for (y=0 ; y<vid.height ; y++)
		{
			pbuf = (byte *)(vid.buffer + vid.rowbytes*y);
			for (x=0 ; x<vid.width ; x++, pbuf++)
				*pbuf = tintTable[*pbuf];
		}
		#endif

		VID_UnlockBuffer ();
		S_ExtraUpdate ();
		VID_LockBuffer ();
	}
}
