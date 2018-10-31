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

#ifndef __TEX_FILE__
#define __TEX_FILE__

#ifdef INCLUDELIBS

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

#include "cmdlib.h"
#include "scriplib.h"
#include "trilib.h"
#include "lbmlib.h"
#include "mathlib.h"

#endif

#include "quakedef.h"

typedef struct ex_texture_s
{
	char
		name[16]
	,	path[256]//MAX_PATH]
		;
	unsigned
		width
	,	height
	,	offsets[MIPLEVELS]	// four mip maps stored
		;
	int
		anim_total			// total tenths in sequence ( 0 = no)
	,	anim_min
	,	anim_max			// time for this frame min <=time< max
		;
	struct texture_s
		*anim_next			// in the animation sequence
	,	*alternate_anims	// bmodels in frmae 1 use these
		;
	byte
		id_length
	,	colormap_type
	,	colormap_size
	,	image_type
	,	pixel_size
	,	attributes
	,	data[4]				// variably sized
		;
	unsigned short
		colormap_index
	,	colormap_length
	,	x_origin
	,	y_origin
//	,	width
//	,	height
		;
} ex_texture_t;

#endif
