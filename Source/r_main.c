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
// r_main.c

#include "quakedef.h"
#include "r_local.h"
#include "d_local.h" // mankrip

//define	PASSAGES

float		r_time1;
int			r_numallocatededges;
qboolean	r_drawpolys;
qboolean	r_drawculledpolys;
qboolean	r_worldpolysbacktofront;
qboolean	r_recursiveaffinetriangles = true;
int			r_pixbytes = 1;
float		r_aliasuvscale = 1.0;
int			r_outofsurfaces;
int			r_outofedges;

qboolean	r_dowarp, r_dowarpold, r_viewchanged;

int			numbtofpolys;
btofpoly_t	*pbtofpolys;
mvertex_t	*r_pcurrentvertbase;

int			c_surf;
int			r_maxsurfsseen, r_maxedgesseen, r_cnumsurfs;
qboolean	r_surfsonstack;
int			r_clipflags;

byte		*r_warpbuffer;

byte		*r_stack_start;

qboolean	r_fov_greater_than_90;

//
// view origin
//
vec3_t	vup, base_vup;
vec3_t	vpn, base_vpn;
vec3_t	vright, base_vright;
vec3_t	r_origin;

//
// screen size info
//
refdef_t	r_refdef;
float		xcenter, ycenter;
float		xscale, yscale;
float		xscaleinv, yscaleinv;
float		xscaleshrink, yscaleshrink;
float		aliasxscale, aliasyscale, aliasxcenter, aliasycenter;

int		screenwidth;

float	pixelAspect;
float	screenAspect;
float	verticalFieldOfView;
float	xOrigin, yOrigin;

mplane_t	screenedge[4];

//
// refresh flags
//
int		r_framecount = 1;	// so frame counts initialized to 0 don't match
int		r_visframecount;
int		d_spanpixcount;
int		r_polycount;
int		r_drawnpolycount;
int		r_wholepolycount;

#define		VIEWMODNAME_LENGTH	256
char		viewmodname[VIEWMODNAME_LENGTH+1];
int			modcount;

int			*pfrustum_indexes[4];
int			r_frustum_indexes[4*6];

int		reinit_surfcache = 1;	// if 1, surface cache is currently empty and
								// must be reinitialized for current cache size

mleaf_t		*r_viewleaf, *r_oldviewleaf;

texture_t	*r_notexture_mip;

float		r_aliastransition, r_resfudge;

int		d_lightstylevalue[256];	// 8.8 fraction of base light value

float	dp_time1, dp_time2, db_time1, db_time2, rw_time1, rw_time2;
float	se_time1, se_time2, de_time1, de_time2, dv_time1, dv_time2;

void R_MarkLeaves (void);
int R_BmodelCheckBBox (model_t *clmodel, float *minmaxs); // mankrip

cvar_t	r_draworder = {"r_draworder","0"};
cvar_t	r_speeds = {"r_speeds","0"};
cvar_t	r_timegraph = {"r_timegraph","0"};
cvar_t	r_graphheight = {"r_graphheight","10"};
cvar_t	r_clearcolor = {"r_clearcolor","2"};
cvar_t	r_waterwarp = {"r_waterwarp","1"};
cvar_t	r_fullbright = {"r_fullbright","0"};
cvar_t	r_drawentities = {"r_drawentities","1"};
cvar_t	r_drawviewmodel = {"r_drawviewmodel","1", true}; // mankrip - saved in the config file - edited
cvar_t	r_aliasstats = {"r_polymodelstats","0"};
cvar_t	r_dspeeds = {"r_dspeeds","0"};
cvar_t	r_drawflat = {"r_drawflat", "0"};
cvar_t	r_ambient = {"r_ambient", "0"};
cvar_t	r_reportsurfout = {"r_reportsurfout", "0"};
cvar_t	r_maxsurfs = {"r_maxsurfs", "0"};
cvar_t	r_numsurfs = {"r_numsurfs", "0"};
cvar_t	r_reportedgeout = {"r_reportedgeout", "0"};
cvar_t	r_maxedges = {"r_maxedges", "0"};
cvar_t	r_numedges = {"r_numedges", "0"};
cvar_t	r_aliastransbase = {"r_aliastransbase", "200"};
cvar_t	r_aliastransadj = {"r_aliastransadj", "100"};
// mankrip - begin
// mankrip - begin
cvar_t
	d_dithertexture		= {"d_dithertexture"	,  "0.3"	, true}
,	d_ditherlighting	= {"d_ditherlighting"	,  "1"	, true}
,	r_light_vec_x		= {"r_light_vec_x"		, "-1"	}
,	r_light_vec_y		= {"r_light_vec_y"		,  "0"	}
,	r_light_vec_z		= {"r_light_vec_z"		, "-1"	}
,	r_light_style		= {"r_light_style"		,  "1"	, true}
#ifdef _arch_dreamcast
,	r_light_fx			= {"r_light_fx"			,  "0"	}
#else
,	r_light_fx			= {"r_light_fx"			,  "63" } // (EF_ROCKET|EF_MUZZLEFLASH|EF_BRIGHTLIGHT|EF_DIMLIGHT|16_explosions|32_animated_lightstyles)
#endif
,	r_interpolation		= {"r_interpolation"	,  "1"	} // model interpolation
,	r_sprite_addblend	= {"r_sprite_addblend"	,  "0"	, true}
,	r_sprite_lit		= {"r_sprite_lit"		,  "0"	, true}
,	r_externbsp_lit		= {"r_externbsp_lit"	,  "1"	, true}

,	r_water_blend_mode	= {"r_water_blend_mode"	,  "1"	, true}
,	r_water_blend_map	= {"r_water_blend_map"	,  ""+BLEND_SMOKE		, true}
#ifdef _arch_dreamcast
,	r_water_blend_alpha	= {"r_water_blend_alpha",  "1", true}
#else
,	r_water_blend_alpha	= {"r_water_blend_alpha",  "0.66", true}
#endif

,	r_wateralpha		= {"r_wateralpha"		,  "1", true}

,	r_slime_blend_mode	= {"r_slime_blend_mode"	,  "1"	, true}
,	r_slime_blend_map	= {"r_slime_blend_map"	,  ""+BLEND_WEIGHTEDMEAN	, true}
#ifdef _arch_dreamcast
,	r_slime_blend_alpha	= {"r_slime_blend_alpha",  "1", true}
#else
,	r_slime_blend_alpha	= {"r_slime_blend_alpha",  "0.66", true}
#endif

,	r_lava_blend_mode	= {"r_lava_blend_mode"	,  "1"	, true}
,	r_lava_blend_map	= {"r_lava_blend_map"	,  ""+BLEND_ADDITIVE		, true}
,	r_lava_blend_alpha	= {"r_lava_blend_alpha"	,  "1"	, true}

,	r_portal_blend_mode	= {"r_portal_blend_mode",  "1"	, true}
,	r_portal_blend_map	= {"r_portal_blend_map"	,  ""+BLEND_WEIGHTEDMEAN	, true}
,	r_portal_blend_alpha= {"r_portal_blend_alpha", "1"	, true}

,	r_mirror_blend_mode	= {"r_mirror_blend_mode",  "0"	, false}
,	r_mirror_blend_map	= {"r_mirror_blend_map"	,  ""+BLEND_SMOKE		, false}
,	r_mirror_blend_alpha= {"r_mirroralpha"		,  "0.66", false}

,	r_sky_blend_mode	= {"r_sky_blend_mode"	,  "0"	, true}
,	r_sky_blend_map		= {"r_sky_blend_map"	,  ""+BLEND_WEIGHTEDMEAN	, true}
#ifdef _arch_dreamcast
,	r_sky_blend_alpha	= {"r_sky_blend_alpha"	,  "1", true}
#else
,	r_sky_blend_alpha	= {"r_sky_blend_alpha"	,  "0.66", true}
#endif

,	r_stereo_separation = {"r_stereo_separation",  "0"	, true}
	;

int
	r_stereo_side
,	r_overdraw
,	r_overdrawstep[5] // portals, lava, slime, water, liquids
,	r_overdrawindex
	;

byte
	r_foundtranslucency
	;

entity_t
	*d_externalviewentity
,	*d_internalviewentity
	;
// mankrip - end

extern cvar_t	scr_fov;


/*
==================
R_InitTextures
==================
*/
void	R_InitTextures (void)
{
	int		x,y, m;
	byte	*dest;

// create a simple checkerboard texture for the default
	r_notexture_mip = Hunk_AllocName (sizeof(texture_t) + 16*16+8*8+4*4+2*2, "notexture");

	r_notexture_mip->width = r_notexture_mip->height = 16;
	r_notexture_mip->offsets[0] = sizeof(texture_t);
	r_notexture_mip->offsets[1] = r_notexture_mip->offsets[0] + 16*16;
	r_notexture_mip->offsets[2] = r_notexture_mip->offsets[1] + 8*8;
	r_notexture_mip->offsets[3] = r_notexture_mip->offsets[2] + 4*4;

	for (m=0 ; m<4 ; m++)
	{
		dest = (byte *)r_notexture_mip + r_notexture_mip->offsets[m];
		for (y=0 ; y< (16>>m) ; y++)
			for (x=0 ; x< (16>>m) ; x++)
			{
				if (  (y< (8>>m) ) ^ (x< (8>>m) ) )
					*dest++ = 0;
				else
					*dest++ = 0xff;
			}
	}
}

/*
===============
R_Init
===============
*/
//void SCR_Adjust (void); // mankrip
void R_LoadSky_f (void); // mankrip - skyboxes // Code taken from the ToChriS engine - Author: Vic (vic@quakesrc.org) (http://hkitchen.quakesrc.org/)
void R_Init (void)
{
	int		dummy;

// get stack position so we can guess if we are going to overflow
	r_stack_start = (byte *)&dummy;

	R_InitTurb ();

	Cmd_AddCommand ("timerefresh", R_TimeRefresh_f);
	Cmd_AddCommand ("loadsky", R_LoadSky_f); // mankrip - skyboxes // Code taken from the ToChriS engine - Author: Vic (vic@quakesrc.org) (http://hkitchen.quakesrc.org/)

	Cvar_RegisterVariable (&d_ditherlighting);
	Cvar_RegisterVariable (&d_dithertexture);
	Cvar_RegisterVariable (&r_draworder);
	Cvar_RegisterVariable (&r_speeds);
	Cvar_RegisterVariable (&r_timegraph);
	Cvar_RegisterVariable (&r_graphheight);
	Cvar_RegisterVariable (&r_drawflat);
	Cvar_RegisterVariable (&r_ambient);
	Cvar_RegisterVariable (&r_clearcolor);
	Cvar_RegisterVariable (&r_waterwarp);
	Cvar_RegisterVariable (&r_fullbright);
	Cvar_RegisterVariable (&r_drawentities);
	Cvar_RegisterVariable (&r_drawviewmodel);
	Cvar_RegisterVariable (&r_aliasstats);
	Cvar_RegisterVariable (&r_dspeeds);
	Cvar_RegisterVariable (&r_reportsurfout);
	Cvar_RegisterVariable (&r_maxsurfs);
	Cvar_RegisterVariable (&r_numsurfs);
	Cvar_RegisterVariable (&r_reportedgeout);
	Cvar_RegisterVariable (&r_maxedges);
	Cvar_RegisterVariable (&r_numedges);
	Cvar_RegisterVariable (&r_aliastransbase);
	Cvar_RegisterVariable (&r_aliastransadj);
	// mankrip - begin
	Cvar_RegisterVariable (&r_skyname); // skyboxes // Code taken from the ToChriS engine - Author: Vic (vic@quakesrc.org) (http://hkitchen.quakesrc.org/)
	Cvar_RegisterVariable (&r_skyspeed1);
	Cvar_RegisterVariable (&r_skyspeed2);
	Cvar_RegisterVariable (&r_light_vec_x);
	Cvar_RegisterVariable (&r_light_vec_y);
	Cvar_RegisterVariable (&r_light_vec_z);
	Cvar_RegisterVariable (&r_light_style);
	Cvar_RegisterVariable (&r_light_fx);
	Cvar_RegisterVariable (&r_interpolation);

	Cvar_RegisterVariableWithCallback	(&r_water_blend_mode	, Chase_Contents);
	Cvar_RegisterVariable				(&r_water_blend_map);
	Cvar_RegisterVariableWithCallback	(&r_water_blend_alpha	, Chase_Contents);
	Cvar_RegisterVariableWithCallback	(&r_wateralpha			, Chase_Contents);
	Cvar_RegisterVariableWithCallback	(&r_slime_blend_mode	, Chase_Contents);
	Cvar_RegisterVariable				(&r_slime_blend_map);
	Cvar_RegisterVariableWithCallback	(&r_slime_blend_alpha	, Chase_Contents);
	Cvar_RegisterVariableWithCallback	(&r_lava_blend_mode		, Chase_Contents);
	Cvar_RegisterVariable				(&r_lava_blend_map);
	Cvar_RegisterVariableWithCallback	(&r_lava_blend_alpha	, Chase_Contents);
	// portals uses the same pointcontents as water, so no callback
	Cvar_RegisterVariable (&r_portal_blend_mode);
	Cvar_RegisterVariable (&r_portal_blend_map);
	Cvar_RegisterVariable (&r_portal_blend_alpha);
	Cvar_RegisterVariable (&r_mirror_blend_mode);
	Cvar_RegisterVariable (&r_mirror_blend_map);
	Cvar_RegisterVariable (&r_mirror_blend_alpha);
	Cvar_RegisterVariable (&r_sky_blend_mode);
	Cvar_RegisterVariable (&r_sky_blend_map);
	Cvar_RegisterVariable (&r_sky_blend_alpha);

	Cvar_RegisterVariable (&r_sprite_addblend);
	Cvar_RegisterVariable (&r_sprite_lit);
	Cvar_RegisterVariable (&r_externbsp_lit);
	Cvar_RegisterVariable (&r_stereo_separation);
	// mankrip - end

	#ifndef _arch_dreamcast
	Cvar_SetValue ("r_maxedges", 4.0f*(float)NUMSTACKEDGES);
	Cvar_SetValue ("r_maxsurfs", 4.0f*(float)NUMSTACKSURFACES);
	#else
	Cvar_SetValue ("r_maxedges", (float)NUMSTACKEDGES);
	Cvar_SetValue ("r_maxsurfs", (float)NUMSTACKSURFACES);
	#endif

	view_clipplanes[0].leftedge = true;
	view_clipplanes[1].rightedge = true;
	view_clipplanes[1].leftedge = view_clipplanes[2].leftedge =
			view_clipplanes[3].leftedge = false;
	view_clipplanes[0].rightedge = view_clipplanes[2].rightedge =
			view_clipplanes[3].rightedge = false;

	r_refdef.xOrigin = XCENTERING;
	r_refdef.yOrigin = YCENTERING;

	R_InitParticles ();

// TODO: collect 386-specific code in one place
#if	id386
	Sys_MakeCodeWriteable ((long)R_EdgeCodeStart,
					     (long)R_EdgeCodeEnd - (long)R_EdgeCodeStart);
#endif	// id386

	D_Init ();
}

// mankrip - skyboxes - begin
// Code taken from the ToChriS engine - Author: Vic (vic@quakesrc.org) (http://hkitchen.quakesrc.org/)
void CL_ParseEntityLump (char *entdata)
{
	char *data;
	char key[128], value[4096];
	extern cvar_t r_skyname;

	data = entdata;

	if (!data)
		return;
	data = COM_Parse (data);
	if (!data || com_token[0] != '{')
		return;							// error

	while (1)
	{
		data = COM_Parse (data);
		if (!data)
			return;						// error
		if (com_token[0] == '}')
			break;						// end of worldspawn

		if (com_token[0] == '_')
			strcpy(key, com_token + 1);
		else
			strcpy(key, com_token);

		while (key[strlen(key)-1] == ' ')
			key[strlen(key)-1] = 0;		// remove trailing spaces

		data = COM_Parse (data);
		if (!data)
			return;						// error
		strcpy (value, com_token);

		if (strcmp (key, "sky") == 0 || strcmp (key, "skyname") == 0 ||
				strcmp (key, "qlsky") == 0)
		//	Cvar_Set ("r_skyname", value);
		// mankrip - begin
		{
			Cbuf_AddText(va("wait;loadsky %s\n", value));
		//	R_LoadSky(value);
			return; // mankrip
		}
		// mankrip - end
		// more checks here..
	}
	// mankrip - begin
	if (r_skyname.string[0])
		Cbuf_AddText(va("wait;loadsky %s\n", r_skyname.string));
	//	R_LoadSky(r_skyname.string); // crashes the engine if the r_skyname cvar is set before the game boots
	else
		R_LoadSky("");
	// mankrip - end
}
// mankrip - skyboxes - end

/*
===============
R_NewMap
===============
*/
void R_NewMap (void)
{
	int		i;

// clear out efrags in case the level hasn't been reloaded
// FIXME: is this one short?
	for (i=0 ; i<cl.worldmodel->numleafs ; i++)
		cl.worldmodel->leafs[i].efrags = NULL;

	CL_ParseEntityLump (cl.worldmodel->entities); // mankrip - skyboxes // Code taken from the ToChriS engine - Author: Vic (vic@quakesrc.org) (http://hkitchen.quakesrc.org/)
	r_viewleaf = NULL;
	R_ClearParticles ();
	R_InitSkyBox (); // mankrip - skyboxes // Code taken from the ToChriS engine - Author: Vic (vic@quakesrc.org) (http://hkitchen.quakesrc.org/)

	r_cnumsurfs = r_maxsurfs.value;

	if (r_cnumsurfs <= MINSURFACES)
		r_cnumsurfs = MINSURFACES;

	if (r_cnumsurfs > NUMSTACKSURFACES)
	{
		surfaces = Hunk_AllocName (r_cnumsurfs * sizeof(surf_t), "surfaces");
		surface_p = surfaces;
		surf_max = &surfaces[r_cnumsurfs];
		r_surfsonstack = false;
	// surface 0 doesn't really exist; it's just a dummy because index 0
	// is used to indicate no edge attached to surface
		surfaces--;
#if	id386 // mankrip - no 386 assembler, no MASM
		R_SurfacePatch ();
#endif // mankrip - no 386 assembler, no MASM
	}
	else
	{
		r_surfsonstack = true;
	}

	r_maxedgesseen = 0;
	r_maxsurfsseen = 0;

	r_numallocatededges = r_maxedges.value;

	if (r_numallocatededges < MINEDGES)
		r_numallocatededges = MINEDGES;

	if (r_numallocatededges <= NUMSTACKEDGES)
	{
		auxedges = NULL;
	}
	else
	{
		auxedges = Hunk_AllocName (r_numallocatededges * sizeof(edge_t),
								   "edges");
	}

	r_dowarpold = false;
	r_viewchanged = false;
#ifdef PASSAGES
CreatePassages ();
#endif
}


/*
===============
R_SetVrect
===============
*/
extern cvar_t temp1;
void R_SetVrect (vrect_t *pvrectin, vrect_t *pvrect, int lineadj)
{
	int		h;
	float	size;

	size = scr_viewsize.value > 100 ? 100 : scr_viewsize.value;
	if (cl.intermission)
	{
		size = 100;
		lineadj = 0;
	}
	size /= 100.0; // fixed

	h = pvrectin->height - lineadj;
	pvrect->width = pvrectin->width * size;
	if (pvrect->width < 96)
	{
		size = 96.0 / pvrectin->width;
		pvrect->width = 96;	// min for icons
	}
	pvrect->width &= ~7;
	pvrect->height = pvrectin->height * size;
	if (pvrect->height > h)//pvrectin->height - lineadj) // optimized
		pvrect->height = h;//pvrectin->height - lineadj; // optimized

	pvrect->height &= ~1;

	pvrect->x = (pvrectin->width - pvrect->width)/2;
	pvrect->y = (h - pvrect->height)/2;

	// mankrip - screen positioning - begin
	{
		int hspace = (int)(vid.width  / 40);//80);
		int vspace = (int)(vid.height / 30);//60);

		if (screen_left > hspace)
		{
			pvrect->x += screen_left - hspace;
			pvrect->width -= screen_left - hspace;
		}

		if (screen_right > hspace)
			pvrect->width -= screen_right - hspace;

	/*	if ((scr_con_current > screen_top) && (scr_con_current < vid.height))
		{
			pvrect->y += scr_con_current;
			pvrect->height -= scr_con_current;
		}
		else	*/
		if (screen_top > vspace)
		{
			pvrect->y += screen_top - vspace;
			pvrect->height -= screen_top - vspace;
		}

		if (sb_lines)
		{
			pvrect->height -= screen_bottom;
			vid.aspect = ((float)(pvrect->height + sb_lines + min(vspace, screen_bottom)) / (float)pvrect->width) * (320.0 / 240.0);
		}
		else
		{
			if (screen_bottom > vspace)
				pvrect->height -= screen_bottom - vspace;
			vid.aspect = ((float)pvrect->height / (float)pvrect->width) * (320.0 / 240.0);
		}
		// mankrip - svc_letterbox - start
		if (cl.letterbox)
		{
			int letterbox_height = (int)((float)pvrect->height * (cl.letterbox/2.0));
			pvrect->y += letterbox_height; // - sb_lines/2
			pvrect->height -= letterbox_height*2;
		}
		// mankrip - svc_letterbox - end
		// pvrect->width and pvrect->height must be a multiple of 4! (I guess)
		{
		int width;
	//	int height;
		width  = (pvrect->width /4)*4;
		if (pvrect->width != width) // pvrect->width is not a multiple of 4
		{
			pvrect->x = (pvrect->x/4)*4;
			pvrect->width = width+4;
		}
		/*
		// disabled this for now, because forcing pvrect->height to a multiple of 4
		// may make animated letterboxes to not look good
		height = (pvrect->height/4)*4;
		if (pvrect->height != height) // pvrect->height is not a multiple of 4
		{
			pvrect->y = (pvrect->y/4)*4;
			pvrect->height = height+4;
		}
		*/
		}
	//	Con_Printf("x=%i w=%i y=%i h=%i\n", pvrect->x, pvrect->width, pvrect->y, pvrect->height);
	}
	// mankrip - stereo 3D - begin
	if (r_stereo_separation.value)
	{
		// right side
		if (r_stereo_side)
		{
			pvrect->width = pvrect->width/2 - 1;
			pvrect->height = pvrect->height/2 - 1;
			pvrect->y += pvrect->height/2;
			pvrect->x += pvrect->width + 2;
		}
		// left side
		else //if (!r_stereo_side)
		{
			pvrect->width = pvrect->width/2 - 1;
			pvrect->height = pvrect->height/2 - 1;
			pvrect->y += pvrect->height/2;
		}
	}
	// mankrip - stereo 3D - end
	// mankrip - split screen - begin
	/*
	// this is just a hack
	if (temp1.value == 1)
	{
		pvrect->width = pvrect->width/2 - 1;
		pvrect->height = pvrect->height/2 - 1;
	}
	if (temp1.value == 2)
	{
		pvrect->width = pvrect->width/2 - 1;
		pvrect->height = pvrect->height/2 - 1;
		pvrect->x += pvrect->width + 2;
	}
	if (temp1.value == 3)
	{
		pvrect->width = pvrect->width/2 - 1;
		pvrect->height = pvrect->height/2 - 1;
		pvrect->y += pvrect->height + 2;
	}
	if (temp1.value == 4)
	{
		pvrect->width = pvrect->width/2 - 1;
		pvrect->height = pvrect->height/2 - 1;
		pvrect->x += pvrect->width + 2;
		pvrect->y += pvrect->height + 2;
	}
	*/
	// mankrip - split screen - end
	// mankrip - screen positioning - end

	{
		if (lcd_x.value)
		{
			vid.aspect *= 0.5;
			pvrect->y >>= 1;
			pvrect->height >>= 1;

/*			// mankrip - begin
			pvrect->y += (r_framecount&1);
			pvrect->height -= (r_framecount&1);
			if ((r_framecount&1))
				vid.buffer += vid.rowbytes>>1;
			else
				vid.buffer -= vid.rowbytes>>1;
*/			// mankrip - end
		}
	}
}


/*
===============
R_ViewChanged

Called every time the vid structure or r_refdef changes.
Guaranteed to be called before the first refresh
===============
*/
void R_ViewChanged (vrect_t *pvrect, int lineadj, float aspect)
{
	int		i;
	float	res_scale;

	r_viewchanged = true;

	R_SetVrect (pvrect, &r_refdef.vrect, lineadj);

	r_refdef.horizontalFieldOfView = 2.0 * tan (r_refdef.fov_x/360*M_PI);
	r_refdef.fvrectx = (float)r_refdef.vrect.x;
	r_refdef.fvrectx_adj = (float)r_refdef.vrect.x - 0.5;
	r_refdef.vrect_x_adj_shift20 = (r_refdef.vrect.x<<20) + (1<<19) - 1;
	r_refdef.fvrecty = (float)r_refdef.vrect.y;
	r_refdef.fvrecty_adj = (float)r_refdef.vrect.y - 0.5;
	r_refdef.vrectright = r_refdef.vrect.x + r_refdef.vrect.width;
	r_refdef.vrectright_adj_shift20 = (r_refdef.vrectright<<20) + (1<<19) - 1;
	r_refdef.fvrectright = (float)r_refdef.vrectright;
	r_refdef.fvrectright_adj = (float)r_refdef.vrectright - 0.5;
	r_refdef.vrectrightedge = (float)r_refdef.vrectright - 0.99;
	r_refdef.vrectbottom = r_refdef.vrect.y + r_refdef.vrect.height;
	r_refdef.fvrectbottom = (float)r_refdef.vrectbottom;
	r_refdef.fvrectbottom_adj = (float)r_refdef.vrectbottom - 0.5;

	r_refdef.aliasvrect.x = (int)(r_refdef.vrect.x * r_aliasuvscale);
	r_refdef.aliasvrect.y = (int)(r_refdef.vrect.y * r_aliasuvscale);
	r_refdef.aliasvrect.width = (int)(r_refdef.vrect.width * r_aliasuvscale);
	r_refdef.aliasvrect.height = (int)(r_refdef.vrect.height * r_aliasuvscale);
	r_refdef.aliasvrectright = r_refdef.aliasvrect.x +
			r_refdef.aliasvrect.width;
	r_refdef.aliasvrectbottom = r_refdef.aliasvrect.y +
			r_refdef.aliasvrect.height;

	pixelAspect = aspect;
	xOrigin = r_refdef.xOrigin;
	yOrigin = r_refdef.yOrigin;

	screenAspect = r_refdef.vrect.width*pixelAspect /
			r_refdef.vrect.height;
// 320*200 1.0 pixelAspect = 1.6 screenAspect
// 320*240 1.0 pixelAspect = 1.3333 screenAspect
// proper 320*200 pixelAspect = 0.8333333

	verticalFieldOfView = r_refdef.horizontalFieldOfView / screenAspect;

// values for perspective projection
// if math were exact, the values would range from 0.5 to to range+0.5
// hopefully they wll be in the 0.000001 to range+.999999 and truncate
// the polygon rasterization will never render in the first row or column
// but will definately render in the [range] row and column, so adjust the
// buffer origin to get an exact edge to edge fill
	xcenter = ((float)r_refdef.vrect.width * XCENTERING) +
			r_refdef.vrect.x - 0.5;
	aliasxcenter = xcenter * r_aliasuvscale;
	ycenter = ((float)r_refdef.vrect.height * YCENTERING) +
			r_refdef.vrect.y - 0.5;
	aliasycenter = ycenter * r_aliasuvscale;

	xscale = r_refdef.vrect.width / r_refdef.horizontalFieldOfView;
	aliasxscale = xscale * r_aliasuvscale;
	xscaleinv = 1.0 / xscale;
	yscale = xscale * pixelAspect;
	aliasyscale = yscale * r_aliasuvscale;
	yscaleinv = 1.0 / yscale;
	xscaleshrink = (r_refdef.vrect.width-6)/r_refdef.horizontalFieldOfView;
	yscaleshrink = xscaleshrink*pixelAspect;

// left side clip
	screenedge[0].normal[0] = -1.0 / (xOrigin*r_refdef.horizontalFieldOfView);
	screenedge[0].normal[1] = 0;
	screenedge[0].normal[2] = 1;
	screenedge[0].type = PLANE_ANYZ;

// right side clip
	screenedge[1].normal[0] =
			1.0 / ((1.0-xOrigin)*r_refdef.horizontalFieldOfView);
	screenedge[1].normal[1] = 0;
	screenedge[1].normal[2] = 1;
	screenedge[1].type = PLANE_ANYZ;

// top side clip
	screenedge[2].normal[0] = 0;
	screenedge[2].normal[1] = -1.0 / (yOrigin*verticalFieldOfView);
	screenedge[2].normal[2] = 1;
	screenedge[2].type = PLANE_ANYZ;

// bottom side clip
	screenedge[3].normal[0] = 0;
	screenedge[3].normal[1] = 1.0 / ((1.0-yOrigin)*verticalFieldOfView);
	screenedge[3].normal[2] = 1;
	screenedge[3].type = PLANE_ANYZ;

	for (i=0 ; i<4 ; i++)
		VectorNormalize (screenedge[i].normal);

	res_scale = sqrt ((double)(r_refdef.vrect.width * r_refdef.vrect.height) /
			          (320.0 * 152.0)) *
			(2.0 / r_refdef.horizontalFieldOfView);
	r_aliastransition = r_aliastransbase.value * res_scale;
	r_resfudge = r_aliastransadj.value * res_scale;

	if (scr_fov.value <= 90.0)
		r_fov_greater_than_90 = false;
	else
		r_fov_greater_than_90 = true;

// TODO: collect 386-specific code in one place
#if	id386
	if (r_pixbytes == 1)
	{
		Sys_MakeCodeWriteable ((long)R_Surf8Start,
						     (long)R_Surf8End - (long)R_Surf8Start);
		colormap = colorshadingmap_world;//vid.colormap; // mankrip
		R_Surf8Patch ();
	}
	else
	{
		Sys_MakeCodeWriteable ((long)R_Surf16Start,
						     (long)R_Surf16End - (long)R_Surf16Start);
		colormap = vid.colormap16;
		R_Surf16Patch ();
	}
#endif	// id386

	D_ViewChanged ();
}


/*
===============
R_MarkLeaves
===============
*/
void R_MarkLeaves (void)
{
	byte	*vis;
	mnode_t	*node;
	int		i;

	if (r_oldviewleaf == r_viewleaf)
		return;

	r_visframecount++;
	r_oldviewleaf = r_viewleaf;

	vis = Mod_LeafPVS (r_viewleaf, cl.worldmodel);

	for (i=0 ; i<cl.worldmodel->numleafs ; i++)
	{
		if (vis[i>>3] & (1<<(i&7)))
		{
			node = (mnode_t *)&cl.worldmodel->leafs[i+1];
			do
			{
				if (node->visframe == r_visframecount)
					break;
				node->visframe = r_visframecount;
				node = node->parent;
			} while (node);
		}
	}
}


// reckless (inside3d.com) - depth sorting - begin
/*
=============
R_DepthSortEntities

sort entities by depth.
=============
*/
int R_DepthSortEntityPair (const void *a, const void *b)
{
	entity_t
		*enta = *( (entity_t **) a)
	,	*entb = *( (entity_t **) b)
		;

	// sort back to front
	if (enta->distance > entb->distance)
		return -1;
	if (enta->distance < entb->distance)
		return 1;
	return 0;
}

void R_DepthSortEntities (void)
{
	// if there's only one then the list is already sorted!
	if (cl_numblendededicts > 1)
	{
		int i;
		entity_t *ent;

		// evaluate distances
		for (i = 0; i < cl_numblendededicts; i++)
		{
			ent = cl_blendededicts[i];

			// set distance from viewer - no need to sqrt them as the order will be the same
			ent->distance =
				(ent->origin[0] - r_origin[0]) * (ent->origin[0] - r_origin[0])
			+	(ent->origin[1] - r_origin[1]) * (ent->origin[1] - r_origin[1])
			+	(ent->origin[2] - r_origin[2]) * (ent->origin[2] - r_origin[2])
				;
		}

		if (cl_numblendededicts == 2) // trivial case - 2 entities
		{
			if (cl_blendededicts[0]->distance < cl_blendededicts[1]->distance)
			{
				ent = cl_blendededicts[1];

				// reorder correctly
				cl_blendededicts[1] = cl_blendededicts[0];
				cl_blendededicts[0] = ent;
			}
		}
		else // general case - depth sort the transedicts from back to front
			qsort ( (void *) cl_blendededicts, cl_numblendededicts, sizeof (entity_t *), R_DepthSortEntityPair);
	}
}
// reckless (inside3d.com) - depth sorting - end

/*
=============
R_DrawEntitiesOnList
=============
*/
extern qboolean chase_nodraw; // mankrip - mh's chasecam fix
void R_DrawEntity (void)
{
	if (currententity == d_externalviewentity) // mankrip
	// 2000-01-09 ChaseCam fix by FrikaC  start
	{
		if (!chase_active.value)
	// 2000-01-09 ChaseCam fix by FrikaC  end
			return;	// don't draw the player
		if (chase_nodraw)
			return; // mankrip - mh's chasecam fix
		// mankrip - chase camera translucency hack - begin
		if (currententity->effects & (EF_SHADOW | EF_ADDITIVE) || currententity->alpha < 1.0f)
			return;
		// mankrip - chase camera translucency hack - end
	// 2000-01-09 ChaseCam fix by FrikaC  start
		currententity->angles[0] = 0;
		currententity->angles[2] = 0;
	}
	// 2000-01-09 ChaseCam fix by FrikaC  end

	switch (currententity->model->type)
	{
	case mod_sprite:
		VectorCopy (currententity->origin, r_entorigin);
		VectorSubtract (r_origin, r_entorigin, modelorg);
		R_DrawSprite ();
		break;

	case mod_alias:
		VectorCopy (currententity->origin, r_entorigin);
		VectorSubtract (r_origin, r_entorigin, modelorg);
		R_AliasDrawModel (); // mankrip - edited
		break;

	default:
		return;
	}
}

// mankrip - begin
void R_DrawBlendedEntity (void)
{
	static int			j, k, clipflags;
	static vec3_t		oldorigin;
	static model_t		*clmodel;
	static float		minmaxs[6];

	if (currententity == d_externalviewentity) // mankrip
	// 2000-01-09 ChaseCam fix by FrikaC  start
	{
		if (!chase_active.value)
	// 2000-01-09 ChaseCam fix by FrikaC  end
			return;	// don't draw the player
		if (chase_nodraw)
			return; // mankrip - mh's chasecam fix
		// mankrip - chase camera translucency hack
		if (currententity->D_PolysetDrawSpans != MDL_DrawSpans_Blend
		&& currententity->D_PolysetDrawSpans != MDL_DrawSpans_BlendBackwards)
			return;
	// 2000-01-09 ChaseCam fix by FrikaC  start
		currententity->angles[0] = 0;
		currententity->angles[2] = 0;
	}
	// 2000-01-09 ChaseCam fix by FrikaC  end

	colorblendingmap = currententity->colorblendingmap;
	switch (currententity->model->type)
	{
		case mod_sprite:
			VectorCopy (currententity->origin, r_entorigin);
			VectorSubtract (r_origin, r_entorigin, modelorg);
			R_DrawSprite ();
			break;

		case mod_alias:
			VectorCopy (currententity->origin, r_entorigin);
			VectorSubtract (r_origin, r_entorigin, modelorg);
			R_AliasDrawModel (); // mankrip - edited
			break;

		case mod_brush:
			d_drawturb  = currententity->D_DrawTurbulent;
			#if 1
			{
				edge_t	ledges[NUMSTACKEDGES +
							((CACHE_SIZE - 1) / sizeof(edge_t)) + 1];
				surf_t	lsurfs[NUMSTACKSURFACES +
							((CACHE_SIZE - 1) / sizeof(surf_t)) + 1];

				if (auxedges)
				{
					r_edges = auxedges;
				}
				else
				{
					r_edges =  (edge_t *)
							(((long)&ledges[0] + CACHE_SIZE - 1) & ~(CACHE_SIZE - 1));
				}

				if (r_surfsonstack)
				{
					surfaces =  (surf_t *)
							(((long)&lsurfs[0] + CACHE_SIZE - 1) & ~(CACHE_SIZE - 1));
					surf_max = &surfaces[r_cnumsurfs];
				// surface 0 doesn't really exist; it's just a dummy because index 0
				// is used to indicate no edge attached to surface
					surfaces--;
					#if	id386 // mankrip - no 386 assembler, no MASM
					R_SurfacePatch ();
					#endif // mankrip - no 386 assembler, no MASM
				}
			}
			#endif
			R_BeginEdgeFrame ();

			// only the world can be drawn back to front with no z reads or compares, just
			// z writes, so have the driver turn z compares on now
			D_TurnZOn ();

			clmodel = currententity->model;

			VectorCopy (modelorg, oldorigin); // TODO: shoul be done only once for all BSP entities
			// see if the bounding box lets us trivially reject, also sets trivial accept status
			minmaxs[0  ] = currententity->origin[0] + clmodel->mins[0];
			minmaxs[1  ] = currententity->origin[1] + clmodel->mins[1];
			minmaxs[2  ] = currententity->origin[2] + clmodel->mins[2];
			minmaxs[3+0] = currententity->origin[0] + clmodel->maxs[0];
			minmaxs[3+1] = currententity->origin[1] + clmodel->maxs[1];
			minmaxs[3+2] = currententity->origin[2] + clmodel->maxs[2];
			clipflags = R_BmodelCheckBBox (clmodel, minmaxs);

			if (clipflags != BMODEL_FULLY_CLIPPED)
			{
				VectorCopy (currententity->origin, r_entorigin);
				VectorSubtract (r_origin, r_entorigin, modelorg);
				// FIXME: is this needed?
			//	VectorCopy (modelorg, r_worldmodelorg); // mankrip - removed

				r_pcurrentvertbase = clmodel->vertexes;

				// FIXME: stop transforming twice
				R_RotateBmodel ();

				// calculate dynamic lighting for bmodel if it's not an instanced model
				if (clmodel->firstmodelsurface != 0)
				{
					for (k=0 ; k<MAX_DLIGHTS ; k++)
					{
						if ( (cl_dlights[k].die < cl.time) ||
							(!cl_dlights[k].radius))
						{
							continue;
						}

						R_MarkLights (&cl_dlights[k], 1<<k, clmodel->nodes + clmodel->hulls[0].firstclipnode);
					}
				}
				// mankrip - begin
				currententity->lightlevel = 1.0f;
				if (r_externbsp_lit.value)
					if (currententity->model->name[0] != '*')
					{
						int
							shadelight
						,	lnum
							;
						dlight_t
							*dl
							;
						vec3_t
							dist
						,	t
							;
						float
							add
							;
						VectorCopy (currententity->origin, t);
						// actually, the center of BSP models isn't at the origin of the entity
						t[0] += currententity->model->radius/4; // this 4 should probably be a pi
						t[1] += currententity->model->radius/4; // this 4 should probably be a pi
						shadelight = R_LightPoint (t); // ambient light
						// add dynamic lights
						#if 1
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
									shadelight += (int)add;
							}
						}
						#endif
						// minimum of (255 minus) 16 (8 less than in the viewmodel), maximum of (255 minus) 192 (same as MDL models)
						currententity->lightlevel = (shadelight > 192) ? 1.0f : ( (shadelight < 16) ? (16.0f / 192.0f) : ( (float)shadelight / 192.0f));
					}
				// mankrip - end

				// if the driver wants polygons, deliver those.
				// Z-buffering is on at this point, so no clipping to the world tree is needed, just frustum clipping
				if (r_drawpolys | r_drawculledpolys)
				{
					R_ZDrawSubmodelPolys (clmodel);
				}
				else
				{
					r_pefragtopnode = NULL;

					for (j=0 ; j<3 ; j++)
					{
						r_emins[j] = minmaxs[j];
						r_emaxs[j] = minmaxs[3+j];
					}

					R_SplitEntityOnNode2 (cl.worldmodel->nodes);

					if (r_pefragtopnode)
					{
						currententity->topnode = r_pefragtopnode;

						if (r_pefragtopnode->contents >= 0)
						{
							// not a leaf; has to be clipped to the world BSP
							r_clipflags = clipflags;
							R_DrawSolidClippedSubmodelPolygons (clmodel);
						}
						else
						{
							// falls entirely in one leaf,
							// so we just put all the edges in the edge list and let 1/z sorting handle drawing order
							R_DrawSubmodelPolygons (clmodel, clipflags);
						}
						currententity->topnode = NULL;
					}
					R_ScanEdges (); // mankrip
				}
				// put back world rotation and frustum clipping
				// FIXME: R_RotateBmodel should just work off base_vxx
				VectorCopy (base_vpn, vpn);
				VectorCopy (base_vup, vup);
				VectorCopy (base_vright, vright);
			//	VectorCopy (base_modelorg, modelorg);
				VectorCopy (oldorigin, modelorg);
// build the transformation matrix for the given view angles
	VectorCopy (r_refdef.vieworg, modelorg);
	VectorCopy (r_refdef.vieworg, r_origin);

	AngleVectors (r_refdef.viewangles, vpn, vright, vup);
				R_TransformFrustum ();
			}
		//	if (!(r_drawpolys | r_drawculledpolys))
		//		R_ScanEdges (); // mankrip - wrong place?
			break;

		default:
			return;
	}
}
// mankrip - end

void R_DrawEntitiesOnList (void)
{
	// mankrip - begin
//	if ( ( (int)r_drawentities.value) & 1)
	if (r_drawentities.value)
	{
		int i;
		for (i = 0 ; i < cl_numvisedicts ; i++)
		{
			currententity = cl_visedicts[i];

			if (currententity->effects & EF_CELSHADING)
			{
				#if 0
				vec3_t dist;
				float distlen;

				currententity->colormap = &colorshadingmap[CEL_WITH_BRIGHTS][256];
				R_DrawEntity ();

				VectorSubtract (r_refdef.vieworg, currententity->origin, dist);
				distlen = Length(dist);
				if (distlen < 250.0)
					r_drawoutlines = 1;
				else if (distlen < 350.0)
					r_drawoutlines = 6;
				else if (distlen < 420.0) // 450
					r_drawoutlines = 3;
				else
					r_drawoutlines = false;
				#else
					r_drawoutlines = 1;
				#endif

				R_DrawEntity ();
				r_drawoutlines = false;
			}
			else
				R_DrawEntity ();
		}
	}
	// mankrip - end
}

/*
=============
R_DrawViewModel
=============
*/
void R_DrawViewModel (int opaque)
{
	// mankrip - begin
	d_internalviewentity->alpha = (cl.items & IT_INVISIBILITY && d_externalviewentity->alpha == 1) ? 0.3f : d_externalviewentity->alpha;
	if (opaque != ((d_internalviewentity->alpha == 1.0f) && !(d_externalviewentity->effects & EF_ADDITIVE))) // if expecting a opaque viewmodel when the viewmodel isn't opaque
		return;

	if (!d_internalviewentity->model
	||	cl.stats[STAT_HEALTH] <= 0
	||	chase_active.value
	|| !r_drawviewmodel.value
//	||	r_fov_greater_than_90
		)
	{
		d_internalviewentity->reset_frame_interpolation = true;
		return;
	}

	currententity = d_internalviewentity;

	d_internalviewentity->effects = d_externalviewentity->effects;
	d_internalviewentity->scale2 = d_externalviewentity->scale2;
	VectorCopy (d_externalviewentity->scalev, d_internalviewentity->scalev);

	VectorCopy (d_internalviewentity->origin, r_entorigin);
	VectorSubtract (r_origin, r_entorigin, modelorg);

	if (r_dspeeds.value)
		dv_time1 = Sys_FloatTime (); // draw viewmodel time

	r_drawoutlines = d_internalviewentity->effects & EF_CELSHADING;
	currententity->D_PolysetDrawSpans = (!opaque && d_externalviewentity->D_PolysetDrawSpans == MDL_DrawSpans_ColorKeyed) ? MDL_DrawSpans_BlendBackwards : d_externalviewentity->D_PolysetDrawSpans;
	colorblendingmap = /* currententity->colorblendingmap = */ (cl.items & ( /* IT_INVULNERABILITY | */ IT_QUAD)) ? colorblendingmode[BLEND_ADDITIVE] : d_externalviewentity->colorblendingmap;
	R_AliasDrawModel ();
	r_drawoutlines = false;

	if (r_dspeeds.value)
		dv_time2 = Sys_FloatTime (); // draw viewmodel time
	// mankrip - end
}


/*
=============
R_BmodelCheckBBox
=============
*/
int R_BmodelCheckBBox (model_t *clmodel, float *minmaxs)
{
	int			i, *pindex, clipflags;
	vec3_t		acceptpt, rejectpt;
	double		d;

	clipflags = 0;

	if (currententity->angles[0] || currententity->angles[1]
		|| currententity->angles[2])
	{
		for (i=0 ; i<4 ; i++)
		{
			d = DotProduct (currententity->origin, view_clipplanes[i].normal);
			d -= view_clipplanes[i].dist;

			if (d <= -clmodel->radius)
				return BMODEL_FULLY_CLIPPED;

			if (d <= clmodel->radius)
				clipflags |= (1<<i);
		}
	}
	else
	{
		for (i=0 ; i<4 ; i++)
		{
		// generate accept and reject points
		// FIXME: do with fast look-ups or integer tests based on the sign bit
		// of the floating point values

			pindex = pfrustum_indexes[i];

			rejectpt[0] = minmaxs[pindex[0]];
			rejectpt[1] = minmaxs[pindex[1]];
			rejectpt[2] = minmaxs[pindex[2]];

			d = DotProduct (rejectpt, view_clipplanes[i].normal);
			d -= view_clipplanes[i].dist;

			if (d <= 0)
				return BMODEL_FULLY_CLIPPED;

			acceptpt[0] = minmaxs[pindex[3+0]];
			acceptpt[1] = minmaxs[pindex[3+1]];
			acceptpt[2] = minmaxs[pindex[3+2]];

			d = DotProduct (acceptpt, view_clipplanes[i].normal);
			d -= view_clipplanes[i].dist;

			if (d <= 0)
				clipflags |= (1<<i);
		}
	}

	return clipflags;
}


/*
=============
R_DrawBEntitiesOnList
=============
*/
void R_DrawBEntitiesOnList (void)
{
	int			i, j, k, clipflags;
	vec3_t		oldorigin;
	model_t		*clmodel;
	float		minmaxs[6];

//	if (! ( ( (int)r_drawentities.value) & 8))
	if (!r_drawentities.value)
		return;

	VectorCopy (modelorg, oldorigin);
	insubmodel = true;
	r_dlightframecount = r_framecount;

	for (i=0 ; i<cl_numvisedicts ; i++)
	{
		currententity = cl_visedicts[i];

		switch (currententity->model->type)
		{
		case mod_brush:

			clmodel = currententity->model;

		// see if the bounding box lets us trivially reject, also sets
		// trivial accept status
			for (j=0 ; j<3 ; j++)
			{
				minmaxs[j] = currententity->origin[j] +
						clmodel->mins[j];
				minmaxs[3+j] = currententity->origin[j] +
						clmodel->maxs[j];
			}

			clipflags = R_BmodelCheckBBox (clmodel, minmaxs);

			if (clipflags != BMODEL_FULLY_CLIPPED)
			{
				VectorCopy (currententity->origin, r_entorigin);
				VectorSubtract (r_origin, r_entorigin, modelorg);
			// FIXME: is this needed?
				VectorCopy (modelorg, r_worldmodelorg); // mankrip - removed

				r_pcurrentvertbase = clmodel->vertexes;

			// FIXME: stop transforming twice
				R_RotateBmodel ();

			// calculate dynamic lighting for bmodel if it's not an
			// instanced model
				if (clmodel->firstmodelsurface != 0)
				{
					for (k=0 ; k<MAX_DLIGHTS ; k++)
					{
						if ((cl_dlights[k].die < cl.time) ||
							(!cl_dlights[k].radius))
						{
							continue;
						}

						R_MarkLights (&cl_dlights[k], 1<<k,
							clmodel->nodes + clmodel->hulls[0].firstclipnode);
					}
				}
				// mankrip - begin
				currententity->lightlevel = 1.0f;
				if (r_externbsp_lit.value)
					if (currententity->model->name[0] != '*')
					{
						int
							shadelight
						,	lnum
							;
						dlight_t
							*dl
							;
						vec3_t
							dist
						,	t
							;
						float
							add
							;
						VectorCopy (currententity->origin, t);
						// actually, the center of BSP models isn't at the origin of the entity
						t[0] += currententity->model->radius/4; // this 4 should probably be a pi
						t[1] += currententity->model->radius/4; // this 4 should probably be a pi
						shadelight = R_LightPoint (t); // ambient light
						// add dynamic lights
						#if 1
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
									shadelight += (int)add;
							}
						}
						#endif
						// minimum of (255 minus) 16 (8 less than in the viewmodel), maximum of (255 minus) 192 (same as MDL models)
						currententity->lightlevel = (shadelight > 192) ? 1.0f : ( (shadelight < 16) ? (16.0f / 192.0f) : ( (float)shadelight / 192.0f));
					}
				// mankrip - end

			// if the driver wants polygons, deliver those. Z-buffering is on
			// at this point, so no clipping to the world tree is needed, just
			// frustum clipping
				if (r_drawpolys | r_drawculledpolys)
				{
					R_ZDrawSubmodelPolys (clmodel);
				}
				else
				{
					r_pefragtopnode = NULL;

					for (j=0 ; j<3 ; j++)
					{
						r_emins[j] = minmaxs[j];
						r_emaxs[j] = minmaxs[3+j];
					}

					R_SplitEntityOnNode2 (cl.worldmodel->nodes);

					if (r_pefragtopnode)
					{
						currententity->topnode = r_pefragtopnode;

						if (r_pefragtopnode->contents >= 0)
						{
						// not a leaf; has to be clipped to the world BSP
							r_clipflags = clipflags;
							R_DrawSolidClippedSubmodelPolygons (clmodel);
						}
						else
						{
						// falls entirely in one leaf, so we just put all the
						// edges in the edge list and let 1/z sorting handle
						// drawing order
							R_DrawSubmodelPolygons (clmodel, clipflags);
						}

						currententity->topnode = NULL;
					}
				}

			// put back world rotation and frustum clipping
			// FIXME: R_RotateBmodel should just work off base_vxx
				VectorCopy (base_vpn, vpn);
				VectorCopy (base_vup, vup);
				VectorCopy (base_vright, vright);
				VectorCopy (base_modelorg, modelorg);
				VectorCopy (oldorigin, modelorg);
				R_TransformFrustum ();
			}

			break;

		default:
			break;
		}
	}

	insubmodel = false;
}

// mankrip - begin
void R_DrawShadowEntitiesOnList (void)
{
//	if ( ( (int)r_drawentities.value) & 4)
	if (r_drawentities.value)
	{
		int i;

		r_dlightframecount = r_framecount;
		insubmodel = true;

		for (i = 0 ; i < cl_numshadowedicts ; i++)
		{
			currententity = cl_shadowedicts[i];
			if (currententity->effects & EF_SHADOW)
				R_DrawBlendedEntity ();
		}
		insubmodel = false;
	}
}

void R_DrawBlendedEntitiesOnList (void)
{
//	if ( ( (int)r_drawentities.value) & 2)
	if (r_drawentities.value)
	{
		int i;

		r_dlightframecount = r_framecount;
		insubmodel = true;

		for (i = 0 ; i < cl_numblendededicts ; i++)
		{
			currententity = cl_blendededicts[i];
			if (currententity->effects & EF_ADDITIVE || currententity->alpha < 1.0f)
			{
				if (currententity->effects & EF_SHADOW)
					continue;
				R_DrawBlendedEntity ();
			}
		}
		insubmodel = false;
	}
}
// mankrip - end

/*
================
R_EdgeDrawing
================
*/
void R_EdgeDrawing (void)
{
	edge_t	ledges[NUMSTACKEDGES	+ ((CACHE_SIZE - 1) / sizeof(edge_t)) + 1];
	surf_t	lsurfs[NUMSTACKSURFACES	+ ((CACHE_SIZE - 1) / sizeof(surf_t)) + 1];

	r_edges = (auxedges) ? auxedges : (edge_t *) ( ( (long)&ledges[0] + CACHE_SIZE - 1) & ~(CACHE_SIZE - 1));

	if (r_surfsonstack)
	{
		surfaces = (surf_t *) ( ( (long)&lsurfs[0] + CACHE_SIZE - 1) & ~(CACHE_SIZE - 1));
		surf_max = &surfaces[r_cnumsurfs];
		surfaces--; // surface 0 doesn't really exist; it's just a dummy because index 0 is used to indicate no edge attached to surface
		#if	id386 // mankrip - no 386 assembler, no MASM
		R_SurfacePatch ();
		#endif // mankrip - no 386 assembler, no MASM
	}
	R_BeginEdgeFrame ();

	if (r_dspeeds.value)
	{
		rw_time1 = Sys_FloatTime ();
	}

	R_RenderWorld ();

	if (r_drawculledpolys)
		R_ScanEdges ();

// only the world can be drawn back to front with no z reads or compares, just
// z writes, so have the driver turn z compares on now
	D_TurnZOn ();

	if (!r_overdraw) // mankrip
	{
		if (r_dspeeds.value)
		{
			rw_time2 = Sys_FloatTime ();
			db_time1 = rw_time2;
		}

		R_DrawBEntitiesOnList ();

		if (r_dspeeds.value)
		{
			db_time2 = Sys_FloatTime ();
			se_time1 = db_time2;
		}

		if (!r_dspeeds.value)
		{
			VID_UnlockBuffer ();
			S_ExtraUpdate ();	// don't let sound get messed up if going slow
			VID_LockBuffer ();
		}
	}
	if (!(r_drawpolys | r_drawculledpolys))
		R_ScanEdges ();
}

// mankrip - begin
void D_SetTurbulentMode (int mode, int map, float alpha)
{
	mode = minmaxclamp (0, mode, 2);
	if (mode)//(mode == 2)
	{
		colorblendingmap = colorblendingmode[minmaxclamp (0, map, COLORBLENDING_MAPS - 1)];
		if (alpha < 0.5f)
			d_drawturb = D_DrawTurbulent16_BlendBackwards;
		else
			d_drawturb = D_DrawTurbulent16_Blend;
	}
	/*
	else if (mode == 1)
	{
			d_drawturb = D_DrawTurbulent16_Stipple;
	}
	*/
	else
	{
		colorblendingmap = colorblendingmode[BLEND_WEIGHTEDMEAN];
		if (r_wateralpha.value < 0.5f)
			d_drawturb = D_DrawTurbulent16_BlendBackwards;
		else
			d_drawturb = D_DrawTurbulent16_Blend;
	}
}
// mankrip - end
/*
================
R_RenderView

r_refdef must be set before the first call
================
*/
byte	*warpbuffer = NULL; // mankrip - hi-res waterwarp & buffered video
void R_RenderView_ (void)
{
//	byte	warpbuffer[WARP_WIDTH * WARP_HEIGHT]; // mankrip - hi-res waterwarp & buffered video - removed
	// mankrip - hi-res waterwarp & buffered video - begin
	if (warpbuffer)
		free(warpbuffer);
	warpbuffer = malloc (abs (vid.rowbytes) * vid.height);
	// mankrip - hi-res waterwarp & buffered video - end

	r_warpbuffer = warpbuffer;

	if (r_timegraph.value || r_speeds.value || r_dspeeds.value)
		r_time1 = Sys_FloatTime ();

	R_SetupFrame ();

#ifdef PASSAGES
SetVisibilityByPassages ();
#else
	R_MarkLeaves ();	// done here so we know if we're in water
#endif

// make FDIV fast. This reduces timing precision after we've been running for a
// while, so we don't do it globally.  This also sets chop mode, and we do it
// here so that setup stuff like the refresh area calculations match what's
// done in screen.c
	Sys_LowFPPrecision ();

	if (!cl_entities[0].model || !cl.worldmodel)
		Sys_Error ("R_RenderView: NULL worldmodel");

	if (!r_dspeeds.value)
	{
		VID_UnlockBuffer ();
		S_ExtraUpdate ();	// don't let sound get messed up if going slow
		VID_LockBuffer ();
	}

	// mankrip - begin
	d_externalviewentity = &cl_entities[cl.viewentity];
	d_internalviewentity = &cl.viewent;
	R_DepthSortEntities ();
	r_overdraw = r_foundtranslucency = false;
	r_overdrawindex = 0;
	memset (r_overdrawstep, 0, sizeof (int) * 5);
	d_drawturb = D_DrawTurbulent16_Solid;
#if 0
	if (((int)developer.value & 64)) // for betatesting
	{
		extern short		*d_pzbuffer;
		Q_memset (d_pzbuffer, 0, vid.width * vid.height * sizeof(d_pzbuffer));
		Q_memset (vid.buffer, 8, vid.width * vid.height);
		Q_memset (r_warpbuffer, 8, vid.width * vid.height);
	}
	else
#endif
	// mankrip - end
	/*
	// mankrip - Drawing order:
	world		(solid surfaces)
	BSP models	(solid surfaces)
	viewmodel	(solid)
	TEnts (?)
	BSP models	(color-keyed, must overlap on same entity)
	entities	(solid, color-keyed)
	entities	(EF_SHADOW, color-keyed)
	particles	(solid)
	entities	(translucent, color-keyed, overlapping surfaces on same entity, Z-based no-overlapping pixels on same entity)
	particles	(translucent)
	world (translucent surfaces)
	viewmodel (translucent)
	*/
	R_EdgeDrawing ();

	if (!r_dspeeds.value)
	{
		VID_UnlockBuffer ();
		S_ExtraUpdate ();	// don't let sound get messed up if going slow
		VID_LockBuffer ();
	}

	if (r_dspeeds.value)
	{
		se_time2 = Sys_FloatTime (); // scan edges time
//		de_time1 = se_time2; // draw entities time
	}
	R_DrawViewModel (true);

	if (r_dspeeds.value) // mankrip
		de_time1 = Sys_FloatTime (); // mankrip - draw entities time
	CL_UpdateTEnts (); // mankrip
	R_DrawEntitiesOnList ();

	r_overdraw = true;
	R_DrawShadowEntitiesOnList (); // mankrip
	r_overdraw = false;

	if (r_dspeeds.value)
	{
		de_time2 = Sys_FloatTime (); // draw entities time
		dp_time1 = de_time2; // draw particles time
	}
	colormap = vid.colormap = &colorshadingmap[WITH_BRIGHTS][256]; // mankrip
	R_DrawParticles ();

	if (r_dspeeds.value)
		dp_time2 = Sys_FloatTime (); // draw particles time
	r_overdraw = true;
	R_DrawBlendedEntitiesOnList (); // mankrip
	r_overdraw = false;

#if 1
	// mankrip - begin
	if (r_foundtranslucency)
	{
		#if 0
		int i = 0;
		r_overdraw = true;
		while (i < r_overdrawindex)
		{
			switch (r_overdrawstep[i++])
		#else
		r_overdraw = true;
		while ( (--r_overdrawindex) > -1)
		{
			switch (r_overdrawstep[r_overdrawindex])
		#endif
			{
				case SURF_BLEND_WATER:	D_SetTurbulentMode ( (int)	r_water_blend_mode.value, (int)  r_water_blend_map.value,  r_water_blend_alpha.value); break;
				case SURF_BLEND_SLIME:	D_SetTurbulentMode ( (int)	r_slime_blend_mode.value, (int)  r_slime_blend_map.value,  r_slime_blend_alpha.value); break;
				case SURF_BLEND_LAVA:	D_SetTurbulentMode ( (int)	 r_lava_blend_mode.value, (int)   r_lava_blend_map.value,   r_lava_blend_alpha.value); break;
				case SURF_BLEND_PORTAL:	D_SetTurbulentMode ( (int) r_portal_blend_mode.value, (int) r_portal_blend_map.value, r_portal_blend_alpha.value); break;
				case SURF_DRAWTURB:		D_SetTurbulentMode ( 1, BLEND_WEIGHTEDMEAN, r_wateralpha.value); break;
				default: break;
			}
			R_EdgeDrawing ();
		}
	}
#endif

	R_DrawViewModel (false);
	colormap = vid.colormap = &colorshadingmap[WITH_BRIGHTS][256];

#if 0
	// fog
	if (developer.value >= 100)
	if (developer.value < 356)
	{
		int			x, y, level, fogcolor=(int)developer.value-100;
		byte		*pbuf, *fogshadingmap=&colorshadingmap[WITHOUT_BRIGHTS][256];
		short		*pz;
		extern short		*d_pzbuffer;
		extern unsigned int	d_zwidth;
		extern int			d_scantable[1024];
		for (y=0 ; y<r_refdef.vrect.height/*vid.height*/ ; y++)
		{
		//	pbuf = (byte *)(vid.buffer + vid.rowbytes*y);

			for (x=0 ; x<r_refdef.vrect.width/*vid.width*/ ; x++)
			{
				pz = d_pzbuffer + (d_zwidth * (y+r_refdef.vrect.y)) + x+r_refdef.vrect.x;
			//*
				level = 64 - (int)(*pz * (32.0/256.0));
			//	level = fabs(level);
				if (level > 31 && level < 64)
				{
#ifndef _WIN32 // mankrip - buffered video (bloody hack)
					if (!r_dowarp)
						pbuf = vid.buffer + d_scantable[y+r_refdef.vrect.y] + x+r_refdef.vrect.x;
					else
#endif // mankrip - buffered video (bloody hack)
						pbuf = r_warpbuffer + d_scantable[y+r_refdef.vrect.y] + x+r_refdef.vrect.x;
				//	*pbuf = colorblendingmap[(int)colorshadingmap[WITHOUT_BRIGHTS][*pbuf + ((32+level) * 256)]*256];
					*pbuf = colorblendingmap[
						fogshadingmap[*pbuf + ((63 - level) * 256)]
					+	(int)fogshadingmap[
							fogcolor
						+	((63 - level) * 256)]
						*256];
				}
			/*/
				level = (int)(*pz * (32.0/256.0)) - 32;
			//	if (level >= 32)
			//		*pbuf = additivemap[*pbuf + (int)colorshadingmap[WITH_BRIGHTS][(int)developer.value-100 + ((64-31) * 256)]*256];
			//	else
				if (level > 0 && level < 32)
				{
					pbuf = r_warpbuffer + d_scantable[y] + x;
				//	*pbuf = additivemap[(int)colorshadingmap[WITHOUT_BRIGHTS][*pbuf + ((32+level) * 256)]*256];
					*pbuf = additivemap[*pbuf + (int)colorshadingmap[WITHOUT_BRIGHTS][(int)developer.value-100 + ((64-level) * 256)]*256];
				}
			//*/
			}
		}
	}
	// mankrip - end
#endif
	// mankrip - buffered video (bloody hack) - begin
#ifdef _WIN32
	if (r_stereo_separation.value)
	{
		int		i;
		byte	*src = r_warpbuffer + scr_vrect.y * vid.width;
		byte	*dest = vid.buffer + scr_vrect.y * vid.rowbytes;
//		for (i=0 ; i<vid.height ; i++, src += vid.width, dest += vid.rowbytes)
		for (i=0 ; i<scr_vrect.height ; i++, src += vid.width, dest += vid.rowbytes)
			Q_memcpy(dest, src, vid.width);
	}
	else if (r_dowarp)
		D_WarpScreen ();
	else // copy buffer to video
	{
		int		i;
		byte	*src = r_warpbuffer + scr_vrect.y * vid.width + scr_vrect.x;
		byte	*dest = vid.buffer + scr_vrect.y * vid.rowbytes + scr_vrect.x;
		for (i=0 ; i<scr_vrect.height ; i++, src += vid.width, dest += vid.rowbytes)
			Q_memcpy(dest, src, scr_vrect.width);
	}
#else
	if (r_stereo_separation.value && r_dowarp)
	{
		int		i;
		byte	*src = r_warpbuffer + scr_vrect.y * vid.width;
		byte	*dest = vid.buffer + scr_vrect.y * vid.rowbytes;
		for (i=0 ; i<scr_vrect.height ; i++, src += vid.width, dest += vid.rowbytes)
			Q_memcpy(dest, src, vid.width);
	}
	else
	// mankrip - buffered video (bloody hack) - end
	if (r_dowarp)
		D_WarpScreen ();
#endif // mankrip - buffered video (bloody hack)

	V_SetContentsColor (r_viewleaf->contents);

	if (r_timegraph.value)
		R_TimeGraph ();

	if (r_aliasstats.value)
		R_PrintAliasStats ();

	if (r_speeds.value)
		R_PrintTimes ();

	if (r_dspeeds.value)
		R_PrintDSpeeds ();

	if (r_reportsurfout.value && r_outofsurfaces)
		Con_Printf ("Short %d surfaces\n", r_outofsurfaces);

	if (r_reportedgeout.value && r_outofedges)
		Con_Printf ("Short roughly %d edges\n", r_outofedges * 2 / 3);

// back to high floating-point precision
	Sys_HighFPPrecision ();
}

void R_RenderView (void)
{
	int		dummy;
	int		delta;

	delta = (byte *)&dummy - r_stack_start;
	if (delta < -10000 || delta > 10000)
		Sys_Error ("R_RenderView: called without enough stack");

	if ( Hunk_LowMark() & 3 )
		Sys_Error ("Hunk is missaligned");

	if ( (long)(&dummy) & 3 )
		Sys_Error ("Stack is missaligned");

	if ( (long)(&r_warpbuffer) & 3 )
		Sys_Error ("Globals are missaligned");

	// mankrip - split screen - begin
	/*
	if (developer.value == 4)
	{
		r_viewchanged = true;
		temp1.value = 1;
		R_RenderView_ ();
		r_viewchanged = true;
		temp1.value = 2;
		R_RenderView_ ();
		r_viewchanged = true;
		temp1.value = 3;
		R_RenderView_ ();
		r_viewchanged = true;
		temp1.value = 4;
		R_RenderView_ ();
	}
	else
	//*/
	// mankrip - split screen - end
	R_RenderView_ ();
}

/*
================
R_InitTurb
================
*/
void R_InitTurb (void)
{
	int		i;

	for (i=0 ; i<(SIN_BUFFER_SIZE) ; i++)
	{
		sintable[i] = AMP + sin(i*3.14159*2/CYCLE)*AMP;
		intsintable[i] = AMP2 + sin(i*3.14159*2/CYCLE)*AMP2;	// AMP2, not 20
	}
}

