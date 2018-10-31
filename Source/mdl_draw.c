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
// d_polyset.c: routines for drawing sets of polygons sharing the same
// texture (used for Alias models)

#include "quakedef.h"
#include "r_local.h"
#include "d_local.h"

// TODO: put in span spilling to shrink list size
// !!! if this is changed, it must be changed in d_polysa.s too !!!
#define DPS_MAXSPANS			MAXHEIGHT+1
									// 1 extra for spanpackage that marks end

typedef struct {
	int		isflattop;
	int		numleftedges;
	int		*pleftedgevert0;
	int		*pleftedgevert1;
	int		*pleftedgevert2;
	int		numrightedges;
	int		*prightedgevert0;
	int		*prightedgevert1;
	int		*prightedgevert2;
} edgetable;

int	r_p0[6], r_p1[6], r_p2[6];

byte		*d_pcolormap;

int			d_aflatcolor;
int			d_xdenom;

edgetable	*pedgetable;

edgetable	edgetables[12] = {
	{0, 1, r_p0, r_p2, NULL, 2, r_p0, r_p1, r_p2 },
	{0, 2, r_p1, r_p0, r_p2,   1, r_p1, r_p2, NULL},
	{1, 1, r_p0, r_p2, NULL, 1, r_p1, r_p2, NULL},
	{0, 1, r_p1, r_p0, NULL, 2, r_p1, r_p2, r_p0 },
	{0, 2, r_p0, r_p2, r_p1,   1, r_p0, r_p1, NULL},
	{0, 1, r_p2, r_p1, NULL, 1, r_p2, r_p0, NULL},
	{0, 1, r_p2, r_p1, NULL, 2, r_p2, r_p0, r_p1 },
	{0, 2, r_p2, r_p1, r_p0,   1, r_p2, r_p0, NULL},
	{0, 1, r_p1, r_p0, NULL, 1, r_p1, r_p2, NULL},
	{1, 1, r_p2, r_p1, NULL, 1, r_p0, r_p1, NULL},
	{1, 1, r_p1, r_p0, NULL, 1, r_p2, r_p0, NULL},
	{0, 1, r_p0, r_p2, NULL, 1, r_p0, r_p1, NULL},
};

// FIXME: some of these can become statics
int				a_sstepxfrac, a_tstepxfrac, r_lstepx, a_ststepxwhole;
int				r_sstepx, r_tstepx, r_lstepy, r_sstepy, r_tstepy;
int				r_zistepx, r_zistepy;
int				d_aspancount, d_countextrastep;

spanpackage_t
	* a_spans
,	* pstart
	;
spanpackage_t			*d_pedgespanpackage;
static int				ystart;
byte					*d_pdest, *d_ptex;
short					*d_pz;
int						d_sfrac, d_tfrac, d_light, d_zi;
int						d_ptexextrastep, d_sfracextrastep;
int						d_tfracextrastep, d_lightextrastep, d_pdestextrastep;
int						d_lightbasestep, d_pdestbasestep, d_ptexbasestep;
int						d_sfracbasestep, d_tfracbasestep;
int						d_ziextrastep, d_zibasestep;
int						d_pzextrastep, d_pzbasestep;

typedef struct {
	int		quotient;
	int		remainder;
} adivtab_t;

static adivtab_t	adivtab[32*32] = {
#include "adivtab.h"
};

byte	*skintable[MAX_LBM_HEIGHT];
int		skinwidth;
byte	*skinstart;

void D_PolysetCalcGradients (int skinwidth);
void D_DrawSubdiv (void);
void D_DrawNonSubdiv (void);
void D_PolysetRecursiveTriangle (int *p1, int *p2, int *p3);
void D_PolysetSetEdgeTable (void);
void D_RasterizeAliasPolySmooth (void);
void D_PolysetScanLeftEdge (int height);
void D_DrawNonSubdiv_C (void); // mankrip - transparencies

//#if	!id386 // mankrip - transparencies - removed

/*
================
D_PolysetDraw
================
*/
void D_PolysetDraw_C (void) // mankrip - transparencies - edited
{
	spanpackage_t	spans[DPS_MAXSPANS + 1 +
			((CACHE_SIZE - 1) / sizeof(spanpackage_t)) + 1];
						// one extra because of cache line pretouching

	a_spans = (spanpackage_t *)
			(((long)&spans[0] + CACHE_SIZE - 1) & ~(CACHE_SIZE - 1));

	if (r_affinetridesc.drawtype)
	{
		D_DrawSubdiv ();
	}
	else
	{
		D_DrawNonSubdiv_C (); // mankrip - transparencies - edited
	}
}

#if	!id386 // mankrip - transparencies

/*
================
D_PolysetDrawFinalVerts
================
*/
void D_PolysetDrawFinalVerts (finalvert_t *fv, int numverts)
{
	int		i, z;
	short	*zbuf;
	int		pix;

	for (i=0 ; i<numverts ; i++, fv++)
	{
	// valid triangle coordinates for filling can include the bottom and
	// right clip edges, due to the fill rule; these shouldn't be drawn
		if ((fv->v[0] < r_refdef.vrectright) &&
			(fv->v[1] < r_refdef.vrectbottom))
		{
			z = fv->v[5]>>16;
			zbuf = zspantable[fv->v[1]] + fv->v[0];
			if (z >= *zbuf)
			{
				*zbuf = z;
				pix = skintable[fv->v[3]>>16][fv->v[2]>>16];
				pix = ((byte *)acolormap)[pix + (fv->v[4] & 0xFF00) ];
				d_viewbuffer[d_scantable[fv->v[1]] + fv->v[0]] = pix;
			}
		}
	}
}
#endif	// !id386 // mankrip - transparencies


/*
================
D_DrawSubdiv
================
*/
void D_DrawSubdiv (void)
{
	mtriangle_t		*ptri;
	finalvert_t		*pfv, *index0, *index1, *index2;
	int				i;
	int				lnumtriangles;

	pfv = r_affinetridesc.pfinalverts;
	ptri = r_affinetridesc.ptriangles;
	lnumtriangles = r_affinetridesc.numtriangles;

	for (i=0 ; i<lnumtriangles ; i++)
	{
		index0 = pfv + ptri[i].vertindex[0];
		index1 = pfv + ptri[i].vertindex[1];
		index2 = pfv + ptri[i].vertindex[2];

		if (((index0->v[1]-index1->v[1]) *
			 (index0->v[0]-index2->v[0]) -
			 (index0->v[0]-index1->v[0]) *
			 (index0->v[1]-index2->v[1])) >= 0)
		{
			continue;
		}

		d_pcolormap = &((byte *)acolormap)[index0->v[4] & 0xFF00];

		if (ptri[i].facesfront)
		{
			D_PolysetRecursiveTriangle(index0->v, index1->v, index2->v);
		}
		else
		{
			int		s0, s1, s2;

			s0 = index0->v[2];
			s1 = index1->v[2];
			s2 = index2->v[2];

			if (index0->flags & ALIAS_ONSEAM)
				index0->v[2] += r_affinetridesc.seamfixupX16;
			if (index1->flags & ALIAS_ONSEAM)
				index1->v[2] += r_affinetridesc.seamfixupX16;
			if (index2->flags & ALIAS_ONSEAM)
				index2->v[2] += r_affinetridesc.seamfixupX16;

			D_PolysetRecursiveTriangle(index0->v, index1->v, index2->v);

			index0->v[2] = s0;
			index1->v[2] = s1;
			index2->v[2] = s2;
		}
	}
}


/*
================
D_DrawNonSubdiv
================
*/
void D_DrawNonSubdiv_C (void) // mankrip - transparencies - edited
{
	mtriangle_t		*ptri;
	finalvert_t		*pfv, *index0, *index1, *index2;
	int				i;
	int				lnumtriangles;

	pfv = r_affinetridesc.pfinalverts;
	ptri = r_affinetridesc.ptriangles;
	lnumtriangles = r_affinetridesc.numtriangles;

	for (i=0 ; i<lnumtriangles ; i++, ptri++)
	{
		index0 = pfv + ptri->vertindex[0];
		index1 = pfv + ptri->vertindex[1];
		index2 = pfv + ptri->vertindex[2];

		d_xdenom = (index0->v[1]-index1->v[1]) *
				(index0->v[0]-index2->v[0]) -
				(index0->v[0]-index1->v[0])*(index0->v[1]-index2->v[1]);

		if (d_xdenom >= 0)
		{
			continue;
		}

		r_p0[0] = index0->v[0];		// u
		r_p0[1] = index0->v[1];		// v
		r_p0[2] = index0->v[2];		// s
		r_p0[3] = index0->v[3];		// t
		r_p0[4] = index0->v[4];		// light
		r_p0[5] = index0->v[5];		// iz

		r_p1[0] = index1->v[0];
		r_p1[1] = index1->v[1];
		r_p1[2] = index1->v[2];
		r_p1[3] = index1->v[3];
		r_p1[4] = index1->v[4];
		r_p1[5] = index1->v[5];

		r_p2[0] = index2->v[0];
		r_p2[1] = index2->v[1];
		r_p2[2] = index2->v[2];
		r_p2[3] = index2->v[3];
		r_p2[4] = index2->v[4];
		r_p2[5] = index2->v[5];

		if (!ptri->facesfront)
		{
			if (index0->flags & ALIAS_ONSEAM)
				r_p0[2] += r_affinetridesc.seamfixupX16;
			if (index1->flags & ALIAS_ONSEAM)
				r_p1[2] += r_affinetridesc.seamfixupX16;
			if (index2->flags & ALIAS_ONSEAM)
				r_p2[2] += r_affinetridesc.seamfixupX16;
		}

		D_PolysetSetEdgeTable ();
		D_RasterizeAliasPolySmooth ();
	}
}

#if	!id386 // mankrip - transparencies

/*
================
D_PolysetRecursiveTriangle
================
*/
void D_PolysetRecursiveTriangle (int *lp1, int *lp2, int *lp3)
{
	int		*temp;
	int		d;
	int		new[6];
	int		z;
	short	*zbuf;

	d = lp2[0] - lp1[0];
	if (d < -1 || d > 1)
		goto split;
	d = lp2[1] - lp1[1];
	if (d < -1 || d > 1)
		goto split;

	d = lp3[0] - lp2[0];
	if (d < -1 || d > 1)
		goto split2;
	d = lp3[1] - lp2[1];
	if (d < -1 || d > 1)
		goto split2;

	d = lp1[0] - lp3[0];
	if (d < -1 || d > 1)
		goto split3;
	d = lp1[1] - lp3[1];
	if (d < -1 || d > 1)
	{
split3:
		temp = lp1;
		lp1 = lp3;
		lp3 = lp2;
		lp2 = temp;

		goto split;
	}

	return;			// entire tri is filled

split2:
	temp = lp1;
	lp1 = lp2;
	lp2 = lp3;
	lp3 = temp;

split:
// split this edge
	new[0] = (lp1[0] + lp2[0]) >> 1;
	new[1] = (lp1[1] + lp2[1]) >> 1;
	new[2] = (lp1[2] + lp2[2]) >> 1;
	new[3] = (lp1[3] + lp2[3]) >> 1;
	new[5] = (lp1[5] + lp2[5]) >> 1;

// draw the point if splitting a leading edge
	if (lp2[1] > lp1[1])
		goto nodraw;
	if ((lp2[1] == lp1[1]) && (lp2[0] < lp1[0]))
		goto nodraw;


	z = new[5]>>16;
	zbuf = zspantable[new[1]] + new[0];
	if (z >= *zbuf)
	{
		int		pix;

		*zbuf = z;
		pix = d_pcolormap[skintable[new[3]>>16][new[2]>>16]];
		d_viewbuffer[d_scantable[new[1]] + new[0]] = pix;
	}

nodraw:
// recursively continue
	D_PolysetRecursiveTriangle (lp3, lp1, new);
	D_PolysetRecursiveTriangle (lp3, new, lp2);
}

#endif	// !id386


/*
================
D_PolysetUpdateTables
================
*/
void D_PolysetUpdateTables (void)
{
	int		i;
	byte	*s;

	if (r_affinetridesc.skinwidth != skinwidth ||
		r_affinetridesc.pskin != skinstart)
	{
		skinwidth = r_affinetridesc.skinwidth;
		skinstart = r_affinetridesc.pskin;
		s = skinstart;
		for (i=0 ; i<MAX_LBM_HEIGHT ; i++, s+=skinwidth)
			skintable[i] = s;
	}
}


//#if	!id386 // mankrip - transparencies - removed

/*
===================
D_PolysetScanLeftEdge
====================
*/
void D_PolysetScanLeftEdge_C (int height) // mankrip - transparencies - edited
{

	do
	{
		d_pedgespanpackage->pdest = d_pdest;
		d_pedgespanpackage->pz = d_pz;
		d_pedgespanpackage->count = d_aspancount;
		d_pedgespanpackage->ptex = d_ptex;

		d_pedgespanpackage->sfrac = d_sfrac;
		d_pedgespanpackage->tfrac = d_tfrac;

	// FIXME: need to clamp l, s, t, at both ends?
		d_pedgespanpackage->light = d_light;
		d_pedgespanpackage->zi = d_zi;

		d_pedgespanpackage++;

		errorterm += erroradjustup;
		if (errorterm >= 0)
		{
			d_pdest += d_pdestextrastep;
			d_pz += d_pzextrastep;
			d_aspancount += d_countextrastep;
			d_ptex += d_ptexextrastep;
			d_sfrac += d_sfracextrastep;
			d_ptex += d_sfrac >> 16;

			d_sfrac &= 0xFFFF;
			d_tfrac += d_tfracextrastep;
			if (d_tfrac & 0x10000)
			{
				d_ptex += r_affinetridesc.skinwidth;
				d_tfrac &= 0xFFFF;
			}
			d_light += d_lightextrastep;
			d_zi += d_ziextrastep;
			errorterm -= erroradjustdown;
		}
		else
		{
			d_pdest += d_pdestbasestep;
			d_pz += d_pzbasestep;
			d_aspancount += ubasestep;
			d_ptex += d_ptexbasestep;
			d_sfrac += d_sfracbasestep;
			d_ptex += d_sfrac >> 16;
			d_sfrac &= 0xFFFF;
			d_tfrac += d_tfracbasestep;
			if (d_tfrac & 0x10000)
			{
				d_ptex += r_affinetridesc.skinwidth;
				d_tfrac &= 0xFFFF;
			}
			d_light += d_lightbasestep;
			d_zi += d_zibasestep;
		}
	} while (--height);
}

//#endif	// !id386 // mankrip - transparencies - removed


/*
===================
D_PolysetSetUpForLineScan
====================
*/
void D_PolysetSetUpForLineScan(fixed8_t startvertu, fixed8_t startvertv,
		fixed8_t endvertu, fixed8_t endvertv)
{
	double		dm, dn;
	int			tm, tn;
	adivtab_t	*ptemp;

// TODO: implement x86 version

	errorterm = -1;

	tm = endvertu - startvertu;
	tn = endvertv - startvertv;

	if (((tm <= 16) && (tm >= -15)) &&
		((tn <= 16) && (tn >= -15)))
	{
		ptemp = &adivtab[((tm+15) << 5) + (tn+15)];
		ubasestep = ptemp->quotient;
		erroradjustup = ptemp->remainder;
		erroradjustdown = tn;
	}
	else
	{
		dm = (double)tm;
		dn = (double)tn;

		FloorDivMod (dm, dn, &ubasestep, &erroradjustup);

		erroradjustdown = dn;
	}
}


//#if	!id386 // mankrip - transparencies - removed

/*
================
D_PolysetCalcGradients
================
*/
void D_PolysetCalcGradients_C (int skinwidth) // mankrip - transparencies - edited
{
	float	xstepdenominv, ystepdenominv, t0, t1;
	float	p01_minus_p21, p11_minus_p21, p00_minus_p20, p10_minus_p20;

	p00_minus_p20 = r_p0[0] - r_p2[0];
	p01_minus_p21 = r_p0[1] - r_p2[1];
	p10_minus_p20 = r_p1[0] - r_p2[0];
	p11_minus_p21 = r_p1[1] - r_p2[1];

	xstepdenominv = 1.0 / (float)d_xdenom;

	ystepdenominv = -xstepdenominv;

	// mankrip - optimization
	p00_minus_p20 = (r_p0[0] - r_p2[0]) * ystepdenominv;
	p01_minus_p21 = (r_p0[1] - r_p2[1]) * xstepdenominv;
	p10_minus_p20 = (r_p1[0] - r_p2[0]) * ystepdenominv;
	p11_minus_p21 = (r_p1[1] - r_p2[1]) * xstepdenominv;

	t0 = r_p0[2] - r_p2[2];
	t1 = r_p1[2] - r_p2[2];
	r_sstepx  = (int)      (t1 * p01_minus_p21 - t0 * p11_minus_p21);
	r_sstepy  = (int)      (t1 * p00_minus_p20 - t0 * p10_minus_p20);

	t0 = r_p0[3] - r_p2[3];
	t1 = r_p1[3] - r_p2[3];
	r_tstepx  = (int)      (t1 * p01_minus_p21 - t0 * p11_minus_p21);
	r_tstepy  = (int)      (t1 * p00_minus_p20 - t0 * p10_minus_p20);

// ceil () for light so positive steps are exaggerated, negative steps
// diminished,  pushing us away from underflow toward overflow. Underflow is
// very visible, overflow is very unlikely, because of ambient lighting
	t0 = r_p0[4] - r_p2[4];
	t1 = r_p1[4] - r_p2[4];
	r_lstepx  = (int) ceil (t1 * p01_minus_p21 - t0 * p11_minus_p21);
	r_lstepy  = (int) ceil (t1 * p00_minus_p20 - t0 * p10_minus_p20);

	t0 = r_p0[5] - r_p2[5];
	t1 = r_p1[5] - r_p2[5];
	r_zistepx = (int)      (t1 * p01_minus_p21 - t0 * p11_minus_p21);
	r_zistepy = (int)      (t1 * p00_minus_p20 - t0 * p10_minus_p20);

	// these are for affine rendering only
	a_sstepxfrac = r_sstepx & 0xFFFF;
	a_tstepxfrac = r_tstepx & 0xFFFF;
	a_ststepxwhole = skinwidth * (r_tstepx >> 16) + (r_sstepx >> 16);
}

//#endif	// !id386 // mankrip - transparencies - removed



/*
================
D_PolysetFillSpans8
================
*/
void D_PolysetFillSpans8 (void)
{
	spanpackage_t
		* pspanpackage = pstart
		;
	int				color;

// FIXME: do z buffering

	color = d_aflatcolor++;

	while (1)
	{
		int		lcount;
		byte	*lpdest;

		lcount = pspanpackage->count;

		if (lcount == -1)
			return;

		if (lcount)
		{
			lpdest = pspanpackage->pdest;

			do
			{
				*lpdest++ = color;
			} while (--lcount);
		}

		pspanpackage++;
	}
}

/*
================
D_PolysetDrawSpans8
================
*/
#if 1 /// drawing macros
#define SCANBITS		4
#define SCANSIZE		(1 << SCANBITS)
#define SCANMASK		(SCANSIZE - 1)

#define TEXELPRECISION	16 // bits of precision per texel coordinate
#define	TEXELSIZE		(1 << TEXELPRECISION) // 128 0x80

#define TEXEL			lptex[ (t_s >> TEXELPRECISION) + (t_t >> TEXELPRECISION) * r_affinetridesc.skinwidth]
#define DTEXEL(A,B)		lptex[ ( (t_s + A) >> TEXELPRECISION) + ( (t_t + B) >> TEXELPRECISION) * r_affinetridesc.skinwidth]
#define TEXEL_A			DTEXEL (TXYSa, TXYTa)
#define TEXEL_B			DTEXEL (TXYSb, TXYTb)
#define TEXEL_C			DTEXEL (TXYSc, TXYTc)
#define TEXEL_D			DTEXEL (TXYSd, TXYTd)

#define MLIGHT			(llight & 0xAA00)
#define LIGHT			(llight & 0x3F00)
#define DLIGHT(L)		( (llight + (L << 6)) & 0x3F00)
#define LIGHT_A			DLIGHT (LXYVa)
#define LIGHT_B			DLIGHT (LXYVb)
#define LIGHT_C			DLIGHT (LXYVc)
#define LIGHT_D			DLIGHT (LXYVd)

#define DEPTH       	            (izi >> 16)
#define DEPTHTEST(I)	if (pz[I] <= DEPTH)
#define SETDEPTH(I) 	    pz[I]  = DEPTH

#define ALPHATEST(T)	if ( (pcolor = T) != TRANSPARENT_COLOR)
#define TEXELSHADE		mdlcolormap [LIGHT | TEXEL]
#define SETPIXEL(I,T) 	pdest[I]  = T

#define FULLOPAQUE_SHADE(I)		DEPTHTEST (I) { SETPIXEL (I, TEXELSHADE); SETDEPTH (I); }
#define FULLOPAQUE_SHADE_A(I)	DEPTHTEST (I) {                     pdest[I] =                                      mdlcolormap[LIGHT_A | TEXEL ];        SETDEPTH (I);   }
#define FULLOPAQUE_SHADE_B(I)	DEPTHTEST (I) {                     pdest[I] =                                      mdlcolormap[LIGHT_B | TEXEL ];        SETDEPTH (I);   }
#define FULLOPAQUE_SHADE_C(I)	DEPTHTEST (I) {                     pdest[I] =                                      mdlcolormap[LIGHT_C | TEXEL ];        SETDEPTH (I);   }
#define FULLOPAQUE_SHADE_D(I)	DEPTHTEST (I) {                     pdest[I] =                                      mdlcolormap[LIGHT_D | TEXEL ];        SETDEPTH (I);   }
#define COLORKEYED_SHADE_A(I)	DEPTHTEST (I) { ALPHATEST (TEXEL) { pdest[I] =                                      mdlcolormap[LIGHT_A | pcolor];        SETDEPTH (I); } }
#define COLORKEYED_SHADE_B(I)	DEPTHTEST (I) { ALPHATEST (TEXEL) { pdest[I] =                                      mdlcolormap[LIGHT_B | pcolor];        SETDEPTH (I); } }
#define COLORKEYED_SHADE_C(I)	DEPTHTEST (I) { ALPHATEST (TEXEL) { pdest[I] =                                      mdlcolormap[LIGHT_C | pcolor];        SETDEPTH (I); } }
#define COLORKEYED_SHADE_D(I)	DEPTHTEST (I) { ALPHATEST (TEXEL) { pdest[I] =                                      mdlcolormap[LIGHT_D | pcolor];        SETDEPTH (I); } }
#define FULLOPAQUE_METAL(I)		DEPTHTEST (I) {                     pdest[I] =                                      mdlcolormap[MLIGHT  | TEXEL ];        SETDEPTH (I);   }
#define COLORBLEND_SHADE_A(I)	DEPTHTEST (I) {                     pdest[I] = colorblendingmap[  pdest[I]       | (mdlcolormap[LIGHT_A | TEXEL ] << 8)];                 }
#define COLORBLEND_SHADE_B(I)	DEPTHTEST (I) {                     pdest[I] = colorblendingmap[  pdest[I]       | (mdlcolormap[LIGHT_B | TEXEL ] << 8)];                 }
#define COLORBLEND_SHADE_C(I)	DEPTHTEST (I) {                     pdest[I] = colorblendingmap[  pdest[I]       | (mdlcolormap[LIGHT_C | TEXEL ] << 8)];                 }
#define COLORBLEND_SHADE_D(I)	DEPTHTEST (I) {                     pdest[I] = colorblendingmap[  pdest[I]       | (mdlcolormap[LIGHT_D | TEXEL ] << 8)];                 }
#define BLENDCOLOR_SHADE_A(I)	DEPTHTEST (I) {                     pdest[I] = colorblendingmap[ (pdest[I] << 8) |  mdlcolormap[LIGHT_A | TEXEL ]      ];                 }
#define BLENDCOLOR_SHADE_B(I)	DEPTHTEST (I) {                     pdest[I] = colorblendingmap[ (pdest[I] << 8) |  mdlcolormap[LIGHT_B | TEXEL ]      ];                 }
#define BLENDCOLOR_SHADE_C(I)	DEPTHTEST (I) {                     pdest[I] = colorblendingmap[ (pdest[I] << 8) |  mdlcolormap[LIGHT_C | TEXEL ]      ];                 }
#define BLENDCOLOR_SHADE_D(I)	DEPTHTEST (I) {                     pdest[I] = colorblendingmap[ (pdest[I] << 8) |  mdlcolormap[LIGHT_D | TEXEL ]      ];                 }

#define DEPTH_STEP		izi += r_zistepx; // view distance
#define TEXEL_STEP		t_s += r_sstepx; t_t += r_tstepx;
#define LIGHT_STEP		llight += r_lstepx; //if ( (llight + r_lstepx) & 0xFFFF0000) llight -= r_lstepx; // clamp lighting (remove this?)
#define STEP_DRAWING	DEPTH_STEP TEXEL_STEP LIGHT_STEP
#endif

#define OFSMATRIX16(O0, O1, O2, O3, O4, O5, O6, O7, O8, O9, OA, OB, OC, OD, OE, OF) \
	{OFS(O0), OFS(O1), OFS(O2), OFS(O3)} \
,	{OFS(O4), OFS(O5), OFS(O6), OFS(O7)} \
,	{OFS(O8), OFS(O9), OFS(OA), OFS(OB)} \
,	{OFS(OC), OFS(OD), OFS(OE), OFS(OF)}
int scan_l_kernel[4][4] =
{
	#define OFS(A) (-3 + (    (A)))
	OFSMATRIX16 (
		3, 0, 4, 1,
		2, 6, 3, 5,
		4, 1, 4, 0,
		2, 5, 2, 6
		)
	#undef OFS
};
static int null_kernel16[4][4] =
{
	{ 0, 0, 0, 0 } // [0,0]
,	{ 0, 0, 0, 0 } // [0,1]
,	{ 0, 0, 0, 0 } // [1,0]
,	{ 0, 0, 0, 0 } // [1,1]
};
static int
	// integers for dithering
	* uo
,	u
,	vo
,	v
	;
int
	// integers for dithering
	* lvkernelY
,	* xykernel[4]
,	uofs[8] = {0, 1, 2, 3, 0, 1, 2, 3} // the indexes of the horizontal dimension of scan_s_kernel, repeated twice
,	LXYVa
,	LXYVb
,	LXYVc
,	LXYVd
	;
void MDL_DrawSpans_FullOpaque (void)
{
	#undef DRAW_A
	#undef DRAW_B
	#undef DRAW_C
	#undef DRAW_D
	#define DRAW_A(I)	FULLOPAQUE_SHADE_A(I);
	#define DRAW_B(I)	FULLOPAQUE_SHADE_B(I);
	#define DRAW_C(I)	FULLOPAQUE_SHADE_C(I);
	#define DRAW_D(I)	FULLOPAQUE_SHADE_D(I);
	spanpackage_t
		* pspanpackage = pstart
		;
	int
		count
	,	spancount
	,	t_s
	,	t_t
	,	llight
	,	izi
		;
	short
		* pz
		;
	byte
		* pdest
	,	* lptex
		;

	if (d_ditherlighting.value)
	{
		xykernel[0] = scan_l_kernel[0];
		xykernel[1] = scan_l_kernel[1];
		xykernel[2] = scan_l_kernel[2];
		xykernel[3] = scan_l_kernel[3];
	}
	else
	{
		xykernel[0] = null_kernel16[0];
		xykernel[1] = null_kernel16[1];
		xykernel[2] = null_kernel16[2];
		xykernel[3] = null_kernel16[3];
	}

	do // line drawing
	{
		count = d_aspancount - pspanpackage->count; // must be done before updating errorterm

		if ( (errorterm += erroradjustup) >= 0)
		{
			errorterm -= erroradjustdown;
			d_aspancount += d_countextrastep;
		}
		else
			d_aspancount += ubasestep;

		// skip lines with no pixels to draw
		if (count)
		{
			pdest = pspanpackage->pdest;

			lptex = pspanpackage->ptex;
			t_s = pspanpackage->sfrac;
			t_t = pspanpackage->tfrac;
			llight = pspanpackage->light;
			pz = pspanpackage->pz;
			izi = pspanpackage->zi;

			// prepare dither values
			#if 1
			// u v used for setting the dithering only
			u = (int)(pdest - d_viewbuffer);
			v = u / vid.rowbytes; // position of lines
			u -= v * screenwidth; // position of columns

			uo = &uofs[u & 3];
			vo =  uofs[v & 3];
			// for shading
			lvkernelY = xykernel[vo];
			LXYVa = lvkernelY[uo[0]];
			LXYVb = lvkernelY[uo[1]];
			LXYVc = lvkernelY[uo[2]];
			LXYVd = lvkernelY[uo[3]];
			#endif

			spancount = count & SCANMASK;
			count >>= SCANBITS;
			while (count--)
			{
				// scan drawing
				pdest += SCANSIZE;
				pz    += SCANSIZE;
				DRAW_A(-16); STEP_DRAWING;
				DRAW_B(-15); STEP_DRAWING;
				DRAW_C(-14); STEP_DRAWING;
				DRAW_D(-13); STEP_DRAWING;
				DRAW_A(-12); STEP_DRAWING;
				DRAW_B(-11); STEP_DRAWING;
				DRAW_C(-10); STEP_DRAWING;
				DRAW_D( -9); STEP_DRAWING;
				DRAW_A( -8); STEP_DRAWING;
				DRAW_B( -7); STEP_DRAWING;
				DRAW_C( -6); STEP_DRAWING;
				DRAW_D( -5); STEP_DRAWING;
				DRAW_A( -4); STEP_DRAWING;
				DRAW_B( -3); STEP_DRAWING;
				DRAW_C( -2); STEP_DRAWING;
				DRAW_D( -1); STEP_DRAWING;
			}
			if (spancount)
			{
				// scan drawing
				pdest += spancount;
				pz    += spancount;
				switch (spancount)
				{
				//	case 16: DRAW_A(-16); STEP_DRAWING;
					case 15: DRAW_B(-15); STEP_DRAWING;
					case 14: DRAW_C(-14); STEP_DRAWING;
					case 13: DRAW_D(-13); STEP_DRAWING;
					case 12: DRAW_A(-12); STEP_DRAWING;
					case 11: DRAW_B(-11); STEP_DRAWING;
					case 10: DRAW_C(-10); STEP_DRAWING;
					case  9: DRAW_D( -9); STEP_DRAWING;
					case  8: DRAW_A( -8); STEP_DRAWING;
					case  7: DRAW_B( -7); STEP_DRAWING;
					case  6: DRAW_C( -6); STEP_DRAWING;
					case  5: DRAW_D( -5); STEP_DRAWING;
					case  4: DRAW_A( -4); STEP_DRAWING;
					case  3: DRAW_B( -3); STEP_DRAWING;
					case  2: DRAW_C( -2); STEP_DRAWING;
					case  1: DRAW_D( -1);
					break;
				}
			}
		}
	} while ( (++pspanpackage)->count != -999999);
}

void MDL_DrawSpans_ColorKeyed (void)
{
#ifdef _arch_dreamcast
	MDL_DrawSpans_FullOpaque ();
#else
	#undef DRAW_A
	#undef DRAW_B
	#undef DRAW_C
	#undef DRAW_D
	#define DRAW_A(I)	COLORKEYED_SHADE_A(I);
	#define DRAW_B(I)	COLORKEYED_SHADE_B(I);
	#define DRAW_C(I)	COLORKEYED_SHADE_C(I);
	#define DRAW_D(I)	COLORKEYED_SHADE_D(I);
	spanpackage_t
		* pspanpackage = pstart
		;
	int
		count
	,	spancount
	,	t_s
	,	t_t
	,	llight
	,	izi
		;
	short
		* pz
		;
	byte
		* pdest
	,	* lptex
	,	pcolor
		;

	if (d_ditherlighting.value)
	{
		xykernel[0] = scan_l_kernel[0];
		xykernel[1] = scan_l_kernel[1];
		xykernel[2] = scan_l_kernel[2];
		xykernel[3] = scan_l_kernel[3];
	}
	else
	{
		xykernel[0] = null_kernel16[0];
		xykernel[1] = null_kernel16[1];
		xykernel[2] = null_kernel16[2];
		xykernel[3] = null_kernel16[3];
	}

	do // line drawing
	{
		count = d_aspancount - pspanpackage->count; // must be done before updating errorterm

		if ( (errorterm += erroradjustup) >= 0)
		{
			errorterm -= erroradjustdown;
			d_aspancount += d_countextrastep;
		}
		else
			d_aspancount += ubasestep;

		// skip lines with no pixels to draw
		if (count)
		{
			pdest = pspanpackage->pdest;

			lptex = pspanpackage->ptex;
			t_s = pspanpackage->sfrac;
			t_t = pspanpackage->tfrac;
			llight = pspanpackage->light;
			pz = pspanpackage->pz;
			izi = pspanpackage->zi;

			// prepare dither values
			#if 1
			// u v used for setting the dithering only
			u = (int)(pdest - d_viewbuffer);
			v = u / vid.rowbytes; // position of lines
			u -= v * screenwidth; // position of columns

			uo = &uofs[u & 3];
			vo =  uofs[v & 3];
			// for shading
			lvkernelY = xykernel[vo];
			LXYVa = lvkernelY[uo[0]];
			LXYVb = lvkernelY[uo[1]];
			LXYVc = lvkernelY[uo[2]];
			LXYVd = lvkernelY[uo[3]];
			#endif

			spancount = count & SCANMASK;
			count >>= SCANBITS;
			while (count--)
			{
				// scan drawing
				pdest += SCANSIZE;
				pz    += SCANSIZE;
				DRAW_A(-16); STEP_DRAWING;
				DRAW_B(-15); STEP_DRAWING;
				DRAW_C(-14); STEP_DRAWING;
				DRAW_D(-13); STEP_DRAWING;
				DRAW_A(-12); STEP_DRAWING;
				DRAW_B(-11); STEP_DRAWING;
				DRAW_C(-10); STEP_DRAWING;
				DRAW_D( -9); STEP_DRAWING;
				DRAW_A( -8); STEP_DRAWING;
				DRAW_B( -7); STEP_DRAWING;
				DRAW_C( -6); STEP_DRAWING;
				DRAW_D( -5); STEP_DRAWING;
				DRAW_A( -4); STEP_DRAWING;
				DRAW_B( -3); STEP_DRAWING;
				DRAW_C( -2); STEP_DRAWING;
				DRAW_D( -1); STEP_DRAWING;
			}
			if (spancount)
			{
				// scan drawing
				pdest += spancount;
				pz    += spancount;
				switch (spancount)
				{
				//	case 16: DRAW_A(-16); STEP_DRAWING;
					case 15: DRAW_B(-15); STEP_DRAWING;
					case 14: DRAW_C(-14); STEP_DRAWING;
					case 13: DRAW_D(-13); STEP_DRAWING;
					case 12: DRAW_A(-12); STEP_DRAWING;
					case 11: DRAW_B(-11); STEP_DRAWING;
					case 10: DRAW_C(-10); STEP_DRAWING;
					case  9: DRAW_D( -9); STEP_DRAWING;
					case  8: DRAW_A( -8); STEP_DRAWING;
					case  7: DRAW_B( -7); STEP_DRAWING;
					case  6: DRAW_C( -6); STEP_DRAWING;
					case  5: DRAW_D( -5); STEP_DRAWING;
					case  4: DRAW_A( -4); STEP_DRAWING;
					case  3: DRAW_B( -3); STEP_DRAWING;
					case  2: DRAW_C( -2); STEP_DRAWING;
					case  1: DRAW_D( -1);
					break;
				}
			}
		}
	} while ( (++pspanpackage)->count != -999999);
#endif
}

// mankrip - EF_REFLECTIVE - begin
void MDL_DrawSpans_Metal (void)
{
	#undef DRAW_A
	#undef DRAW_B
	#undef DRAW_C
	#undef DRAW_D
	#define DRAW_A(I)	FULLOPAQUE_METAL(I);
	#define DRAW_B(I)	FULLOPAQUE_METAL(I);
	#define DRAW_C(I)	FULLOPAQUE_METAL(I);
	#define DRAW_D(I)	FULLOPAQUE_METAL(I);
	spanpackage_t
		* pspanpackage = pstart
		;
	int
		count
	,	spancount
	,	t_s
	,	t_t
	,	llight
	,	izi
		;
	short
		* pz
		;
	byte
		* pdest
	,	* lptex
		;

	do // line drawing
	{
		count = d_aspancount - pspanpackage->count; // must be done before updating errorterm

		if ( (errorterm += erroradjustup) >= 0)
		{
			errorterm -= erroradjustdown;
			d_aspancount += d_countextrastep;
		}
		else
			d_aspancount += ubasestep;

		// skip lines with no pixels to draw
		if (count)
		{
			pdest = pspanpackage->pdest;

			lptex = pspanpackage->ptex;
			t_s = pspanpackage->sfrac;
			t_t = pspanpackage->tfrac;
			llight = pspanpackage->light;
			pz = pspanpackage->pz;
			izi = pspanpackage->zi;

			spancount = count & SCANMASK;
			count >>= SCANBITS;
			while (count--)
			{
				// scan drawing
				pdest += SCANSIZE;
				pz    += SCANSIZE;
				DRAW_A(-16); STEP_DRAWING;
				DRAW_B(-15); STEP_DRAWING;
				DRAW_C(-14); STEP_DRAWING;
				DRAW_D(-13); STEP_DRAWING;
				DRAW_A(-12); STEP_DRAWING;
				DRAW_B(-11); STEP_DRAWING;
				DRAW_C(-10); STEP_DRAWING;
				DRAW_D( -9); STEP_DRAWING;
				DRAW_A( -8); STEP_DRAWING;
				DRAW_B( -7); STEP_DRAWING;
				DRAW_C( -6); STEP_DRAWING;
				DRAW_D( -5); STEP_DRAWING;
				DRAW_A( -4); STEP_DRAWING;
				DRAW_B( -3); STEP_DRAWING;
				DRAW_C( -2); STEP_DRAWING;
				DRAW_D( -1); STEP_DRAWING;
			}
			if (spancount)
			{
				// scan drawing
				pdest += spancount;
				pz    += spancount;
				switch (spancount)
				{
				//	case 16: DRAW_A(-16); STEP_DRAWING;
					case 15: DRAW_B(-15); STEP_DRAWING;
					case 14: DRAW_C(-14); STEP_DRAWING;
					case 13: DRAW_D(-13); STEP_DRAWING;
					case 12: DRAW_A(-12); STEP_DRAWING;
					case 11: DRAW_B(-11); STEP_DRAWING;
					case 10: DRAW_C(-10); STEP_DRAWING;
					case  9: DRAW_D( -9); STEP_DRAWING;
					case  8: DRAW_A( -8); STEP_DRAWING;
					case  7: DRAW_B( -7); STEP_DRAWING;
					case  6: DRAW_C( -6); STEP_DRAWING;
					case  5: DRAW_D( -5); STEP_DRAWING;
					case  4: DRAW_A( -4); STEP_DRAWING;
					case  3: DRAW_B( -3); STEP_DRAWING;
					case  2: DRAW_C( -2); STEP_DRAWING;
					case  1: DRAW_D( -1);
					break;
				}
			}
		}
	} while ( (++pspanpackage)->count != -999999);
}
// mankrip - EF_REFLECTIVE - end

// 66% transparency
void MDL_DrawSpans_Blend (void) // mankrip - transparencies - edited
{
	#undef DRAW_A
	#undef DRAW_B
	#undef DRAW_C
	#undef DRAW_D
	#define DRAW_A(I)	COLORBLEND_SHADE_A(I);
	#define DRAW_B(I)	COLORBLEND_SHADE_B(I);
	#define DRAW_C(I)	COLORBLEND_SHADE_C(I);
	#define DRAW_D(I)	COLORBLEND_SHADE_D(I);
	spanpackage_t
		* pspanpackage = pstart
		;
	int
		count
	,	spancount
	,	t_s
	,	t_t
	,	llight
	,	izi
		;
	short
		* pz
		;
	byte
		* pdest
	,	* lptex
		;

	if (d_ditherlighting.value)
	{
		xykernel[0] = scan_l_kernel[0];
		xykernel[1] = scan_l_kernel[1];
		xykernel[2] = scan_l_kernel[2];
		xykernel[3] = scan_l_kernel[3];
	}
	else
	{
		xykernel[0] = null_kernel16[0];
		xykernel[1] = null_kernel16[1];
		xykernel[2] = null_kernel16[2];
		xykernel[3] = null_kernel16[3];
	}

	do // line drawing
	{
		count = d_aspancount - pspanpackage->count; // must be done before updating errorterm

		if ( (errorterm += erroradjustup) >= 0)
		{
			errorterm -= erroradjustdown;
			d_aspancount += d_countextrastep;
		}
		else
			d_aspancount += ubasestep;

		// skip lines with no pixels to draw
		if (count)
		{
			pdest = pspanpackage->pdest;

			lptex = pspanpackage->ptex;
			t_s = pspanpackage->sfrac;
			t_t = pspanpackage->tfrac;
			llight = pspanpackage->light;
			pz = pspanpackage->pz;
			izi = pspanpackage->zi;

			// prepare dither values
			#if 1
			// u v used for setting the dithering only
			u = (int)(pdest - d_viewbuffer);
			v = u / vid.rowbytes; // position of lines
			u -= v * screenwidth; // position of columns

			uo = &uofs[u & 3];
			vo =  uofs[v & 3];
			// for shading
			lvkernelY = xykernel[vo];
			LXYVa = lvkernelY[uo[0]];
			LXYVb = lvkernelY[uo[1]];
			LXYVc = lvkernelY[uo[2]];
			LXYVd = lvkernelY[uo[3]];
			#endif

			spancount = count & SCANMASK;
			count >>= SCANBITS;
			while (count--)
			{
				// scan drawing
				pdest += SCANSIZE;
				pz    += SCANSIZE;
				DRAW_A(-16); STEP_DRAWING;
				DRAW_B(-15); STEP_DRAWING;
				DRAW_C(-14); STEP_DRAWING;
				DRAW_D(-13); STEP_DRAWING;
				DRAW_A(-12); STEP_DRAWING;
				DRAW_B(-11); STEP_DRAWING;
				DRAW_C(-10); STEP_DRAWING;
				DRAW_D( -9); STEP_DRAWING;
				DRAW_A( -8); STEP_DRAWING;
				DRAW_B( -7); STEP_DRAWING;
				DRAW_C( -6); STEP_DRAWING;
				DRAW_D( -5); STEP_DRAWING;
				DRAW_A( -4); STEP_DRAWING;
				DRAW_B( -3); STEP_DRAWING;
				DRAW_C( -2); STEP_DRAWING;
				DRAW_D( -1); STEP_DRAWING;
			}
			if (spancount)
			{
				// scan drawing
				pdest += spancount;
				pz    += spancount;
				switch (spancount)
				{
				//	case 16: DRAW_A(-16); STEP_DRAWING;
					case 15: DRAW_B(-15); STEP_DRAWING;
					case 14: DRAW_C(-14); STEP_DRAWING;
					case 13: DRAW_D(-13); STEP_DRAWING;
					case 12: DRAW_A(-12); STEP_DRAWING;
					case 11: DRAW_B(-11); STEP_DRAWING;
					case 10: DRAW_C(-10); STEP_DRAWING;
					case  9: DRAW_D( -9); STEP_DRAWING;
					case  8: DRAW_A( -8); STEP_DRAWING;
					case  7: DRAW_B( -7); STEP_DRAWING;
					case  6: DRAW_C( -6); STEP_DRAWING;
					case  5: DRAW_D( -5); STEP_DRAWING;
					case  4: DRAW_A( -4); STEP_DRAWING;
					case  3: DRAW_B( -3); STEP_DRAWING;
					case  2: DRAW_C( -2); STEP_DRAWING;
					case  1: DRAW_D( -1);
					break;
				}
			}
		}
	} while ( (++pspanpackage)->count != -999999);
}
// 33% transparency
void MDL_DrawSpans_BlendBackwards (void) // mankrip - transparencies - edited
{
	#undef DRAW_A
	#undef DRAW_B
	#undef DRAW_C
	#undef DRAW_D
	#define DRAW_A(I)	BLENDCOLOR_SHADE_A(I);
	#define DRAW_B(I)	BLENDCOLOR_SHADE_B(I);
	#define DRAW_C(I)	BLENDCOLOR_SHADE_C(I);
	#define DRAW_D(I)	BLENDCOLOR_SHADE_D(I);
	spanpackage_t
		* pspanpackage = pstart
		;
	int
		count
	,	spancount
	,	t_s
	,	t_t
	,	llight
	,	izi
		;
	short
		* pz
		;
	byte
		* pdest
	,	* lptex
		;

	if (d_ditherlighting.value)
	{
		xykernel[0] = scan_l_kernel[0];
		xykernel[1] = scan_l_kernel[1];
		xykernel[2] = scan_l_kernel[2];
		xykernel[3] = scan_l_kernel[3];
	}
	else
	{
		xykernel[0] = null_kernel16[0];
		xykernel[1] = null_kernel16[1];
		xykernel[2] = null_kernel16[2];
		xykernel[3] = null_kernel16[3];
	}

	do // line drawing
	{
		count = d_aspancount - pspanpackage->count; // must be done before updating errorterm

		if ( (errorterm += erroradjustup) >= 0)
		{
			errorterm -= erroradjustdown;
			d_aspancount += d_countextrastep;
		}
		else
			d_aspancount += ubasestep;

		// skip lines with no pixels to draw
		if (count)
		{
			pdest = pspanpackage->pdest;

			lptex = pspanpackage->ptex;
			t_s = pspanpackage->sfrac;
			t_t = pspanpackage->tfrac;
			llight = pspanpackage->light;
			pz = pspanpackage->pz;
			izi = pspanpackage->zi;

			// prepare dither values
			#if 1
			// u v used for setting the dithering only
			u = (int)(pdest - d_viewbuffer);
			v = u / vid.rowbytes; // position of lines
			u -= v * screenwidth; // position of columns

			uo = &uofs[u & 3];
			vo =  uofs[v & 3];
			// for shading
			lvkernelY = xykernel[vo];
			LXYVa = lvkernelY[uo[0]];
			LXYVb = lvkernelY[uo[1]];
			LXYVc = lvkernelY[uo[2]];
			LXYVd = lvkernelY[uo[3]];
			#endif

			spancount = count & SCANMASK;
			count >>= SCANBITS;
			while (count--)
			{
				// scan drawing
				pdest += SCANSIZE;
				pz    += SCANSIZE;
				DRAW_A(-16); STEP_DRAWING;
				DRAW_B(-15); STEP_DRAWING;
				DRAW_C(-14); STEP_DRAWING;
				DRAW_D(-13); STEP_DRAWING;
				DRAW_A(-12); STEP_DRAWING;
				DRAW_B(-11); STEP_DRAWING;
				DRAW_C(-10); STEP_DRAWING;
				DRAW_D( -9); STEP_DRAWING;
				DRAW_A( -8); STEP_DRAWING;
				DRAW_B( -7); STEP_DRAWING;
				DRAW_C( -6); STEP_DRAWING;
				DRAW_D( -5); STEP_DRAWING;
				DRAW_A( -4); STEP_DRAWING;
				DRAW_B( -3); STEP_DRAWING;
				DRAW_C( -2); STEP_DRAWING;
				DRAW_D( -1); STEP_DRAWING;
			}
			if (spancount)
			{
				// scan drawing
				pdest += spancount;
				pz    += spancount;
				switch (spancount)
				{
				//	case 16: DRAW_A(-16); STEP_DRAWING;
					case 15: DRAW_B(-15); STEP_DRAWING;
					case 14: DRAW_C(-14); STEP_DRAWING;
					case 13: DRAW_D(-13); STEP_DRAWING;
					case 12: DRAW_A(-12); STEP_DRAWING;
					case 11: DRAW_B(-11); STEP_DRAWING;
					case 10: DRAW_C(-10); STEP_DRAWING;
					case  9: DRAW_D( -9); STEP_DRAWING;
					case  8: DRAW_A( -8); STEP_DRAWING;
					case  7: DRAW_B( -7); STEP_DRAWING;
					case  6: DRAW_C( -6); STEP_DRAWING;
					case  5: DRAW_D( -5); STEP_DRAWING;
					case  4: DRAW_A( -4); STEP_DRAWING;
					case  3: DRAW_B( -3); STEP_DRAWING;
					case  2: DRAW_C( -2); STEP_DRAWING;
					case  1: DRAW_D( -1);
					break;
				}
			}
		}
	} while ( (++pspanpackage)->count != -999999);
}

// shadow blending
void MDL_DrawSpans_Shadow (void) // mankrip - EF_SHADOW - edited
{
	spanpackage_t
		* pspanpackage = pstart
		;
	int		lcount;
	byte	*lpdest;
	int		lzi;
	short	*lpz;
	// mankrip - begin
	int		dist;
	byte	*shadowmap;
	shadowmap = &colorshadingmap[WITH_BRIGHTS][256] + ((32 + (int)(31.0*currententity->alpha) ) * 256);
	// mankrip - end

	do
	{
		lcount = d_aspancount - pspanpackage->count;

		errorterm += erroradjustup;
		if (errorterm >= 0)
		{
			d_aspancount += d_countextrastep;
			errorterm -= erroradjustdown;
		}
		else
			d_aspancount += ubasestep;

		if (lcount)
		{
			lpdest = pspanpackage->pdest;
			lpz = pspanpackage->pz;
			lzi = pspanpackage->zi;

			do
			{
				// mankrip - begin
				dist = (lzi >> 16) - *lpz;
				// faster, looks worse
			//	if (dist >= 0)
			//		*lpdest = 0;
				// slower, looks better:
				if (dist >= 0 && dist < 8)
					*lpdest = shadowmap[*lpdest];
				// mankrip - end
				lpdest++;
				lzi += r_zistepx;
				lpz++;
			} while (--lcount);
		}

		pspanpackage++;
	} while (pspanpackage->count != -999999);
}

// mankrip - EF_CELSHADING - begin
byte D_PolysetDraw_OutlinePixel_100 (byte lpdest, byte pixel)
{
//	return (byte)colorshadingmap[WITHOUT_BRIGHTS][pixel + ((32 + (int)(31.0*0.7) ) * 256)];
	return colorblendingmap[pixel];
}
byte D_PolysetDraw_OutlinePixel_66 (byte lpdest, byte pixel)
{
//	return alphamap[lpdest + ((byte)colorshadingmap[WITHOUT_BRIGHTS][pixel + ((32 + (int)(31.0*0.7) ) * 256)])*256];
	return colorblendingmap[lpdest + colorblendingmap[pixel]*256];
}
byte D_PolysetDraw_OutlinePixel_33 (byte lpdest, byte pixel)
{
//	return alphamap[((byte)colorshadingmap[WITHOUT_BRIGHTS][pixel + ((32 + (int)(31.0*0.5) ) * 256)]) + lpdest*256];
	return colorblendingmap[colorblendingmap[pixel] + lpdest*256];
}
void D_PolysetDrawSpans8_Outline (void)
{
/*
preencher o pixel 5 vezes, desse jeito:
 #
###
 #

preencher pixels superiores/inferiores nesses casos:
L = lateral
A = acima/abaixo
       AAA
    AAL---LA
 AAL--------LA
L-------------L
 AL--------LAA
   AL---LAA
     AAA

sempre repetir a primeira e a última linha integralmente.
? fazer uma união dos conjuntos da primeira linha e da segunda linha, pra saber o comprimento máximo da linha a ser desenhada
*/
	spanpackage_t
		* pspanpackage = pstart
		;
	int		lcount;
	byte	*lpdest;
	int		lzi;
	short	*lpz;
	int		firstline = true;
//	byte	*prevlpdest; // previous
	unsigned int	prevbegin=0, prevend=0, nextbegin, nextend; // previous
#if 0
	byte	(*DrawPixel)(byte, byte);
	if (r_drawoutlines == 1)
		DrawPixel = D_PolysetDraw_OutlinePixel_100;
	else if (r_drawoutlines == 6)
		DrawPixel = D_PolysetDraw_OutlinePixel_66;
	else /*if (r_drawoutlines == 3)*/
		DrawPixel = D_PolysetDraw_OutlinePixel_33;
#endif

	do
	{
		lcount = d_aspancount - pspanpackage->count;

		errorterm += erroradjustup;
		if (errorterm >= 0)
		{
			d_aspancount += d_countextrastep;
			errorterm -= erroradjustdown;
		}
		else
			d_aspancount += ubasestep;

		if (lcount)
		{
			lpdest = pspanpackage->pdest;
			lpz = pspanpackage->pz;
			lzi = pspanpackage->zi;

	// one more pixel on the left side
			if ((lzi >> 16) > lpz[-1])
				lpdest[-1] = colorblendingmap[*lpdest];//DrawPixel(lpdest[-1], *lpdest);
	// pixels on the top
			if (firstline)
			{
				// é possível o polígono ter uma linha só, ou seja, é possível (firstline == lastline),
				// então nesse caso é necessário desenhar tanto acima quanto abaixo.
				if (lpdest - vid.width >= vid.buffer)//(vid.buffer + vid.width*r_refdef.vrect.y + r_refdef.vrect.x))
				do
				{
					if ((lzi >> 16) > lpz[-(signed int)(vid.width)])
						lpdest[-(signed int)(vid.width)] = colorblendingmap[*lpdest];//DrawPixel(lpdest[-vid.width], *lpdest);
					// checar se a última linha sai pra fora da tela
				//	if (lpdest + vid.width < (vid.buffer + vid.width*(r_refdef.vrect.y+r_refdef.vrect.height) - (vid.width-(r_refdef.vrect.x+r_refdef.vrect.width)) ))
					if (pspanpackage[1].count == -999999) // last line
						if ((lzi >> 16) > lpz[vid.width])
							lpdest[vid.width] = colorblendingmap[*lpdest];//DrawPixel(lpdest[vid.width], *lpdest);
					lpdest++;
					lzi += r_zistepx;
					lpz++;
				} while (--lcount);
			}
	// pixels on the bottom
			else if (pspanpackage[1].count == -999999) // last line
			{
			//	if (lpdest + vid.width < (vid.buffer + vid.width*vid.height - vid.width))
			//	(r_refdef.vrect.y+r_refdef.vrect.height) -
			//	(vid.width-(r_refdef.vrect.x+r_refdef.vrect.width)) ))
				do
				{
					if ((lzi >> 16) > lpz[vid.width])
						lpdest[vid.width] = colorblendingmap[*lpdest];//DrawPixel(lpdest[vid.width], *lpdest);
					lpdest++;
					lzi += r_zistepx;
					lpz++;
				} while (--lcount);
			}
	// pixels on the center
			else
			{
				nextbegin = (unsigned int)pspanpackage[1].pdest - vid.width - 1;
				nextend = nextbegin + (unsigned int)(d_aspancount - pspanpackage[1].count) + 1;
				do
				{
					// se o pixel atual começar  antes  do de cima, desenhar ele acima.
					if ((unsigned int)lpdest < prevbegin)
					{
						if ((lzi >> 16) > lpz[-(signed int)(vid.width)])
							lpdest[-(signed int)(vid.width)] = colorblendingmap[*lpdest];//DrawPixel(lpdest[-vid.width], *lpdest);
					}
					// se o pixel atual terminar depois do de cima, desenhar ele acima.
					else if ((unsigned int)lpdest > prevend)
					{
						if ((lzi >> 16) > lpz[-(signed int)(vid.width)])
							lpdest[-(signed int)(vid.width)] = colorblendingmap[*lpdest];//DrawPixel(lpdest[-vid.width], *lpdest);
					}
					// se o pixel atual começar  antes  do de baixo, desenhar ele abaixo.
					if ((unsigned int)lpdest < nextbegin)
					{
						if ((lzi >> 16) > lpz[vid.width])
							lpdest[vid.width] = colorblendingmap[*lpdest];//DrawPixel(lpdest[vid.width], *lpdest);
					}
					// se o pixel atual terminar depois do de baixo, desenhar ele abaixo.
					else if ((unsigned int)lpdest > nextend)
					{
						if ((lzi >> 16) > lpz[vid.width])
							lpdest[vid.width] = colorblendingmap[*lpdest];//DrawPixel(lpdest[vid.width], *lpdest);
					}
					lpdest++;
					lzi += r_zistepx;
					lpz++;
				} while (--lcount);
			}
	// one more pixel on the right side
			if (((lzi-r_zistepx) >> 16) > *lpz)
				*lpdest = colorblendingmap[lpdest[-1]];//DrawPixel(*lpdest, lpdest[-1]);
			firstline = false;
		}

		prevbegin = ((unsigned int)pspanpackage->pdest + vid.width) - 1;
		if ((errorterm+erroradjustdown) >= 0)
			prevend = prevbegin + 1 + (unsigned int)(d_aspancount - d_countextrastep - pspanpackage->count);
		else
			prevend = prevbegin + 1 + (unsigned int)(d_aspancount - ubasestep - pspanpackage->count);
		pspanpackage++;
	} while (pspanpackage->count != -999999);
}

/*
================
D_RasterizeAliasPolySmooth
================
*/
void D_RasterizeAliasPolySmooth (void)
{
	int				initialleftheight, initialrightheight;
	int				*plefttop, *prighttop, *pleftbottom, *prightbottom;
	int				working_lstepx, originalcount;

	plefttop = pedgetable->pleftedgevert0;
	prighttop = pedgetable->prightedgevert0;

	pleftbottom = pedgetable->pleftedgevert1;
	prightbottom = pedgetable->prightedgevert1;

	initialleftheight = pleftbottom[1] - plefttop[1];
	initialrightheight = prightbottom[1] - prighttop[1];

//
// set the s, t, and light gradients, which are consistent across the triangle
// because being a triangle, things are affine
//
	D_PolysetCalcGradients_C (r_affinetridesc.skinwidth); // mankrip - transparencies

//
// rasterize the polygon
//

//
// scan out the top (and possibly only) part of the left edge
//
	d_pedgespanpackage = a_spans;

	ystart = plefttop[1];
	d_aspancount = plefttop[0] - prighttop[0];

	d_ptex = (byte *)r_affinetridesc.pskin + (plefttop[2] >> 16) +
			(plefttop[3] >> 16) * r_affinetridesc.skinwidth;
	d_sfrac = plefttop[2] & 0xFFFF; // mankrip - transparencies
	d_tfrac = plefttop[3] & 0xFFFF; // mankrip - transparencies
	d_light = plefttop[4];
	d_zi = plefttop[5];

	d_pdest = (byte *)d_viewbuffer +
			ystart * screenwidth + plefttop[0];
	d_pz = d_pzbuffer + ystart * d_zwidth + plefttop[0];

	if (initialleftheight == 1)
	{
		d_pedgespanpackage->pdest = d_pdest;
		d_pedgespanpackage->pz = d_pz;
		d_pedgespanpackage->count = d_aspancount;
		d_pedgespanpackage->ptex = d_ptex;

		d_pedgespanpackage->sfrac = d_sfrac;
		d_pedgespanpackage->tfrac = d_tfrac;

	// FIXME: need to clamp l, s, t, at both ends?
		d_pedgespanpackage->light = d_light;
		d_pedgespanpackage->zi = d_zi;

		d_pedgespanpackage++;
	}
	else
	{
		D_PolysetSetUpForLineScan(plefttop[0], plefttop[1],
							  pleftbottom[0], pleftbottom[1]);

		d_pzbasestep = d_zwidth + ubasestep; // mankrip - transparencies
		d_pzextrastep = d_pzbasestep + 1; // mankrip - transparencies

		d_pdestbasestep = screenwidth + ubasestep;
		d_pdestextrastep = d_pdestbasestep + 1;

	// TODO: can reuse partial expressions here

	// for negative steps in x along left edge, bias toward overflow rather than
	// underflow (sort of turning the floor () we did in the gradient calcs into
	// ceil (), but plus a little bit)
		if (ubasestep < 0)
			working_lstepx = r_lstepx - 1;
		else
			working_lstepx = r_lstepx;

		d_countextrastep = ubasestep + 1;
		d_ptexbasestep = ((r_sstepy + r_sstepx * ubasestep) >> 16) +
				((r_tstepy + r_tstepx * ubasestep) >> 16) *
				r_affinetridesc.skinwidth;
		d_sfracbasestep = (r_sstepy + r_sstepx * ubasestep) & 0xFFFF; // mankrip - transparencies
		d_tfracbasestep = (r_tstepy + r_tstepx * ubasestep) & 0xFFFF; // mankrip - transparencies
		d_lightbasestep = r_lstepy + working_lstepx * ubasestep;
		d_zibasestep = r_zistepy + r_zistepx * ubasestep;

		d_ptexextrastep = ((r_sstepy + r_sstepx * d_countextrastep) >> 16) +
				((r_tstepy + r_tstepx * d_countextrastep) >> 16) *
				r_affinetridesc.skinwidth;
		d_sfracextrastep = (r_sstepy + r_sstepx*d_countextrastep) & 0xFFFF; // mankrip - transparencies
		d_tfracextrastep = (r_tstepy + r_tstepx*d_countextrastep) & 0xFFFF; // mankrip - transparencies
		d_lightextrastep = d_lightbasestep + working_lstepx;
		d_ziextrastep = d_zibasestep + r_zistepx;

		D_PolysetScanLeftEdge_C (initialleftheight); // mankrip - transparencies
	}

//
// scan out the bottom part of the left edge, if it exists
//
	if (pedgetable->numleftedges == 2)
	{
		int		height;

		plefttop = pleftbottom;
		pleftbottom = pedgetable->pleftedgevert2;

		height = pleftbottom[1] - plefttop[1];

// TODO: make this a function; modularize this function in general

		ystart = plefttop[1];
		d_aspancount = plefttop[0] - prighttop[0];
		d_ptex = (byte *)r_affinetridesc.pskin + (plefttop[2] >> 16) +
				(plefttop[3] >> 16) * r_affinetridesc.skinwidth;
		d_sfrac = 0;
		d_tfrac = 0;
		d_light = plefttop[4];
		d_zi = plefttop[5];

		d_pdest = (byte *)d_viewbuffer + ystart * screenwidth + plefttop[0];
		d_pz = d_pzbuffer + ystart * d_zwidth + plefttop[0];

		if (height == 1)
		{
			d_pedgespanpackage->pdest = d_pdest;
			d_pedgespanpackage->pz = d_pz;
			d_pedgespanpackage->count = d_aspancount;
			d_pedgespanpackage->ptex = d_ptex;

			d_pedgespanpackage->sfrac = d_sfrac;
			d_pedgespanpackage->tfrac = d_tfrac;

		// FIXME: need to clamp l, s, t, at both ends?
			d_pedgespanpackage->light = d_light;
			d_pedgespanpackage->zi = d_zi;

			d_pedgespanpackage++;
		}
		else
		{
			D_PolysetSetUpForLineScan(plefttop[0], plefttop[1],
								  pleftbottom[0], pleftbottom[1]);

			d_pdestbasestep = screenwidth + ubasestep;
			d_pdestextrastep = d_pdestbasestep + 1;

			d_pzbasestep = d_zwidth + ubasestep; // mankrip - transparencies
			d_pzextrastep = d_pzbasestep + 1; // mankrip - transparencies

			if (ubasestep < 0)
				working_lstepx = r_lstepx - 1;
			else
				working_lstepx = r_lstepx;

			d_countextrastep = ubasestep + 1;
			d_ptexbasestep = ((r_sstepy + r_sstepx * ubasestep) >> 16) +
					((r_tstepy + r_tstepx * ubasestep) >> 16) *
					r_affinetridesc.skinwidth;
			d_sfracbasestep = (r_sstepy + r_sstepx * ubasestep) & 0xFFFF; // mankrip - transparencies
			d_tfracbasestep = (r_tstepy + r_tstepx * ubasestep) & 0xFFFF; // mankrip - transparencies
			d_lightbasestep = r_lstepy + working_lstepx * ubasestep;
			d_zibasestep = r_zistepy + r_zistepx * ubasestep;

			d_ptexextrastep = ((r_sstepy + r_sstepx * d_countextrastep) >> 16) +
					((r_tstepy + r_tstepx * d_countextrastep) >> 16) *
					r_affinetridesc.skinwidth;
			d_sfracextrastep = (r_sstepy+r_sstepx*d_countextrastep) & 0xFFFF; // mankrip - transparencies
			d_tfracextrastep = (r_tstepy+r_tstepx*d_countextrastep) & 0xFFFF; // mankrip - transparencies
			d_lightextrastep = d_lightbasestep + working_lstepx;
			d_ziextrastep = d_zibasestep + r_zistepx;

			D_PolysetScanLeftEdge_C (height); // mankrip - transparencies
		}
	}

	// scan out the top (and possibly only) part of the right edge, updating the count field
	d_pedgespanpackage = a_spans;

	D_PolysetSetUpForLineScan(prighttop[0], prighttop[1],
						  prightbottom[0], prightbottom[1]);
	d_aspancount = 0;
	d_countextrastep = ubasestep + 1;
	originalcount = a_spans[initialrightheight].count;
	a_spans[initialrightheight].count = -999999; // mark end of the spanpackages

	// mankrip - begin
	pstart = a_spans; // mankrip
	#if 1
	currententity->D_PolysetDrawSpans ();
//	D_PolysetDrawSpans8_ColorKeyed ();
	#else
	// mankrip - end
	// mankrip - EF_CELSHADING - begin
	else if (currententity->effects & EF_CELSHADING)
	{
		if (r_drawoutlines)
			D_PolysetDrawSpans8_Outline ();
		else
		{
			celtri_drawn = true;
			D_PolysetDrawSpans8_ColorKeyed ();
		}
	}
	else
	// mankrip - EF_CELSHADING - end
		D_PolysetDrawSpans8_ColorKeyed ();
	#endif

// scan out the bottom part of the right edge, if it exists
	if (pedgetable->numrightedges == 2)
	{
		int				height;

		pstart = a_spans + initialrightheight;
		pstart->count = originalcount;

		d_aspancount = prightbottom[0] - prighttop[0];

		prighttop = prightbottom;
		prightbottom = pedgetable->prightedgevert2;

		height = prightbottom[1] - prighttop[1];

		D_PolysetSetUpForLineScan(prighttop[0], prighttop[1],
							  prightbottom[0], prightbottom[1]);

		d_countextrastep = ubasestep + 1;
		a_spans[initialrightheight + height].count = -999999;
											// mark end of the spanpackages
		// mankrip - begin
		#if 1
		currententity->D_PolysetDrawSpans ();
	//	D_PolysetDrawSpans8_ColorKeyed ();
		#else
		// mankrip - end
		// mankrip - EF_CELSHADING - begin
		else if (currententity->effects & EF_CELSHADING)
		{
			if (r_drawoutlines)
				D_PolysetDrawSpans8_Outline ();
			else
				D_PolysetDrawSpans8_ColorKeyed ();
		}
		else
		// mankrip - EF_CELSHADING - end
			D_PolysetDrawSpans8_ColorKeyed ();
		#endif
	}
}


/*
================
D_PolysetSetEdgeTable
================
*/
void D_PolysetSetEdgeTable (void)
{
	int
		edgetableindex = 0	// assume the vertices are already in top to bottom order
		;
	//
	// determine which edges are right & left, and the order in which to rasterize them
	//
	if (r_p0[1] >= r_p1[1])
	{
		if (r_p0[1] == r_p1[1])
		{
			pedgetable = &edgetables[ (r_p0[1] < r_p2[1]) ? 2 : 5];
			return;
		}
		edgetableindex = 1;
	}
	if (r_p0[1] == r_p2[1])
	{
		pedgetable = &edgetables[9 - edgetableindex];
		return;
	}
	if (r_p1[1] == r_p2[1])
	{
		pedgetable = &edgetables[11 - edgetableindex];
		return;
	}

	if (r_p0[1] > r_p2[1])
		edgetableindex += 2;

	if (r_p1[1] > r_p2[1])
		edgetableindex += 4;

	pedgetable = &edgetables[edgetableindex];
}


#if 0

void D_PolysetRecursiveDrawLine (int *lp1, int *lp2)
{
	int		d;
	int		new[6];
	int 	ofs;

	d = lp2[0] - lp1[0];
	if (d < -1 || d > 1)
		goto split;
	d = lp2[1] - lp1[1];
	if (d < -1 || d > 1)
		goto split;

	return;	// line is completed

split:
// split this edge
	new[0] = (lp1[0] + lp2[0]) >> 1;
	new[1] = (lp1[1] + lp2[1]) >> 1;
	new[5] = (lp1[5] + lp2[5]) >> 1;
	new[2] = (lp1[2] + lp2[2]) >> 1;
	new[3] = (lp1[3] + lp2[3]) >> 1;
	new[4] = (lp1[4] + lp2[4]) >> 1;

// draw the point
	ofs = d_scantable[new[1]] + new[0];
	if (new[5] > d_pzbuffer[ofs])
	{
		int		pix;

		d_pzbuffer[ofs] = new[5];
		pix = skintable[new[3]>>16][new[2]>>16];
//		pix = ((byte *)acolormap)[pix + (new[4] & 0xFF00)];
		d_viewbuffer[ofs] = pix;
	}

// recursively continue
	D_PolysetRecursiveDrawLine (lp1, new);
	D_PolysetRecursiveDrawLine (new, lp2);
}

void D_PolysetRecursiveTriangle2 (int *lp1, int *lp2, int *lp3)
{
	int		d;
	int		new[4];

	d = lp2[0] - lp1[0];
	if (d < -1 || d > 1)
		goto split;
	d = lp2[1] - lp1[1];
	if (d < -1 || d > 1)
		goto split;
	return;

split:
// split this edge
	new[0] = (lp1[0] + lp2[0]) >> 1;
	new[1] = (lp1[1] + lp2[1]) >> 1;
	new[5] = (lp1[5] + lp2[5]) >> 1;
	new[2] = (lp1[2] + lp2[2]) >> 1;
	new[3] = (lp1[3] + lp2[3]) >> 1;
	new[4] = (lp1[4] + lp2[4]) >> 1;

	D_PolysetRecursiveDrawLine (new, lp3);

// recursively continue
	D_PolysetRecursiveTriangle (lp1, new, lp3);
	D_PolysetRecursiveTriangle (new, lp2, lp3);
}

#endif

