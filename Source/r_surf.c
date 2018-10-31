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
// r_surf.c: surface-related refresh code

#include "quakedef.h"
#include "r_local.h"

drawsurf_t	r_drawsurf;

int				lightleft, sourcesstep, blocksize, sourcetstep;
int				lightdelta, lightdeltastep;
int				lightright, lightleftstep, lightrightstep, blockdivshift;
unsigned		blockdivmask;
void			*prowdestbase;
unsigned char	*pbasesource;
int				surfrowbytes;	// used by ASM files
unsigned		*r_lightptr;
int				r_stepback;
int				r_lightwidth;
int				r_numhblocks, r_numvblocks;
unsigned char	*r_source, *r_sourcemax;

void R_DrawSurfaceBlock8_mip0 (void);
void R_DrawSurfaceBlock8_mip1 (void);
void R_DrawSurfaceBlock8_mip2 (void);
void R_DrawSurfaceBlock8_mip3 (void);

void R_DrawSurfaceBlock8_mip0_fullbright (void);
void R_DrawSurfaceBlock8_mip1_fullbright (void);
void R_DrawSurfaceBlock8_mip2_fullbright (void);
void R_DrawSurfaceBlock8_mip3_fullbright (void);

static void	(*surfmiptable[4])(void) = {
	R_DrawSurfaceBlock8_mip0,
	R_DrawSurfaceBlock8_mip1,
	R_DrawSurfaceBlock8_mip2,
	R_DrawSurfaceBlock8_mip3
};

static void	(*surfmiptable_fullbright[4])(void) = {
	R_DrawSurfaceBlock8_mip0_fullbright,
	R_DrawSurfaceBlock8_mip1_fullbright,
	R_DrawSurfaceBlock8_mip2_fullbright,
	R_DrawSurfaceBlock8_mip3_fullbright
};



unsigned		blocklights[18*18];

/*
===============
R_AddDynamicLights
===============
*/
void R_AddDynamicLights (void)
{
	msurface_t *surf;
	int			lnum;
	int			sd, td;
	float		dist, rad, minlight;
	vec3_t		impact, local;
	vec3_t		origin_for_ent; // mankrip - dynamic lights on moving brush models fix
	int			s, t;
	int			smax, tmax;
	mtexinfo_t	*tex;

	surf = r_drawsurf.surf;
	smax = (surf->extents[0]>>4)+1;
	tmax = (surf->extents[1]>>4)+1;
	tex = surf->texinfo;

	for (lnum=0 ; lnum<MAX_DLIGHTS ; lnum++)
	{
		if ( !(surf->dlightbits & (1<<lnum) ) )
			continue;		// not lit by this light

		rad = cl_dlights[lnum].radius;
		VectorSubtract (cl_dlights[lnum].origin, currententity->origin, origin_for_ent); // mankrip - dynamic lights on moving brush models fix
		dist = DotProduct (origin_for_ent, surf->plane->normal) - // mankrip - dynamic lights on moving brush models fix - edited
				surf->plane->dist;
		rad -= fabs(dist);
		minlight = cl_dlights[lnum].minlight;
		if (rad < minlight)
			continue;
		minlight = rad - minlight;

		// mankrip - dynamic lights on moving brush models fix - begin
		impact[0] = origin_for_ent[0] - surf->plane->normal[0]*dist;
		impact[1] = origin_for_ent[1] - surf->plane->normal[1]*dist;
		impact[2] = origin_for_ent[2] - surf->plane->normal[2]*dist;
		// mankrip - dynamic lights on moving brush models fix - end

		local[0] = DotProduct (impact, tex->vecs[0]) + tex->vecs[0][3];
		local[1] = DotProduct (impact, tex->vecs[1]) + tex->vecs[1][3];

		local[0] -= surf->texturemins[0];
		local[1] -= surf->texturemins[1];

		for (t = 0 ; t<tmax ; t++)
		{
			td = local[1] - t*16;
			if (td < 0)
				td = -td;
			for (s=0 ; s<smax ; s++)
			{
				sd = local[0] - s*16;
				if (sd < 0)
					sd = -sd;

				if (sd > td)
					dist = sd + (td>>1);
				else
					dist = td + (sd>>1);
				if (dist < minlight)
				{
					#ifdef QUAKE2
					if (cl_dlights[lnum].dark)
					{
						unsigned temp = (rad - dist)*256; // mankrip
						int i = t*smax + s;
						if (blocklights[i] > temp)
							blocklights[i] -= temp;
						else
							blocklights[i] = 0;
					}
					else
					#endif
						blocklights[t*smax + s] += (rad - dist)*256;
				}
			}
		}
	}
}

/*
===============
R_BuildLightMap

Combine and scale multiple lightmaps into the 8.8 format in blocklights
===============
*/
// mankrip - begin
#define DLMIN (2 << 8)
#define DLMAX ( (255 - 3) << 8)
#define LIGHTSHIFT(L) ( (L) >> (8 - VID_CBITS))
/*
(255 -1 * 4) to get 1 shade darker than the levelscale variable defined in GrabColormap,
	resulting in an overall shading level 0.5 units darker than the vanilla version (within the same 64 units range),
	because dithered lighting intensity is offset from -0.5 to 0.5 units (so, 2 - 1 - 0.5 = 0.5 units).
*/
#define LIGHTBLOCK_BOUNDINVERTSHIFT_DITHER(i) t = (0xFF00 - (int)blocks[i]); blocks[i] = ( (t<DLMIN) ? LIGHTSHIFT(DLMIN) : ( (t>DLMAX) ? LIGHTSHIFT(DLMAX) : LIGHTSHIFT(t) ) );
// mankrip - end
void R_BuildLightMap (void)
{
	int			smax, tmax;
	int			t;
	int			i, size;
	byte		*lightmap;
	unsigned	scale;
	int			maps;
	int			count, remainder; // mankrip
	unsigned	*blocks; // mankrip
	msurface_t	*surf;

	surf = r_drawsurf.surf;

	smax = (surf->extents[0]>>4)+1;
	tmax = (surf->extents[1]>>4)+1;
	size = smax*tmax; // in most cases, 64*64

	if (r_fullbright.value || !cl.worldmodel->lightdata)
	{
		memset (blocklights, (255 - 31) << 8, sizeof (unsigned) * size); // mankrip - neutral brightness with 2x overbright
		return;
	}

	// clear to ambient
	memset (blocklights, (unsigned)r_refdef.ambientlight << 8, sizeof (unsigned) * size); // mankrip

	lightmap = surf->samples;

	// add all the lightmaps
	if (lightmap)
		for (maps = 0 ; maps < MAXLIGHTMAPS && surf->styles[maps] != 255 ; maps++)
		{
		//	scale = r_drawsurf.lightadj[maps];	// 8.8 fraction
			// mankrip - begin
		//	if (!r_externbsp_lit.value || currententity != cl_entities && currententity->model->name[0] != '*')
				scale = (unsigned) (0.5f + (float)r_drawsurf.lightadj[maps] * currententity->lightlevel);
			for (i=0 ; i<size ; i++)
				blocklights[i] += lightmap[i] * scale;
			// mankrip - end
			lightmap += size;	// skip to next lightmap
		}

	// add all the dynamic lights
	if (surf->dlightframe == r_framecount)
		R_AddDynamicLights ();

	// bound, invert, and shift
	// mankrip - begin
	blocks = blocklights;
	count = size >> 5;
	remainder = size & 31;
	for (i = 0 ; i < count ; i++, blocks += 32)
	{
		LIGHTBLOCK_BOUNDINVERTSHIFT_DITHER (31);
		LIGHTBLOCK_BOUNDINVERTSHIFT_DITHER (30);
		LIGHTBLOCK_BOUNDINVERTSHIFT_DITHER (29);
		LIGHTBLOCK_BOUNDINVERTSHIFT_DITHER (28);
		LIGHTBLOCK_BOUNDINVERTSHIFT_DITHER (27);
		LIGHTBLOCK_BOUNDINVERTSHIFT_DITHER (26);
		LIGHTBLOCK_BOUNDINVERTSHIFT_DITHER (25);
		LIGHTBLOCK_BOUNDINVERTSHIFT_DITHER (24);
		LIGHTBLOCK_BOUNDINVERTSHIFT_DITHER (23);
		LIGHTBLOCK_BOUNDINVERTSHIFT_DITHER (22);
		LIGHTBLOCK_BOUNDINVERTSHIFT_DITHER (21);
		LIGHTBLOCK_BOUNDINVERTSHIFT_DITHER (20);
		LIGHTBLOCK_BOUNDINVERTSHIFT_DITHER (19);
		LIGHTBLOCK_BOUNDINVERTSHIFT_DITHER (18);
		LIGHTBLOCK_BOUNDINVERTSHIFT_DITHER (17);
		LIGHTBLOCK_BOUNDINVERTSHIFT_DITHER (16);
		LIGHTBLOCK_BOUNDINVERTSHIFT_DITHER (15);
		LIGHTBLOCK_BOUNDINVERTSHIFT_DITHER (14);
		LIGHTBLOCK_BOUNDINVERTSHIFT_DITHER (13);
		LIGHTBLOCK_BOUNDINVERTSHIFT_DITHER (12);
		LIGHTBLOCK_BOUNDINVERTSHIFT_DITHER (11);
		LIGHTBLOCK_BOUNDINVERTSHIFT_DITHER (10);
		LIGHTBLOCK_BOUNDINVERTSHIFT_DITHER ( 9);
		LIGHTBLOCK_BOUNDINVERTSHIFT_DITHER ( 8);
		LIGHTBLOCK_BOUNDINVERTSHIFT_DITHER ( 7);
		LIGHTBLOCK_BOUNDINVERTSHIFT_DITHER ( 6);
		LIGHTBLOCK_BOUNDINVERTSHIFT_DITHER ( 5);
		LIGHTBLOCK_BOUNDINVERTSHIFT_DITHER ( 4);
		LIGHTBLOCK_BOUNDINVERTSHIFT_DITHER ( 3);
		LIGHTBLOCK_BOUNDINVERTSHIFT_DITHER ( 2);
		LIGHTBLOCK_BOUNDINVERTSHIFT_DITHER ( 1);
		LIGHTBLOCK_BOUNDINVERTSHIFT_DITHER ( 0);
	}
	switch (remainder)
	{
		case 31: LIGHTBLOCK_BOUNDINVERTSHIFT_DITHER(30);
		case 30: LIGHTBLOCK_BOUNDINVERTSHIFT_DITHER(29);
		case 29: LIGHTBLOCK_BOUNDINVERTSHIFT_DITHER(28);
		case 28: LIGHTBLOCK_BOUNDINVERTSHIFT_DITHER(27);
		case 27: LIGHTBLOCK_BOUNDINVERTSHIFT_DITHER(26);
		case 26: LIGHTBLOCK_BOUNDINVERTSHIFT_DITHER(25);
		case 25: LIGHTBLOCK_BOUNDINVERTSHIFT_DITHER(24);
		case 24: LIGHTBLOCK_BOUNDINVERTSHIFT_DITHER(23);
		case 23: LIGHTBLOCK_BOUNDINVERTSHIFT_DITHER(22);
		case 22: LIGHTBLOCK_BOUNDINVERTSHIFT_DITHER(21);
		case 21: LIGHTBLOCK_BOUNDINVERTSHIFT_DITHER(20);
		case 20: LIGHTBLOCK_BOUNDINVERTSHIFT_DITHER(19);
		case 19: LIGHTBLOCK_BOUNDINVERTSHIFT_DITHER(18);
		case 18: LIGHTBLOCK_BOUNDINVERTSHIFT_DITHER(17);
		case 17: LIGHTBLOCK_BOUNDINVERTSHIFT_DITHER(16);
		case 16: LIGHTBLOCK_BOUNDINVERTSHIFT_DITHER(15);
		case 15: LIGHTBLOCK_BOUNDINVERTSHIFT_DITHER(14);
		case 14: LIGHTBLOCK_BOUNDINVERTSHIFT_DITHER(13);
		case 13: LIGHTBLOCK_BOUNDINVERTSHIFT_DITHER(12);
		case 12: LIGHTBLOCK_BOUNDINVERTSHIFT_DITHER(11);
		case 11: LIGHTBLOCK_BOUNDINVERTSHIFT_DITHER(10);
		case 10: LIGHTBLOCK_BOUNDINVERTSHIFT_DITHER( 9);
		case  9: LIGHTBLOCK_BOUNDINVERTSHIFT_DITHER( 8);
		case  8: LIGHTBLOCK_BOUNDINVERTSHIFT_DITHER( 7);
		case  7: LIGHTBLOCK_BOUNDINVERTSHIFT_DITHER( 6);
		case  6: LIGHTBLOCK_BOUNDINVERTSHIFT_DITHER( 5);
		case  5: LIGHTBLOCK_BOUNDINVERTSHIFT_DITHER( 4);
		case  4: LIGHTBLOCK_BOUNDINVERTSHIFT_DITHER( 3);
		case  3: LIGHTBLOCK_BOUNDINVERTSHIFT_DITHER( 2);
		case  2: LIGHTBLOCK_BOUNDINVERTSHIFT_DITHER( 1);
		case  1: LIGHTBLOCK_BOUNDINVERTSHIFT_DITHER( 0);
		default: break;
	}
	// mankrip - end
}


/*
===============
R_TextureAnimation

Returns the proper texture for a given time and base texture
===============
*/
texture_t *R_TextureAnimation (texture_t *base)
{
	if (currententity->frame)
	{
		if (base->alternate_anims)
			base = base->alternate_anims;
	}

	// mankrip - edited - begin
	if (base->anim_total)
	{
		int
			reletive = (int)(cl.time*10) % base->anim_total
		,	count = 0
			;
	// mankrip - edited - end

		while (base->anim_min > reletive || base->anim_max <= reletive)
		{
			base = base->anim_next;
			if (!base)
				Sys_Error ("R_TextureAnimation: broken cycle");
			if (++count > 100)
				Sys_Error ("R_TextureAnimation: infinite cycle");
		}
	} // mankrip

	return base;
}


// mankrip - begin
static byte
	* psource
,	* prowdest
	;
static int
	* ykernel
,	* xykernel[4]
,	lightstep
,	light
,	l
,	i
,	v
	// integers for dithering
,	Y
,	XYa
,	XYb
,	XYc
,	XYd
	;
// set to >> 7 for more smoothness and less accuracy
// set to >> 9 for more accuracy and less smoothness
#define DITHERSHIFT(L) ( (L) >> 8)
static int lightmap_dither_kernel16[4][4] =
{
	#define OFS(A) DITHERSHIFT ( ( (-15 << 15) + ( (     (A)) << 16)) / 16) // (-15 << 15) == (-7.5 << 16)
	{ OFS( 1), OFS(12), OFS( 2), OFS(14) } // [0,0]
,	{ OFS( 9), OFS( 7), OFS(10), OFS( 4) } // [0,1]
,	{ OFS( 3), OFS(15), OFS( 0), OFS(13) } // [1,0]
,	{ OFS(11), OFS( 5), OFS( 8), OFS( 6) } // [1,1]
	#undef OFS
};

static int null_kernel16[4][4] =
{
	{ 0, 0, 0, 0 } // [0,0]
,	{ 0, 0, 0, 0 } // [0,1]
,	{ 0, 0, 0, 0 } // [1,0]
,	{ 0, 0, 0, 0 } // [1,1]
};
// mankrip - end

/*
===============
R_DrawSurface
===============
*/
void R_DrawSurface (void)
{
	unsigned char	*basetptr;
	int				smax, tmax, twidth;
	int				u;
	int				soffset, basetoffset, texwidth;
	int				horzblockstep;
	unsigned char	*pcolumndest;
	void			(*pblockdrawer)(void);
	texture_t		*mt;

// calculate the lightings
	R_BuildLightMap ();

	surfrowbytes = r_drawsurf.rowbytes;

	mt = r_drawsurf.texture;

	r_source = (byte *)mt + mt->offsets[r_drawsurf.surfmip];

// the fractional light values should range from 0 to (VID_GRADES - 1) << 16
// from a source range of 0 - 255

	texwidth = mt->width >> r_drawsurf.surfmip;

	blocksize = 16 >> r_drawsurf.surfmip;
	blockdivshift = 4 - r_drawsurf.surfmip;
	blockdivmask = (1 << blockdivshift) - 1;

	r_lightwidth = (r_drawsurf.surf->extents[0]>>4)+1;

	r_numhblocks = r_drawsurf.surfwidth >> blockdivshift;
	r_numvblocks = r_drawsurf.surfheight >> blockdivshift;

//==============================

	// TODO: only needs to be set when there is a display settings change
	horzblockstep = blocksize;

	smax = mt->width >> r_drawsurf.surfmip;
	twidth = texwidth;
	tmax = mt->height >> r_drawsurf.surfmip;
	sourcetstep = texwidth;
	r_stepback = tmax * twidth;

	r_sourcemax = r_source + (tmax * smax);

	soffset = r_drawsurf.surf->texturemins[0];
	basetoffset = r_drawsurf.surf->texturemins[1];

// << 16 components are to guarantee positive values for %
	soffset = ((soffset >> r_drawsurf.surfmip) + (smax << 16)) % smax;
	basetptr = &r_source[((((basetoffset >> r_drawsurf.surfmip)
		+ (tmax << 16)) % tmax) * twidth)];

	pcolumndest = r_drawsurf.surfdat;

	// mankrip - begin
	if (d_ditherlighting.value && !r_fullbright.value && cl.worldmodel->lightdata)
	{
		xykernel[0] = lightmap_dither_kernel16[0];
		xykernel[1] = lightmap_dither_kernel16[1];
		xykernel[2] = lightmap_dither_kernel16[2];
		xykernel[3] = lightmap_dither_kernel16[3];
		XYa = DITHERSHIFT (  8192);
		XYb = DITHERSHIFT ( -8192);
		XYc = DITHERSHIFT (-24576);
		XYd = DITHERSHIFT ( 24576);
	}
	else
	{
		xykernel[0] = null_kernel16[0];
		xykernel[1] = null_kernel16[1];
		xykernel[2] = null_kernel16[2];
		xykernel[3] = null_kernel16[3];
		XYa = 0;
		XYb = 0;
		XYc = 0;
		XYd = 0;
	}

	if (!r_fullbright.value && cl.worldmodel->lightdata)
		pblockdrawer = surfmiptable[r_drawsurf.surfmip];
	else
		pblockdrawer = surfmiptable_fullbright[r_drawsurf.surfmip];
	// mankrip - end

	for (u=0 ; u<r_numhblocks; u++)
	{
		r_lightptr = blocklights + u;

		prowdestbase = pcolumndest;

		pbasesource = basetptr + soffset;

		(*pblockdrawer)();

		soffset = soffset + blocksize;
		if (soffset >= smax)
			soffset = 0;

		pcolumndest += horzblockstep;
	}
}


//=============================================================================


void R_DrawSurfaceBlock8_mip0 (void)
{
	psource = pbasesource;
	prowdest = prowdestbase;

	for (v=0 ; v<r_numvblocks ; v++)
	{
	// FIXME: make these locals?
	// FIXME: use delta rather than both right and left, like ASM?
		lightleft = r_lightptr[0];
		lightright = r_lightptr[1];
		r_lightptr += r_lightwidth;
		lightleftstep = (r_lightptr[0] - lightleft) >> 4;
		lightrightstep = (r_lightptr[1] - lightright) >> 4;

		for (i=0 ; i<16 ; i++)
		{
			// mankrip - begin
			light = lightright;
			lightstep = (lightleft - lightright) >> 4;

			ykernel = xykernel[i & 3];
			XYa = ykernel[0];
			XYb = ykernel[1];
			XYc = ykernel[2];
			XYd = ykernel[3];

			prowdest[15] = vid.colormap[ ( (light + XYd) & 0x3F00) | psource[15]]; light += lightstep;
			prowdest[14] = vid.colormap[ ( (light + XYc) & 0x3F00) | psource[14]]; light += lightstep;
			prowdest[13] = vid.colormap[ ( (light + XYb) & 0x3F00) | psource[13]]; light += lightstep;
			prowdest[12] = vid.colormap[ ( (light + XYa) & 0x3F00) | psource[12]]; light += lightstep;
			prowdest[11] = vid.colormap[ ( (light + XYd) & 0x3F00) | psource[11]]; light += lightstep;
			prowdest[10] = vid.colormap[ ( (light + XYc) & 0x3F00) | psource[10]]; light += lightstep;
			prowdest[ 9] = vid.colormap[ ( (light + XYb) & 0x3F00) | psource[ 9]]; light += lightstep;
			prowdest[ 8] = vid.colormap[ ( (light + XYa) & 0x3F00) | psource[ 8]]; light += lightstep;
			prowdest[ 7] = vid.colormap[ ( (light + XYd) & 0x3F00) | psource[ 7]]; light += lightstep;
			prowdest[ 6] = vid.colormap[ ( (light + XYc) & 0x3F00) | psource[ 6]]; light += lightstep;
			prowdest[ 5] = vid.colormap[ ( (light + XYb) & 0x3F00) | psource[ 5]]; light += lightstep;
			prowdest[ 4] = vid.colormap[ ( (light + XYa) & 0x3F00) | psource[ 4]]; light += lightstep;
			prowdest[ 3] = vid.colormap[ ( (light + XYd) & 0x3F00) | psource[ 3]]; light += lightstep;
			prowdest[ 2] = vid.colormap[ ( (light + XYc) & 0x3F00) | psource[ 2]]; light += lightstep;
			prowdest[ 1] = vid.colormap[ ( (light + XYb) & 0x3F00) | psource[ 1]]; light += lightstep;
			prowdest[ 0] = vid.colormap[ ( (light + XYa) & 0x3F00) | psource[ 0]];
			// mankrip - end

			psource += sourcetstep;
			lightright += lightrightstep;
			lightleft += lightleftstep;
			prowdest += surfrowbytes;
		}

		if (psource >= r_sourcemax)
			psource -= r_stepback;
	}
}


void R_DrawSurfaceBlock8_mip1 (void)
{
	psource = pbasesource;
	prowdest = prowdestbase;

	for (v=0 ; v<r_numvblocks ; v++)
	{
	// FIXME: make these locals?
	// FIXME: use delta rather than both right and left, like ASM?
		lightleft = r_lightptr[0];
		lightright = r_lightptr[1];
		r_lightptr += r_lightwidth;
		lightleftstep = (r_lightptr[0] - lightleft) >> 3;
		lightrightstep = (r_lightptr[1] - lightright) >> 3;

		for (i=0 ; i<8 ; i++)
		{
			// mankrip - begin
			light = lightright;
			lightstep = (lightleft - lightright) >> 3;

			ykernel = xykernel[i & 3];
			XYa = ykernel[0];
			XYb = ykernel[1];
			XYc = ykernel[2];
			XYd = ykernel[3];

			prowdest[ 7] = vid.colormap[ ( (light + XYd) & 0x3F00) | psource[ 7]]; light += lightstep;
			prowdest[ 6] = vid.colormap[ ( (light + XYc) & 0x3F00) | psource[ 6]]; light += lightstep;
			prowdest[ 5] = vid.colormap[ ( (light + XYb) & 0x3F00) | psource[ 5]]; light += lightstep;
			prowdest[ 4] = vid.colormap[ ( (light + XYa) & 0x3F00) | psource[ 4]]; light += lightstep;
			prowdest[ 3] = vid.colormap[ ( (light + XYd) & 0x3F00) | psource[ 3]]; light += lightstep;
			prowdest[ 2] = vid.colormap[ ( (light + XYc) & 0x3F00) | psource[ 2]]; light += lightstep;
			prowdest[ 1] = vid.colormap[ ( (light + XYb) & 0x3F00) | psource[ 1]]; light += lightstep;
			prowdest[ 0] = vid.colormap[ ( (light + XYa) & 0x3F00) | psource[ 0]];
			// mankrip - end

			psource += sourcetstep;
			lightright += lightrightstep;
			lightleft += lightleftstep;
			prowdest += surfrowbytes;
		}

		if (psource >= r_sourcemax)
			psource -= r_stepback;
	}
}


void R_DrawSurfaceBlock8_mip2 (void)
{
	psource = pbasesource;
	prowdest = prowdestbase;

	for (v=0 ; v<r_numvblocks ; v++)
	{
	// FIXME: make these locals?
	// FIXME: use delta rather than both right and left, like ASM?
		lightleft = r_lightptr[0];
		lightright = r_lightptr[1];
		r_lightptr += r_lightwidth;
		lightleftstep = (r_lightptr[0] - lightleft) >> 2;
		lightrightstep = (r_lightptr[1] - lightright) >> 2;

		for (i=0 ; i<4 ; i++)
		{
			// mankrip - begin
			light = lightright;
			lightstep = (lightleft - lightright) >> 2;

			ykernel = xykernel[i & 3];

			prowdest[ 3] = vid.colormap[ ( (light + ykernel[3]) & 0x3F00) | psource[ 3]]; light += lightstep;
			prowdest[ 2] = vid.colormap[ ( (light + ykernel[2]) & 0x3F00) | psource[ 2]]; light += lightstep;
			prowdest[ 1] = vid.colormap[ ( (light + ykernel[1]) & 0x3F00) | psource[ 1]]; light += lightstep;
			prowdest[ 0] = vid.colormap[ ( (light + ykernel[0]) & 0x3F00) | psource[ 0]];
			// mankrip - end

			psource += sourcetstep;
			lightright += lightrightstep;
			lightleft += lightleftstep;
			prowdest += surfrowbytes;
		}

		if (psource >= r_sourcemax)
			psource -= r_stepback;
	}
}


void R_DrawSurfaceBlock8_mip3 (void)
{
	psource = pbasesource;
	prowdest = prowdestbase;

	for (v=0 ; v<r_numvblocks ; v++)
	{
		// FIXME: make these locals?
		// FIXME: use delta rather than both right and left, like ASM?
		lightleft = r_lightptr[0];
		lightright = r_lightptr[1];
		r_lightptr += r_lightwidth;

		// mankrip - begin
		prowdest[ 1] = vid.colormap[ ( (  lightright                                     + XYa) & 0x3F00) | psource[ 1]];
		prowdest[ 0] = vid.colormap[ ( ( (lightright + ( (lightleft - lightright) >> 1)) + XYb) & 0x3F00) | psource[ 0]];

		prowdest += surfrowbytes;
		psource += sourcetstep;
		lightleft += ( (r_lightptr[0] - lightleft) >> 1);
		lightright += ( (r_lightptr[1] - lightright) >> 1);

		prowdest[ 1] = vid.colormap[ ( (  lightright                                     + XYc) & 0x3F00) | psource[ 1]];
		prowdest[ 0] = vid.colormap[ ( ( (lightright + ( (lightleft - lightright) >> 1)) + XYd) & 0x3F00) | psource[ 0]];
		// mankrip - end
		prowdest += surfrowbytes;
		psource += sourcetstep;

		if (psource >= r_sourcemax)
			psource -= r_stepback;
	}
}


void R_DrawSurfaceBlock8_mip0_fullbright (void)
{
	psource = pbasesource;
	prowdest = prowdestbase;

	for (v=0 ; v<r_numvblocks ; v++)
	{
		for (i=0 ; i<16 ; i++)
		{
			// mankrip - begin
			prowdest[15] = psource[15];
			prowdest[14] = psource[14];
			prowdest[13] = psource[13];
			prowdest[12] = psource[12];
			prowdest[11] = psource[11];
			prowdest[10] = psource[10];
			prowdest[ 9] = psource[ 9];
			prowdest[ 8] = psource[ 8];
			prowdest[ 7] = psource[ 7];
			prowdest[ 6] = psource[ 6];
			prowdest[ 5] = psource[ 5];
			prowdest[ 4] = psource[ 4];
			prowdest[ 3] = psource[ 3];
			prowdest[ 2] = psource[ 2];
			prowdest[ 1] = psource[ 1];
			prowdest[ 0] = psource[ 0];
			// mankrip - end

			psource += sourcetstep;
			prowdest += surfrowbytes;
		}

		if (psource >= r_sourcemax)
			psource -= r_stepback;
	}
}


void R_DrawSurfaceBlock8_mip1_fullbright (void)
{
	psource = pbasesource;
	prowdest = prowdestbase;

	for (v=0 ; v<r_numvblocks ; v++)
	{
		for (i=0 ; i<8 ; i++)
		{
			// mankrip - begin
			prowdest[ 7] = psource[ 7];
			prowdest[ 6] = psource[ 6];
			prowdest[ 5] = psource[ 5];
			prowdest[ 4] = psource[ 4];
			prowdest[ 3] = psource[ 3];
			prowdest[ 2] = psource[ 2];
			prowdest[ 1] = psource[ 1];
			prowdest[ 0] = psource[ 0];
			// mankrip - end

			psource += sourcetstep;
			prowdest += surfrowbytes;
		}

		if (psource >= r_sourcemax)
			psource -= r_stepback;
	}
}


void R_DrawSurfaceBlock8_mip2_fullbright (void)
{
	psource = pbasesource;
	prowdest = prowdestbase;

	for (v=0 ; v<r_numvblocks ; v++)
	{
		for (i=0 ; i<4 ; i++)
		{
			// mankrip - begin
			prowdest[ 3] = psource[ 3];
			prowdest[ 2] = psource[ 2];
			prowdest[ 1] = psource[ 1];
			prowdest[ 0] = psource[ 0];
			// mankrip - end

			psource += sourcetstep;
			prowdest += surfrowbytes;
		}

		if (psource >= r_sourcemax)
			psource -= r_stepback;
	}
}


void R_DrawSurfaceBlock8_mip3_fullbright (void)
{
	psource = pbasesource;
	prowdest = prowdestbase;

	for (v=0 ; v<r_numvblocks ; v++)
	{
		// mankrip - begin
		prowdest[ 1] = psource[ 1];
		prowdest[ 0] = psource[ 0];

		prowdest += surfrowbytes;
		psource += sourcetstep;

		prowdest[ 1] = psource[ 1];
		prowdest[ 0] = psource[ 0];
		// mankrip - end
		prowdest += surfrowbytes;
		psource += sourcetstep;

		if (psource >= r_sourcemax)
			psource -= r_stepback;
	}
}


#if	!id386
void R_DrawSurfaceBlock16 (void) { }
#endif
