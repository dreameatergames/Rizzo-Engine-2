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

#ifndef __MDL_SPRITE__
#define __MDL_SPRITE__

// modelgen.h: header file for model generation program

// *********************************************************
// * This file must be identical in the modelgen directory *
// * and in the Quake directory, because it's used to      *
// * pass data from one to the other via model files.      *
// *********************************************************

//	Alias models are position independent, so the cache manager can move them.

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

#define ALIAS_VERSION				6
#define ALIAS_ONSEAM				0x0020

// flags in finalvert_t.flags
#define ALIAS_LEFT_CLIP				0x0001
#define ALIAS_TOP_CLIP				0x0002
#define ALIAS_RIGHT_CLIP			0x0004
#define ALIAS_BOTTOM_CLIP			0x0008
#define ALIAS_Z_CLIP				0x0010
// !!! if this is changed, it must be changed in d_ifacea.h too !!!
#define ALIAS_ONSEAM				0x0020	// also defined in modelgen.h;
											//  must be kept in sync
#define ALIAS_XY_CLIP_MASK			0x000F

#define MAXALIASVERTS		2000	// TODO: tune this // 6000	mankrip - (un)edited
#define ALIAS_Z_CLIP_PLANE	0.5		// mankrip - edited
#define DT_FACES_FRONT				0x0010
#define IDPOLYHEADER	(('O'<<24)+('P'<<16)+('D'<<8)+'I') // little-endian "IDPO"

// must match definition in spritegn.h
#ifndef SYNCTYPE_T
#define SYNCTYPE_T
typedef enum
{
	ST_SYNC = 0
,	ST_RAND
} synctype_t;
#endif

typedef enum
{
	ALIAS_SINGLE = 0
,	ALIAS_GROUP
} aliasframetype_t;

typedef enum
{
	ALIAS_SKIN_SINGLE = 0
,	ALIAS_SKIN_GROUP
} aliasskintype_t;

typedef struct
{
	int
		ident
	,	version
		;
	vec3_t
		scale
	,	scale_origin
		;
	float
		boundingradius
		;
	vec3_t
		eyeposition
		;
	int
		numskins
	,	skinwidth
	,	skinheight
	,	numverts
	,	numtris
	,	numframes
		;
	synctype_t
		synctype
		;
	int
		flags
		;
	float
		size
		;
} mdl_t;

// TODO: could be shorts
typedef struct
{
	int
		onseam
	,	s
	,	t
		;
} stvert_t;

/*

d*_t structures are on-disk representations
m*_t structures are in-memory

*/

typedef struct dtriangle_s
{
	int
		facesfront
	,	vertindex[3]
		;
} dtriangle_t;

typedef struct
{
	// This mirrors trivert_t in trilib.h, is present so Quake knows how to load this data
	byte
		v[3]
	,	lightnormalindex
		;
} trivertx_t;

typedef struct
{
	trivertx_t
		bboxmin	// lightnormal isn't used
	,	bboxmax	// lightnormal isn't used
		;
	char
		name[16]	// frame name from grabbing
		;
} daliasframe_t;

typedef struct
{
	int
		numframes
		;
	trivertx_t
		bboxmin	// lightnormal isn't used
	,	bboxmax	// lightnormal isn't used
		;
} daliasgroup_t;

typedef struct
{
	int
		numskins
		;
} daliasskingroup_t;

typedef struct
{
	float
		interval
		;
} daliasinterval_t;

typedef struct
{
	float
		interval
		;
} daliasskininterval_t;

typedef struct
{
	aliasframetype_t
		type
		;
} daliasframetype_t;

typedef struct
{
	aliasskintype_t
		type
		;
} daliasskintype_t;

typedef struct
{
	aliasframetype_t
		type
		;
	trivertx_t
		bboxmin
	,	bboxmax
		;
	int
		frame
		;
	char
		name[16]
		;
} maliasframedesc_t;

typedef struct
{
	aliasskintype_t
		type
		;
	void
		*pcachespot
		;
	int
		skin
		;
} maliasskindesc_t;

typedef struct
{
	trivertx_t
		bboxmin
	,	bboxmax
		;
	int
		frame
	,	numframes
		;
} maliasgroupframedesc_t;

typedef struct maliasgroup_s
{
	int
		numframes
	,	intervals
		;
	maliasgroupframedesc_t
		frames[1]
		;
} maliasgroup_t;

typedef struct
{
	int
		numskins
	,	intervals
		;
	maliasskindesc_t
		skindescs[1]
		;
} maliasskingroup_t;

// !!! if this is changed, it must be changed in asm_draw.h too !!!
typedef struct mtriangle_s
{
	int
		facesfront
	,	vertindex[3]
		;
} mtriangle_t;

typedef struct
{
	int
		model
	,	stverts
	,	skindesc
	,	triangles
		;
	maliasframedesc_t
		frames[1]
		;
} aliashdr_t;

// !!! if this is changed, it must be changed in d_ifacea.h too !!!
typedef struct finalvert_s
{
	int
		v[6]	// u, v, s, t, l, 1/z
	,	flags
		;
	float
		reserved
	;
} finalvert_t;

// !!! if this is changed, it must be changed in d_ifacea.h too !!!
typedef struct
{
	void
		*pskin
		;
	maliasskindesc_t
		*pskindesc
		;
	int
		skinwidth
	,	skinheight
		;
	mtriangle_t
		*ptriangles
		;
	finalvert_t
		*pfinalverts
		;
	int
		numtriangles
	,	drawtype
	,	seamfixupX16
		;
} affinetridesc_t;

// !!! if this is changed, it must be changed in asm_draw.h too !!!
typedef struct
{
	void
		*pdest
		;
	short
		*pz
		;
	int
		count
		;
	byte
		*ptex
		;
	int
		sfrac
	,	tfrac
	,	light
	,	zi
		;
} spanpackage_t;

extern affinetridesc_t
	r_affinetridesc
	;
extern finalvert_t
	*pfinalverts
	;
extern mtriangle_t
	*ptriangles
	;
extern aliashdr_t
	*paliashdr
	;
extern mdl_t
	*pmdl
	;

extern int
	numverts
,	a_skinwidth
,	numtriangles
,	r_acliptype
,	r_drawoutlines
,	celtri_drawn
	;
extern float
	leftclip
,	topclip
,	rightclip
,	bottomclip
	;
extern qboolean
	R_AliasCheckBBox		(void)
	;
extern void
	R_AliasClipTriangle		(mtriangle_t *ptri)
,	D_PolysetDraw			(void)
,	D_PolysetDraw_C			(void)
,	D_PolysetDrawFinalVerts (finalvert_t *fv, int numverts)
,	D_PolysetFillSpans8					(void)
,	MDL_DrawSpans_FullOpaque		(void)
,	MDL_DrawSpans_ColorKeyed		(void)
,	MDL_DrawSpans_Metal			(void)
,	MDL_DrawSpans_Blend			(void)
,	MDL_DrawSpans_BlendBackwards	(void)
,	MDL_DrawSpans_Shadow			(void)
,	MDL_DrawSpans_Outline			(void)
	;
#endif
