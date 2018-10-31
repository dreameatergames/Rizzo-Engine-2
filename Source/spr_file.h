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
//
// spritegn.h: header file for sprite generation program
//

// **********************************************************
// * This file must be identical in the spritegen directory *
// * and in the Quake directory, because it's used to       *
// * pass data from one to the other via .spr files.        *
// **********************************************************

//-------------------------------------------------------
// This program generates .spr sprite package files.
// The format of the files is as follows:
//
// dsprite_t file header structure
// <repeat dsprite_t.numframes times>
//   <if spritegroup, repeat dspritegroup_t.numframes times>
//     dspriteframe_t frame header structure
//     sprite bitmap
//   <else (single sprite frame)>
//     dspriteframe_t frame header structure
//     sprite bitmap
// <endrepeat>
//-------------------------------------------------------

#ifdef INCLUDELIBS

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

#include "cmdlib.h"
#include "scriplib.h"
#include "dictlib.h"
#include "trilib.h"
#include "lbmlib.h"
#include "mathlib.h"

#endif

#define IDSPRITEHEADER	(('P'<<24)+('S'<<16)+('D'<<8)+'I') // little-endian "IDSP"

#define SPRITE_VERSION	1

#define SPR_VP_PARALLEL_UPRIGHT		0
#define SPR_FACING_UPRIGHT			1
#define SPR_VP_PARALLEL				2
#define SPR_ORIENTED				3
#define SPR_VP_PARALLEL_ORIENTED	4

// must match definition in modelgen.h
#ifndef SYNCTYPE_T
#define SYNCTYPE_T
typedef enum
{
	ST_SYNC = 0
,	ST_RAND
} synctype_t;
#endif

// TODO: shorten these?
typedef struct
{
	int
		ident
	,	version
	,	type
		;
	float
		boundingradius
		;
	int
		width
	,	height
	,	numframes
		;
	float
		beamlength
		;
	synctype_t
		synctype
		;
} dsprite_t;

typedef struct
{
	int
		origin[2]
	,	width
	,	height
		;
} dspriteframe_t;

typedef struct
{
	int
		numframes
		;
} dspritegroup_t;

typedef struct
{
	float
		interval
		;
} dspriteinterval_t;

typedef enum
{
	SPR_SINGLE = 0
,	SPR_GROUP
} spriteframetype_t;

typedef struct
{
	spriteframetype_t
		type
		;
} dspriteframetype_t;


/*
==============================================================================

SPRITE MODELS

==============================================================================
*/


// FIXME: shorten these?
typedef struct mspriteframe_s
{
	int
		width
	,	height
		;
	void
		*pcachespot // remove?
		;
	float
		up
	,	down
	,	left
	,	right
		;
	byte
		pixels[4]
		;
} mspriteframe_t;

typedef struct
{
	int
		numframes
		;
	float
		*intervals
		;
	mspriteframe_t
		*frames[1]
		;
} mspritegroup_t;

typedef struct
{
	spriteframetype_t
		type
		;
	mspriteframe_t
		*frameptr
		;
} mspriteframedesc_t;

typedef struct
{
	int
		type
	,	maxwidth
	,	maxheight
	,	numframes
		;
	float
		beamlength // remove?
		;
	void
		*cachespot // remove?
		;
	mspriteframedesc_t
		frames[1]
		;
} msprite_t;

typedef struct
{
	float
		u
	,	v
	,	s
	,	t
		;
	float
		zi
		;
} emitpoint_t;

typedef struct
{
	int
		nump
		;
	emitpoint_t
		// there's room for an extra element at [nump],
		// if the driver wants to duplicate element [0] at element [nump] to avoid dealing with wrapping
		*pverts
		;
	mspriteframe_t
		*pspriteframe
		;
	vec3_t
		// in worldspace
		vup
	,	vright
	,	vpn
		;
	float
		nearzi
		;
} spritedesc_t;

extern spritedesc_t
	r_spritedesc
	;

extern void
	Mod_LoadSpriteModel (model_t *mod, void *buffer)
,	D_SpriteDrawSpans16_ColorKeyed (void)
,	D_SpriteDrawSpans_Blend (void)
,	D_SpriteDrawSpans_BlendBackwards (void)
,	D_SpriteDrawSpans_Shadow (void)
//,	R_AddPolygonEdges (emitpoint_t *pverts, int numverts, int miplevel) // no ocurrences in the whole source?
	;
extern cvar_t
	r_sprite_addblend
,	r_sprite_lit
	;
