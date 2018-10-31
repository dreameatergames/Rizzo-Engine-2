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

#ifndef INCLUDE_PTC_FILE
#define INCLUDE_PTC_FILE
#define PARTICLE_Z_CLIP	8.0

typedef enum
{
	pt_static
,	pt_grav
,	pt_slowgrav
,	pt_fire
,	pt_explode
,	pt_explode2
,	pt_blob
,	pt_blob2
} ptype_t;

// !!! if this is changed, it must be changed in d_ifacea.h too !!!
typedef struct particle_s
{
	// driver-usable fields
	vec3_t		org;
	byte		color; // mankrip - float/byte mismatch fixed (asm code uses float)
	// drivers never touch the following fields
	struct particle_s	*next;
	vec3_t		vel;
	float		ramp;
	float		die;
	ptype_t		type;
	// mankrip - begin
	float
		start_time
	,	alpha // TODO: change to byte
		;
	int
		u
	,	v
	,	z
		;
	byte
		drawcolor
		;
	struct particle_s
		* next_to_draw
		;
	// mankrip - end
} particle_t;


// !!! if this is changed, it must be changed in d_ifacea.h too !!!
typedef struct
{
	int		u, v;
	float	zi;
	int		color;
} zpointdesc_t;

extern zpointdesc_t
	r_zpointdesc
	;

// mankrip - begin
extern void
	D_Particles_Recalc (void)
,	D_TransformParticle (particle_t *pparticle)
,	D_EndParticles (void)
,	R_ParseParticleEffect (void)
,	R_RunParticleEffect (vec3_t org, vec3_t dir, int color, int count)
,	R_RocketTrail (vec3_t start, vec3_t end, int type)
,	R_DarkFieldParticles (entity_t *ent)
,	R_EntityParticles (entity_t *ent)
,	R_BlobExplosion (vec3_t org)
,	R_ParticleExplosion (vec3_t org)
,	R_ParticleExplosion2 (vec3_t org, int colorStart, int colorLength)
,	R_LavaSplash (vec3_t org)
,	R_TeleportSplash (vec3_t org)
	;
// mankrip - end
#endif
