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
// d_part.c: software driver module for drawing particles

#include "quakedef.h"
#include "r_local.h" // mankrip
#include "ptc_file.h" // mankrip

// not used by software driver
int	d_pix_shift;

// mankrip - begin
typedef struct d_particle_s
{
	int
		h_radius
	,	v_radius
	,	vrectright
	,	vrectbottom
	,	zcenter
	,	lines
	,	*scanwidth
	,	*offset
		;
	particle_t
		*unit
		;
} d_particle_t;

typedef struct r_particles_s
{
	int
		max_size
	,	full_size
		;
	d_particle_t
		**for_size
		;
	float
		scale
		;
	double
		h_aspect
		;
} r_particles_t;

r_particles_t	r_particles;

qboolean		r_particles_init = false;

void D_Particles_Recalc (void)
{
	int
		p = 0
	,	max_size
	,	full_size
		;
	double
		h_aspect
	,	pix
	,	width
	,	height
	,	count
	,	offset
		;
	float
		sizelimit
		;

	r_particles.scale = fovscale * ( (float) r_refdef.vrect.height / 60.0f); // 30.0f = 240.0f / 8.0f

	h_aspect = 1.0 / (double) vid.aspect;

	// doubled the minimum size here to make sure the particle will appear,
	// since the width and height calculations reduces its drawing area
	sizelimit = minmaxclamp (0.02f, r_particle_sizelimit.value, 1.0f);
	full_size = min (vid.width, vid.height) + 2;
	max_size = (int)(0.5f + (float)full_size * sizelimit);

	// don't recalculate when only zooming in
	if (r_particles_init
	&&	r_particles.h_aspect	== h_aspect
	&&	r_particles.max_size	== max_size // >=
	&&	r_particles.full_size	== full_size // >=
		)
		return;
	// BUG: adding these Con_DPrintf fixed the problem with the above check being ignored. wtf?
	#ifndef _arch_dreamcast
//	Con_DPrintf ("r_particles.h_aspect %d\n", r_particles.h_aspect);
	Con_DPrintf ("h_aspect %d\n", h_aspect);
	#endif
	r_particles.h_aspect	= h_aspect;

	// free previously allocated memory before allocating
	if (r_particles_init)
	{
		for ( ; p <= r_particles.max_size ; p++)
		{
			free (r_particles.for_size[p]->scanwidth);
			free (r_particles.for_size[p]->offset);
			free (r_particles.for_size[p]);
		}
		p = 0; // the next 'for' loop will use this
		free (r_particles.for_size);
	}
	else
		r_particles_init = true;

	r_particles.max_size	= max_size; // must not be set before freeing the previously allocated memory
	r_particles.full_size	= full_size;

	r_particles.for_size = (d_particle_t **) malloc (sizeof (d_particle_t *) * (r_particles.max_size + 1));
	for ( ; p <= r_particles.max_size ; p++)
	{
		r_particles.for_size[p] = (d_particle_t *) malloc (sizeof (d_particle_t));

		pix = (double) (p + 2);
		width	= floor (0.5 + pix * r_particles.h_aspect);
		height	= floor (0.5 + pix);
		if (width < 1.0)
			width = 1.0;
		if (height < 1.0)
			height = 1.0;

		r_particles.for_size[p]->h_radius = (int) (0.5 * width );
		r_particles.for_size[p]->v_radius = (int) (0.5 * height);
		r_particles.for_size[p]->zcenter = r_particles.for_size[p]->h_radius + d_zwidth * r_particles.for_size[p]->v_radius;

		r_particles.for_size[p]->vrectright		= r_refdef.vrectright  - (int) width;
		r_particles.for_size[p]->vrectbottom	= r_refdef.vrectbottom - (int) height;

		count = height;
		r_particles.for_size[p]->lines = (int) count - 1;
		r_particles.for_size[p]->scanwidth	= (int *) malloc (sizeof (int) * (int) height);
		r_particles.for_size[p]->offset		= (int *) malloc (sizeof (int) * (int) height);
		for ( ; count > 0.0 ; count--)
		{
			// this "magic number" of 0.4 was achieved through trial and testing
			offset = 0.4 + (width - width * cos (asin (-1.0 + 2.0 * (count / height)))) * 0.5;
			r_particles.for_size[p]->scanwidth	[ (int) count - 1] = (int) (width - offset * 2);
			r_particles.for_size[p]->offset		[ (int) count - 1] = (int) offset;
		}
		r_particles.for_size[p]->unit = NULL;
	}
}



// transforms and lists the particle
void D_TransformParticle (particle_t *pparticle)
{
	vec3_t			local, transformed;
	float			zi, izi;
	int				pix;
	d_particle_t	*p;
//	float scale = -1.0f * (1.0f - ((cl.time - pparticle->start_time) / (pparticle->die - cl.time)));
//	scale = 2.0f - 2.0f * scale * scale;

	// transform point
	VectorSubtract (pparticle->org, r_origin, local);

	transformed[2] = DotProduct (local, r_ppn);
	if (transformed[2] < PARTICLE_Z_CLIP)
		return;

	transformed[0] = DotProduct (local, r_pright);
	transformed[1] = DotProduct (local, r_pup);

	// project the point
	zi = 1.0 / transformed[2];

	izi = zi * 0x8000;

	pix = ((int) (izi * r_particles.scale/* * scale*/) >> 8) - 2;
	if (pix < 0)
		p = r_particles.for_size[0];
	else if (pix <= r_particles.max_size)
		p = r_particles.for_size[pix];
	else if (pix < r_particles.full_size)
		p = r_particles.for_size[r_particles.max_size];
	else
		return;

	// width and height needs to be typecasted individually to avoid mismatched precision
	pparticle->u = (int) (xcenter + transformed[0] * zi) - p->h_radius;
	pparticle->v = (int) (ycenter - transformed[1] * zi) - p->v_radius;
	if	(
		(pparticle->u > p->vrectright)
	||	(pparticle->v > p->vrectbottom) // ok
	||	(pparticle->u < d_vrectx) // ok
	||	(pparticle->v < d_vrecty)
		)
		return;

	pparticle->z = (int) izi;
	pparticle->drawcolor = (byte) pparticle->color;
//	if (pparticle->color >= 96) // 192/2 // non-fullbrights (TODO: get the actual index from the palette)
//		pparticle->alpha = 1;
//	else
		pparticle->alpha = (r_particle_blend_mode.value) ? 1.0 - ( (cl.time - pparticle->start_time) / (pparticle->die - cl.time)) : 1.0;
	pparticle->next_to_draw = p->unit;
	p->unit = pparticle;
}



void D_EndParticles (void)
{
	byte	*pdest, *scandest, color;
	short	*pz, *scanz;
	int
		z
	,	i
	,	w
	,	count
	,	size = 0
	// lighting - begin
	,	r_ambientlight
	,	lnum
		;
	dlight_t	*dl;
	vec3_t		dist, t;
	float		add;
	// lighting - end
	d_particle_t	*p;
	particle_t		*pparticle;

	colorblendingmap = colorblendingmode[BLEND_WEIGHTEDMEAN];

	for ( ; size <= r_particles.max_size ; size++)
	{
		p = r_particles.for_size[size];
		while (p->unit != NULL)
		{
			pparticle = p->unit;
			pdest	= d_viewbuffer	+ d_scantable[pparticle->v] + pparticle->u;
			pz		= d_pzbuffer	+ (d_zwidth * pparticle->v) + pparticle->u;

			if (pz[p->zcenter] <= pparticle->z)
			{
				if (r_particle_lit.value)
				{
					VectorCopy (pparticle->org, t);
					r_ambientlight = R_LightPoint (t);
					// add dynamic lights
					for (lnum=0 ; lnum < MAX_DLIGHTS ; lnum++)
					{
						dl = &cl_dlights[lnum];
						if (dl->die < cl.time)
							continue;
						if (dl->dark)
							continue;
						if (dl->radius) // not viewmodel
						{
							VectorSubtract (t, dl->origin, dist);
							add = dl->radius - Length (dist);
							if (add > 0)
								r_ambientlight += (int)add;
						}
					}
					// minimum of (255 minus) 16 (8 less than in the viewmodel), maximum of (255 minus) 192 (same as MDL models)
					color = colorshadingmap_particle[pparticle->color + ( ( ( (r_ambientlight > 192) ? 63 : ( (r_ambientlight < 16) ? 239 : 255 - r_ambientlight)) << VID_CBITS) & 0xFF00)];
				}
				else
					color = currentTable[pparticle->color];
				count = p->lines;

				if (pparticle->alpha < 0.33)
				{
					for ( ; count > -1 ; count-- , pz += d_zwidth , pdest += screenwidth)
					#if 0
						for (i = p->offset[count] , w = p->scanwidth[count] ; i < w ; i++)
							pdest[i] = colorblendingmap[ (int) (pparticle->color + pdest[i] * 256)];
					#else
					{
						scandest = pdest + p->offset[count];
						w = p->scanwidth[count];
						do
						{
							i = (w >= 16) ? 16 : w;
							w -= i;

							scandest += i;
							switch (i)
							{
								case 16: scandest[-16] = colorblendingmap[ (int) (color + scandest[-16] * 256)];
								case 15: scandest[-15] = colorblendingmap[ (int) (color + scandest[-15] * 256)];
								case 14: scandest[-14] = colorblendingmap[ (int) (color + scandest[-14] * 256)];
								case 13: scandest[-13] = colorblendingmap[ (int) (color + scandest[-13] * 256)];
								case 12: scandest[-12] = colorblendingmap[ (int) (color + scandest[-12] * 256)];
								case 11: scandest[-11] = colorblendingmap[ (int) (color + scandest[-11] * 256)];
								case 10: scandest[-10] = colorblendingmap[ (int) (color + scandest[-10] * 256)];
								case  9: scandest[-9 ] = colorblendingmap[ (int) (color + scandest[-9 ] * 256)];
								case  8: scandest[-8 ] = colorblendingmap[ (int) (color + scandest[-8 ] * 256)];
								case  7: scandest[-7 ] = colorblendingmap[ (int) (color + scandest[-7 ] * 256)];
								case  6: scandest[-6 ] = colorblendingmap[ (int) (color + scandest[-6 ] * 256)];
								case  5: scandest[-5 ] = colorblendingmap[ (int) (color + scandest[-5 ] * 256)];
								case  4: scandest[-4 ] = colorblendingmap[ (int) (color + scandest[-4 ] * 256)];
								case  3: scandest[-3 ] = colorblendingmap[ (int) (color + scandest[-3 ] * 256)];
								case  2: scandest[-2 ] = colorblendingmap[ (int) (color + scandest[-2 ] * 256)];
								case  1: scandest[-1 ] = colorblendingmap[ (int) (color + scandest[-1 ] * 256)];
							}
						} while (w > 0);
					}
					#endif
				}
				else if (pparticle->alpha < 0.67)
				{
					for ( ; count > -1 ; count-- , pz += d_zwidth , pdest += screenwidth)
					#if 0
						for (i = p->offset[count] , w = p->scanwidth[count] ; i < w ; i++)
							pdest[i] = colorblendingmap[ (int) (pdest[i] + pparticle->color * 256)];
					#else
					{
						scandest = pdest + p->offset[count];
						w = p->scanwidth[count];
						do
						{
							i = (w >= 16) ? 16 : w;
							w -= i;

							scandest += i;
							switch (i)
							{
								case 16: scandest[-16] = colorblendingmap[ (int) (scandest[-16] + color * 256)];
								case 15: scandest[-15] = colorblendingmap[ (int) (scandest[-15] + color * 256)];
								case 14: scandest[-14] = colorblendingmap[ (int) (scandest[-14] + color * 256)];
								case 13: scandest[-13] = colorblendingmap[ (int) (scandest[-13] + color * 256)];
								case 12: scandest[-12] = colorblendingmap[ (int) (scandest[-12] + color * 256)];
								case 11: scandest[-11] = colorblendingmap[ (int) (scandest[-11] + color * 256)];
								case 10: scandest[-10] = colorblendingmap[ (int) (scandest[-10] + color * 256)];
								case  9: scandest[-9 ] = colorblendingmap[ (int) (scandest[-9 ] + color * 256)];
								case  8: scandest[-8 ] = colorblendingmap[ (int) (scandest[-8 ] + color * 256)];
								case  7: scandest[-7 ] = colorblendingmap[ (int) (scandest[-7 ] + color * 256)];
								case  6: scandest[-6 ] = colorblendingmap[ (int) (scandest[-6 ] + color * 256)];
								case  5: scandest[-5 ] = colorblendingmap[ (int) (scandest[-5 ] + color * 256)];
								case  4: scandest[-4 ] = colorblendingmap[ (int) (scandest[-4 ] + color * 256)];
								case  3: scandest[-3 ] = colorblendingmap[ (int) (scandest[-3 ] + color * 256)];
								case  2: scandest[-2 ] = colorblendingmap[ (int) (scandest[-2 ] + color * 256)];
								case  1: scandest[-1 ] = colorblendingmap[ (int) (scandest[-1 ] + color * 256)];
							}
						} while (w > 0);
					}
					#endif
				}
				else
				{
					z = pparticle->z;
					for ( ; count > -1 ; count-- , pz += d_zwidth , pdest += screenwidth)
					#if 0
						for (i = p->offset[count] , w = p->scanwidth[count] ; i < w ; i++)
						{
							pz[i] = pparticle->z;
							pdest[i] = pparticle->drawcolor;
						}
					#else
					{
						scanz = pz + p->offset[count];
						scandest = pdest + p->offset[count];
						w = p->scanwidth[count];
						do
						{
							i = (w >= 16) ? 16 : w;
							w -= i;

							scandest += i;
							scanz += i;
							switch (i)
							{
								case 16: scandest[-16] = color; scanz[-16] = z;
								case 15: scandest[-15] = color; scanz[-15] = z;
								case 14: scandest[-14] = color; scanz[-14] = z;
								case 13: scandest[-13] = color; scanz[-13] = z;
								case 12: scandest[-12] = color; scanz[-12] = z;
								case 11: scandest[-11] = color; scanz[-11] = z;
								case 10: scandest[-10] = color; scanz[-10] = z;
								case  9: scandest[-9 ] = color; scanz[-9 ] = z;
								case  8: scandest[-8 ] = color; scanz[-8 ] = z;
								case  7: scandest[-7 ] = color; scanz[-7 ] = z;
								case  6: scandest[-6 ] = color; scanz[-6 ] = z;
								case  5: scandest[-5 ] = color; scanz[-5 ] = z;
								case  4: scandest[-4 ] = color; scanz[-4 ] = z;
								case  3: scandest[-3 ] = color; scanz[-3 ] = z;
								case  2: scandest[-2 ] = color; scanz[-2 ] = z;
								case  1: scandest[-1 ] = color; scanz[-1 ] = z;
							}
						} while (w > 0);
					}
					#endif
				}
			}
			p->unit = pparticle->next_to_draw;
		}
		p->unit = NULL;
	}
}
// mankrip - end
