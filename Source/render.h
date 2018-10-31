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

// refresh.h -- public interface to refresh functions

//#include "mdl_file.h"
#include "d_local.h"
#include "d_iface.h"

#define	MAXCLIPPLANES	11

#define	TOP_RANGE		16			// soldier uniform colors
#define	BOTTOM_RANGE	96

//=============================================================================

typedef struct efrag_s
{
	struct mleaf_s		*leaf;
	struct efrag_s		*leafnext;
	struct entity_s		*entity;
	struct efrag_s		*entnext;
} efrag_t;

// mankrip - begin
extern cvar_t
	r_light_vec_x
,	r_light_vec_y
,	r_light_vec_z
,	r_light_style
,	r_light_fx

,	r_interpolation

,	r_particle_sizelimit
,	r_particle_blend_mode
,	r_particle_lit
//,	r_particle_blend_map	// per effect
//,	r_particle_blend_alpha	// dynamic

,	r_water_blend_mode
,	r_water_blend_map
,	r_water_blend_alpha
,	r_wateralpha

,	r_slime_blend_mode
,	r_slime_blend_map
,	r_slime_blend_alpha

,	r_lava_blend_mode
,	r_lava_blend_map
,	r_lava_blend_alpha

,	r_portal_blend_mode
,	r_portal_blend_map
,	r_portal_blend_alpha

,	r_mirror_blend_mode
,	r_mirror_blend_map
,	r_mirror_blend_alpha

,	r_sky_blend_mode
,	r_sky_blend_map
,	r_sky_blend_alpha
	;

extern byte
	r_foundtranslucency
	;

extern int
	r_overdraw
,	r_overdrawstep[5] // portals, lava, slime, water, liquids
,	r_overdrawindex
	;
// mankrip - end

typedef struct entity_s
{
	qboolean				forcelink;		// model changed

	int						update_type;

	entity_state_t			baseline;		// to fill in defaults in updates

	double					msgtime;		// time of last update
	vec3_t					msg_origins[2];	// last two updates (0 is newest)
	vec3_t					origin;
	vec3_t					msg_angles[2];	// last two updates (0 is newest)
	vec3_t					angles;
	struct model_s			*model;			// NULL = no model
	struct efrag_s			*efrag;			// linked list of efrags
	int						frame;
	float					syncbase;		// for client-side animations
	byte					*colormap;
	int						effects;		// light, particals, etc
	int						skinnum;		// for Alias models
	int						visframe;		// last frame this entity was
											//  found in an active leaf

	int						dlightframe;	// dynamic lighting
	int						dlightbits;

// FIXME: could turn these into a union
	int						trivial_accept;
	struct mnode_s			*topnode;		// for bmodels, first world node
											//  that splits bmodel, or NULL if
											//  not split
	// mankrip - model interpolation - begin
	struct model_s
		* lastmodel
		;
	qboolean
		reset_frame_interpolation // mankrip
		;
	float
		frame_start_time
	,	frame_interval // mankrip - QC frame_interval
		;
	struct maliasgroup_s
		* framegroup1 // mankrip
		;
	int
		pose1
		;
	struct maliasgroup_s
		* framegroup2 // mankrip - renamed
		;
	int
		pose2
		;
	float
		translate_start_time
		;
	vec3_t
		origin1
	,	origin2
		;
	float
		rotate_start_time
		;
	vec3_t
		angles1
	,	angles2
		;
	// mankrip - model interpolation - end
	// Tomaz - QC Alpha Scale Glow Begin
	float
		alpha
	,	scale_start_time
//	,	scale1
	,	scale2
		;
	vec3_t
		scalev
		;
	float
		glow_size
//	,	glow_red
//	,	glow_green
//	,	glow_blue
	// Tomaz - QC Alpha Scale Glow End
	,	lightlevel // mankrip
	,	distance // mankrip - depth sorting
		;
	void
		// MDL
		(* D_PolysetDrawSpans)	(void)
	,	(* R_Outline)			(void)
		// SPR
	,	(* D_SpriteDrawSpans)	(void)
		// BSP
	,	(* D_DrawSpans)			() // keep empty, espan_t hasn't been declared yet
	,	(* D_DrawTurbulent)		() // keep empty, espan_t hasn't been declared yet
		;
	byte
		* colorblendingmap
//	,	* colorshadingmap // *colormap
		;
} entity_t;

// !!! if this is changed, it must be changed in asm_draw.h too !!!
typedef struct
{
	vrect_t		vrect;				// subwindow in video for refresh
									// FIXME: not need vrect next field here?
	vrect_t		aliasvrect;			// scaled Alias version
	int			vrectright, vrectbottom;	// right & bottom screen coords
	int			aliasvrectright, aliasvrectbottom;	// scaled Alias versions
	float		vrectrightedge;			// rightmost right edge we care about,
										//  for use in edge list
	float		fvrectx, fvrecty;		// for floating-point compares
	float		fvrectx_adj, fvrecty_adj; // left and top edges, for clamping
	int			vrect_x_adj_shift20;	// (vrect.x + 0.5 - epsilon) << 20
	int			vrectright_adj_shift20;	// (vrectright + 0.5 - epsilon) << 20
	float		fvrectright_adj, fvrectbottom_adj;
										// right and bottom edges, for clamping
	float		fvrectright;			// rightmost edge, for Alias clamping
	float		fvrectbottom;			// bottommost edge, for Alias clamping
	float		horizontalFieldOfView;	// at Z = 1.0, this many X is visible
										// 2.0 = 90 degrees
	float		xOrigin;			// should probably allways be 0.5
	float		yOrigin;			// between be around 0.3 to 0.5

	vec3_t		vieworg;
	vec3_t		viewangles;

	float		fov_x, fov_y;

	int			ambientlight;
} refdef_t;


//
// refresh
//
extern	int		reinit_surfcache;


extern	refdef_t	r_refdef;
extern vec3_t	r_origin, vpn, vright, vup;

float fovscale; // mankrip - FOV-based scaling

void R_Init (void);
void R_InitTextures (void);
void R_InitEfrags (void);
void R_RenderView (void);		// must set r_refdef first
void R_ViewChanged (vrect_t *pvrect, int lineadj, float aspect); // called whenever r_refdef or vid change
void LoadPCX (char *filename, byte **pic, int *width, int *height); // mankrip - skyboxes
void LoadTGA_as8bit (char *filename, byte **pic, int *width, int *height); // mankrip

void R_AddEfrags (entity_t *ent);
void R_RemoveEfrags (entity_t *ent);

void R_NewMap (void);

void R_PushDlights (void);

void R_SetVrect (vrect_t *pvrect, vrect_t *pvrectin, int lineadj);

