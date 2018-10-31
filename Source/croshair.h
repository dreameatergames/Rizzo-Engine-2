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

#ifndef _CROSSHAIR_H_
#define _CROSSHAIR_H_

extern cvar_t
	crosshair
,	cl_crossx
,	cl_crossy
	;
extern void
	Crosshair_Boot (void)
,	Crosshair_Draw (int x, int y, byte *translation) // draw the crosshair at a specific position on the screen
,	Crosshair_Start (void) // draw the crosshair at the center of the screen
	;

#endif
