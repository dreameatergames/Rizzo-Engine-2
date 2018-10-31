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
// r_shared.h: general refresh-related stuff shared between the refresh and the driver

#ifndef GLQUAKE
//#include "d_iface.h"
//#include "render.h"

// FIXME: clean up and move into d_iface.h

#ifndef _R_SHARED_H_
#define _R_SHARED_H_

#define	MAXVERTS	16					// max points in a surface polygon
#define MAXWORKINGVERTS	(MAXVERTS+4)	// max points in an intermediate polygon (while processing)

// !!! if this is changed, it must be changed in d_ifacea.h too !!!
// mankrip - begin
#ifdef _arch_dreamcast
	#define	MAXHEIGHT	608
	#define	MAXWIDTH	800
#else
	#define	MAXHEIGHT	1536 //1024
	#define	MAXWIDTH	2048 //1280
#endif
// mankrip - end
#define MAXDIMENSION	((MAXHEIGHT > MAXWIDTH) ? MAXHEIGHT : MAXWIDTH)

// !!! if this is changed, it must be changed in d_ifacea.h too !!!
#define	CYCLE			128		// turbulent cycle size

#define SIN_BUFFER_SIZE	(MAXDIMENSION+CYCLE) // CYCLE defined in d_iface.h

#define INFINITE_DISTANCE	0x10000		// distance that's always guaranteed to be farther away than anything in the scene

#define NUMSTACKEDGES		2400
#define	MINEDGES			NUMSTACKEDGES
#define NUMSTACKSURFACES	800
#define MINSURFACES			NUMSTACKSURFACES
#define	MAXSPANS			3000

//===================================================================

extern int
	cachewidth
,	screenwidth
,	r_drawnpolycount
,	sintable[SIN_BUFFER_SIZE]
,	intsintable[SIN_BUFFER_SIZE]
	;
extern	float
	pixelAspect
	;
extern cvar_t
	r_clearcolor
	;
extern pixel_t
	* cacheblock
	;
extern	vec3_t
	vup
,	base_vup
,	vpn
,	base_vpn
,	vright
,	base_vright
	;

// surfaces are generated in back to front order by the bsp, so if a surf
// pointer is greater than another one, it should be drawn in front
// surfaces[1] is the background, and is used as the active surface stack.
// surfaces[0] is a dummy, because index 0 is used to indicate no surface attached to an edge_t

//===================================================================

extern vec3_t
	sxformaxis[4]	// s axis transformed into viewspace
,	txformaxis[4]	// t axis transformed into viewspac
,	modelorg
,	base_modelorg
	;
extern	float
	xcenter
,	ycenter
,	xscale
,	yscale
,	xscaleinv
,	yscaleinv
,	xscaleshrink
,	yscaleshrink
	;
extern void
	TransformVector (vec3_t in, vec3_t out)
,	SetUpForLineScan (fixed8_t startvertu, fixed8_t startvertv, fixed8_t endvertu, fixed8_t endvertv)
	;
extern int
	ubasestep
,	errorterm
,	erroradjustup
,	erroradjustdown
,	d_lightstylevalue[256] // 8.8 frac of base light value
	;

#endif	// _R_SHARED_H_

#endif	// GLQUAKE
