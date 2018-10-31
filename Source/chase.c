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
// chase.c -- chase camera code

#include "quakedef.h"

#define NUM_TESTS 64
#define CHASE_DEST_OFFSET 2.0f
// mankrip - begin
#define CHASE_MIN_ALPHADIST 24.0f
#define CHASE_MAX_ALPHADIST 24.0f

static int
	chase_contents // contents where the camera is allowed to go in
	;
float
	previous_chasecam_length = 0.0f // TODO: reset this on new map, and after an intermission (but not on regular changelevel)
	;
qboolean
	chase_nodraw // mh's chasecam fix
	;
cvar_t
	chase_speed  = {"chase_speed" ,  "30", true} // the time it takes to push the camera away from the player to the desired position
,	chase_back   = {"chase_back"  , "100", true}
,	chase_up     = {"chase_up"    ,  "16", true}
,	chase_right  = {"chase_right" ,   "0"}
,	chase_active = {"chase_active",   "0", true}
	;
// mankrip - end



void Chase_Init (void)
{
	Cvar_RegisterVariable (&chase_speed);
	Cvar_RegisterVariable (&chase_back);
	Cvar_RegisterVariable (&chase_up);
	Cvar_RegisterVariable (&chase_right);
	Cvar_RegisterVariableWithCallback (&chase_active, Chase_Contents);
}



// mankrip - begin
void Chase_Contents (void)
{
	chase_contents = (0x1 << (CONTENTS_EMPTY * -1));
	if (r_water_blend_mode.value > 0.0f)
	{
		if (r_water_blend_alpha.value < 1.0f)
			chase_contents |= (0x1 << (CONTENTS_WATER * -1));
	}
	else if (r_wateralpha.value < 1.0f)
		chase_contents |= (0x1 << (CONTENTS_WATER * -1));
	if (r_slime_blend_mode.value > 0.0f)
	{
		if (r_slime_blend_alpha.value < 1.0f)
			chase_contents |= (0x1 << (CONTENTS_SLIME * -1));
	}
	else if (r_wateralpha.value < 1.0f)
		chase_contents |= (0x1 << (CONTENTS_SLIME * -1));
	if (r_lava_blend_mode.value > 0.0f)
	{
		if (r_lava_blend_alpha.value < 1.0f)
			chase_contents |= (0x1 << (CONTENTS_LAVA * -1));
	}
	else if (r_wateralpha.value < 1.0f)
		chase_contents |= (0x1 << (CONTENTS_LAVA * -1));
}
// mankrip - end



// mankrip - mh's chasecam fix
static qboolean Chase_Check (vec3_t checkpoint, int viewcontents)
{
	int
		i
		;
	vec3_t
		mins
	,	maxs
		;
	entity_t
		* e
		;

	// check against world model
	if (! (0x1 << ( (Mod_PointInLeaf (checkpoint, cl.worldmodel))->contents * -1) & viewcontents))
		return false;

	// check visedicts - this happens *after* CL_ReadFromServer so the list will be valid
	for (i = 0 ; i < cl_numvisedicts ; i++)
	{
		// retrieve the current entity
		e = cl_visedicts[i];

		// don't check against self
		if (e == &cl_entities[cl.viewentity])
			continue;

		// derive the bbox
		if (e->model->type == mod_brush && (e->angles[0] || e->angles[1] || e->angles[2]))
		{
			// copied from R_CullBox rotation test for inline bmodels, loop just unrolled
			mins[0] = e->origin[0] - e->model->radius;
			maxs[0] = e->origin[0] + e->model->radius;
			mins[1] = e->origin[1] - e->model->radius;
			maxs[1] = e->origin[1] + e->model->radius;
			mins[2] = e->origin[2] - e->model->radius;
			maxs[2] = e->origin[2] + e->model->radius;
		}
		else
		{
			VectorAdd (e->origin, e->model->mins, mins);
			VectorAdd (e->origin, e->model->maxs, maxs);
		}

		// check against bbox
		if (checkpoint[0] < mins[0]) continue;
		if (checkpoint[1] < mins[1]) continue;
		if (checkpoint[2] < mins[2]) continue;
		if (checkpoint[0] > maxs[0]) continue;
		if (checkpoint[1] > maxs[1]) continue;
		if (checkpoint[2] > maxs[2]) continue;

		// point inside
		return false;
	}

	// it's good now
	return true;
}



void Chase_Update (void)
{
	float
		full_length
	,	chase_length
	,	chase_alpha
		;
	int
		num_tests
	,	viewcontents = (0x1 << ( (Mod_PointInLeaf (r_refdef.vieworg, cl.worldmodel))->contents * -1) | chase_contents) // take the contents of the view leaf
	,	best
	,	nummatches
	,	test
	// certain surfaces can be viewed at an oblique enough angle that they are partially clipped by znear, so now we fix that too...
	,	chase_vert[] = {0, 0, 1, 1, 2, 2}
	,	dest_offset[] = {CHASE_DEST_OFFSET, -CHASE_DEST_OFFSET}
		;
	vec3_t
		forward
	,	up
	,	right
	,	chase_dest
	,	chase_newdest
	#if 0
	,	mins
	,	maxs
	#endif
		;

	// if can't see player, reset
	AngleVectors (cl.viewangles, forward, right, up);

	// calc exact destination
	// mankrip - begin
	chase_dest[0] = r_refdef.vieworg[0]
	+      up[0] * chase_up   .value
	- forward[0] * chase_back .value
	-   right[0] * chase_right.value
	;
	chase_dest[1] = r_refdef.vieworg[1]
	+      up[1] * chase_up   .value
	- forward[1] * chase_back .value
	-   right[1] * chase_right.value
	;
	chase_dest[2] = r_refdef.vieworg[2]
	+      up[2] * chase_up   .value
	- forward[2] * chase_back .value
	-   right[2] * chase_right.value
	;

	full_length = sqrt (
		(r_refdef.vieworg[0] - chase_dest[0]) * (r_refdef.vieworg[0] - chase_dest[0])
	+	(r_refdef.vieworg[1] - chase_dest[1]) * (r_refdef.vieworg[1] - chase_dest[1])
	+	(r_refdef.vieworg[2] - chase_dest[2]) * (r_refdef.vieworg[2] - chase_dest[2])
		)
	;
	// calculate distance between chasecam and original org to establish number of tests we need.
	// an int is good enough here.:)  add a cvar multiplier to this...
	num_tests = ceil (full_length);
	// mankrip - end

	// move along path from r_refdef.vieworg to chase_dest
	for (best = 0; best < num_tests; best++)
	{
		chase_newdest[0] = r_refdef.vieworg[0] + (chase_dest[0] - r_refdef.vieworg[0]) * best / num_tests;
		chase_newdest[1] = r_refdef.vieworg[1] + (chase_dest[1] - r_refdef.vieworg[1]) * best / num_tests;
		chase_newdest[2] = r_refdef.vieworg[2] + (chase_dest[2] - r_refdef.vieworg[2]) * best / num_tests;

		// check for a leaf hit with different contents
		if (!Chase_Check (chase_newdest, viewcontents))
		{
			// go back to the previous best as this one is bad
			if (best > 1)
				best--;
			else
				best = num_tests;
			break;
		}
	}

	// move along path from chase_dest to r_refdef.vieworg
	for (; best >= 0; best--)
	{
		// number of matches
		nummatches = 0;
		test = 0;

		// adjust
		chase_dest[0] = r_refdef.vieworg[0] + (chase_dest[0] - r_refdef.vieworg[0]) * best / num_tests;
		chase_dest[1] = r_refdef.vieworg[1] + (chase_dest[1] - r_refdef.vieworg[1]) * best / num_tests;
		chase_dest[2] = r_refdef.vieworg[2] + (chase_dest[2] - r_refdef.vieworg[2]) * best / num_tests;

		// run 6 tests: -x/+x/-y/+y/-z/+z
		for (; test < 6; test++)
		{
			// adjust, test and put back.
			chase_dest[chase_vert[test]] -= dest_offset[test & 1];
			if (Chase_Check (chase_dest, viewcontents))
				nummatches++;
			chase_dest[chase_vert[test]] += dest_offset[test & 1];
		}

		// test result, if all match we're done in here
		if (nummatches == 6)
			break;
	}

	chase_length =
		(r_refdef.vieworg[0] - chase_dest[0]) * (r_refdef.vieworg[0] - chase_dest[0])
	+	(r_refdef.vieworg[1] - chase_dest[1]) * (r_refdef.vieworg[1] - chase_dest[1])
	+	(r_refdef.vieworg[2] - chase_dest[2]) * (r_refdef.vieworg[2] - chase_dest[2])
		;

	if (chase_length > previous_chasecam_length)
	{
		float
			length_scale
		,	length_max = sqrt (previous_chasecam_length) + chase_speed.value * (float)host_cpu_frametime
		,	chase_length2 = sqrt (chase_length)
			;
		if (chase_length2 > length_max)
		{
			// if can't see player, reset
			AngleVectors (cl.viewangles, forward, right, up);

			length_scale = length_max / full_length; // will be < 0
			chase_dest[0] = r_refdef.vieworg[0]
			+ (
			       up[0] * chase_up   .value
			- forward[0] * chase_back .value
			-   right[0] * chase_right.value
			) * length_scale
			;
			chase_dest[1] = r_refdef.vieworg[1]
			+ (
			       up[1] * chase_up   .value
			- forward[1] * chase_back .value
			-   right[1] * chase_right.value
			) * length_scale
			;
			chase_dest[2] = r_refdef.vieworg[2]
			+ (
			       up[2] * chase_up   .value
			- forward[2] * chase_back .value
			-   right[2] * chase_right.value
			) * length_scale
			;
			previous_chasecam_length =
				(r_refdef.vieworg[0] - chase_dest[0]) * (r_refdef.vieworg[0] - chase_dest[0])
			+	(r_refdef.vieworg[1] - chase_dest[1]) * (r_refdef.vieworg[1] - chase_dest[1])
			+	(r_refdef.vieworg[2] - chase_dest[2]) * (r_refdef.vieworg[2] - chase_dest[2])
				;
		}
		else
			previous_chasecam_length = chase_length;
	}
	else
		previous_chasecam_length = chase_length;

	// move towards destination
	VectorCopy (chase_dest, r_refdef.vieworg);

	// apply alpha to external view model
	#if 1
	if (sqrt (previous_chasecam_length) < CHASE_MIN_ALPHADIST)
	{
		chase_nodraw = true;
		chase_alpha = 1.0f;
	}
	else
	{
		if (cl_entities[cl.viewentity].D_PolysetDrawSpans == MDL_DrawSpans_ColorKeyed) // it's opaque
			cl_entities[cl.viewentity].colorblendingmap = colorblendingmode[BLEND_WEIGHTEDMEAN];
		chase_nodraw = false;
		chase_alpha = (sqrt (previous_chasecam_length) - CHASE_MIN_ALPHADIST) / CHASE_MAX_ALPHADIST;
		if (chase_alpha > 1.0f)
			chase_alpha = 1.0f;
		if (cl_entities[cl.viewentity].D_PolysetDrawSpans != MDL_DrawSpans_Shadow)
		{
			if (chase_alpha < 1.0f)
			{
				if (cl_entities[cl.viewentity].colorblendingmap != colorblendingmode[BLEND_ADDITIVE])
					cl_entities[cl.viewentity].colorblendingmap = colorblendingmode[BLEND_WEIGHTEDMEAN];
				cl_entities[cl.viewentity].D_PolysetDrawSpans = MDL_DrawSpans_Blend;
				cl_entities[cl.viewentity].alpha = chase_alpha;
			}
			if (chase_alpha < 0.5f)
			{
				cl_entities[cl.viewentity].D_PolysetDrawSpans = MDL_DrawSpans_BlendBackwards;
			}
		}
   }
	// mankrip - begin
   #else
// work in progress
	chase_nodraw = false;
{
	// retrieve the external view model entity
	entity_t *e = &cl_entities[cl.viewentity];

	// derive the bbox - TODO: implement rotation in this
	// mankrip - end
	if (e->model->type == mod_brush && (e->angles[0] || e->angles[1] || e->angles[2]))
	{
		// copied from R_CullBox rotation test for inline bmodels, loop just unrolled
		mins[0] = e->origin[0] - e->model->radius;
		maxs[0] = e->origin[0] + e->model->radius;
		mins[1] = e->origin[1] - e->model->radius;
		maxs[1] = e->origin[1] + e->model->radius;
		mins[2] = e->origin[2] - e->model->radius;
		maxs[2] = e->origin[2] + e->model->radius;
	}
	// mankrip - begin
	else
	{
		VectorScale (e->model->mins, 1.5f, mins);
		VectorScale (e->model->maxs, 1.5f, maxs);
		VectorAdd (e->origin, mins, mins);
		VectorAdd (e->origin, maxs, maxs);
	}
}
	// check against bbox
	if (chase_dest[0] < mins[0]) return;
	if (chase_dest[1] < mins[1]) return;
	if (chase_dest[2] < mins[2]) return;
	if (chase_dest[0] > maxs[0]) return;
	if (chase_dest[1] > maxs[1]) return;
	if (chase_dest[2] > maxs[2]) return;

	// point inside
	chase_nodraw = true;
	#endif
	// mankrip - end
}
