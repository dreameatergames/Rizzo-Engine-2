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
// d_scan.c
//
// Portable C scan-level rasterization code, all pixel depths.

#include "quakedef.h"
#include "r_local.h"
#include "d_local.h"

unsigned char	*r_turb_pbase, *r_turb_pdest;
fixed16_t		r_turb_s, r_turb_t, r_turb_sstep, r_turb_tstep;
int				*r_turb_turb;
int				r_turb_spancount;

void D_DrawTurbulent8Span (void);


/*
=============
D_WarpScreen

// this performs a slight compression of the screen at the same time as
// the sine warp, to keep the edges from wrapping
=============
*/
void D_WarpScreen (void)
{
	int		w, h;
	int		u,v;
	byte	*dest;
	int		*turb;
	int		*col;
	byte	**row;
	byte	*rowptr[MAXHEIGHT+(AMP2*2)];
	int		column[MAXWIDTH+(AMP2*2)];
	float	wratio, hratio;

	w = r_refdef.vrect.width;
	h = r_refdef.vrect.height;

	wratio = w / (float)scr_vrect.width;
	hratio = h / (float)scr_vrect.height;

	for (v=0 ; v<scr_vrect.height+AMP2*2 ; v++)
	{
		rowptr[v] = d_viewbuffer + (r_refdef.vrect.y * screenwidth) +
				 (screenwidth * (int)((float)v * hratio * h / (h + AMP2 * 2)));
	}

	for (u=0 ; u<scr_vrect.width+AMP2*2 ; u++)
	{
		column[u] = r_refdef.vrect.x +
				(int)((float)u * wratio * w / (w + AMP2 * 2));
	}

	turb = intsintable + ((int)(cl.time*SPEED)&(CYCLE-1));
	dest = vid.buffer + scr_vrect.y * vid.rowbytes + scr_vrect.x;

	for (v=0 ; v<scr_vrect.height ; v++, dest += vid.rowbytes)
	{
		col = &column[turb[v]];
		row = &rowptr[v];

		for (u=0 ; u<scr_vrect.width ; u+=4)
		{
			dest[u+0] = row[turb[u+0]][col[u+0]];
			dest[u+1] = row[turb[u+1]][col[u+1]];
			dest[u+2] = row[turb[u+2]][col[u+2]];
			dest[u+3] = row[turb[u+3]][col[u+3]];
		}
	}
}


#if	!id386

void D_DrawTurbulent8Span (void)
{	}

#endif	// !id386


/*
=============
Turbulent8
=============
*/
void D_DrawTurbulent16_Solid (espan_t *pspan)
{
	int				count;
	fixed16_t		snext, tnext;
	float			sdivz, tdivz, zi, z, du, dv, spancountminus1;
	float			sdivz16stepu, tdivz16stepu, zi16stepu;

	r_turb_turb = sintable + ((int)(cl.time*SPEED)&(CYCLE-1));

	r_turb_sstep = 0;	// keep compiler happy
	r_turb_tstep = 0;	// ditto

	r_turb_pbase = (unsigned char *)cacheblock;

	sdivz16stepu = d_sdivzstepu * 16;
	tdivz16stepu = d_tdivzstepu * 16;
	zi16stepu = d_zistepu * 16;


	do
	{
		r_turb_pdest = (unsigned char *)((byte *)d_viewbuffer +
				(screenwidth * pspan->v) + pspan->u);

		count = pspan->count;

	// calculate the initial s/z, t/z, 1/z, s, and t and clamp
		du = (float)pspan->u;
		dv = (float)pspan->v;

		sdivz = d_sdivzorigin + dv*d_sdivzstepv + du*d_sdivzstepu;
		tdivz = d_tdivzorigin + dv*d_tdivzstepv + du*d_tdivzstepu;
		zi = d_ziorigin + dv*d_zistepv + du*d_zistepu;
		z = (float)0x10000 / zi;	// prescale to 16.16 fixed-point

		r_turb_s = (int)(sdivz * z) + sadjust;
		if (r_turb_s > bbextents)
			r_turb_s = bbextents;
		else if (r_turb_s < 0)
			r_turb_s = 0;

		r_turb_t = (int)(tdivz * z) + tadjust;
		if (r_turb_t > bbextentt)
			r_turb_t = bbextentt;
		else if (r_turb_t < 0)
			r_turb_t = 0;

		do
		{
		// calculate s and t at the far end of the span
			if (count >= 16)
				r_turb_spancount = 16;
			else
				r_turb_spancount = count;

			count -= r_turb_spancount;

			if (count)
			{
			// calculate s/z, t/z, zi->fixed s and t at far end of span,
			// calculate s and t steps across span by shifting
				sdivz += sdivz16stepu;
				tdivz += tdivz16stepu;
				zi += zi16stepu;
				z = (float)0x10000 / zi;	// prescale to 16.16 fixed-point

				snext = (int)(sdivz * z) + sadjust;
				if (snext > bbextents)
					snext = bbextents;
				else if (snext < 16)
					snext = 16;	// prevent round-off error on <0 steps from
								//  from causing overstepping & running off the
								//  edge of the texture

				tnext = (int)(tdivz * z) + tadjust;
				if (tnext > bbextentt)
					tnext = bbextentt;
				else if (tnext < 16)
					tnext = 16;	// guard against round-off error on <0 steps

				r_turb_sstep = (snext - r_turb_s) >> 4;
				r_turb_tstep = (tnext - r_turb_t) >> 4;
			}
			else
			{
			// calculate s/z, t/z, zi->fixed s and t at last pixel in span (so
			// can't step off polygon), clamp, calculate s and t steps across
			// span by division, biasing steps low so we don't run off the
			// texture
				spancountminus1 = (float)(r_turb_spancount - 1);
				sdivz += d_sdivzstepu * spancountminus1;
				tdivz += d_tdivzstepu * spancountminus1;
				zi += d_zistepu * spancountminus1;
				z = (float)0x10000 / zi;	// prescale to 16.16 fixed-point
				snext = (int)(sdivz * z) + sadjust;
				if (snext > bbextents)
					snext = bbextents;
				else if (snext < 16)
					snext = 16;	// prevent round-off error on <0 steps from
								//  from causing overstepping & running off the
								//  edge of the texture

				tnext = (int)(tdivz * z) + tadjust;
				if (tnext > bbextentt)
					tnext = bbextentt;
				else if (tnext < 16)
					tnext = 16;	// guard against round-off error on <0 steps

				if (r_turb_spancount > 1)
				{
					r_turb_sstep = (snext - r_turb_s) / (r_turb_spancount - 1);
					r_turb_tstep = (tnext - r_turb_t) / (r_turb_spancount - 1);
				}
			}

			r_turb_s = r_turb_s & ((CYCLE<<16)-1);
			r_turb_t = r_turb_t & ((CYCLE<<16)-1);

			#if	id386 // mankrip
			D_DrawTurbulent8Span ();
			// mankrip - begin
			#else
			do
			{
				*r_turb_pdest++ = *(r_turb_pbase + ( ( ( (r_turb_t + r_turb_turb[ (r_turb_s >> 16) & (CYCLE - 1)]) >> 16) & 63) << 6) + ( ( (r_turb_s + r_turb_turb[ (r_turb_t >> 16) & (CYCLE - 1)]) >> 16) & 63));
				r_turb_s += r_turb_sstep;
				r_turb_t += r_turb_tstep;
			} while (--r_turb_spancount > 0);
			#endif
			// mankrip - end

			r_turb_s = snext;
			r_turb_t = tnext;

		} while (count > 0);

	} while ((pspan = pspan->pnext) != NULL);
}

void D_DrawTurbulent16_Blend (espan_t *pspan)
{
	int				count;
	int				izi, izistep, izistep2;//, sturb, tturb; // mankrip - translucent water
	short			*pz; // mankrip - translucent water
	fixed16_t		snext, tnext;
	float			sdivz, tdivz, zi, z, du, dv, spancountminus1;
	float			sdivz16stepu, tdivz16stepu, zi16stepu;

	r_turb_turb = sintable + ((int)(cl.time*SPEED)&(CYCLE-1));

	r_turb_sstep = 0;	// keep compiler happy
	r_turb_tstep = 0;	// ditto

	r_turb_pbase = (unsigned char *)cacheblock;

	sdivz16stepu = d_sdivzstepu * 16;
	tdivz16stepu = d_tdivzstepu * 16;
	zi16stepu = d_zistepu * 16;

	// mankrip - translucent water - begin
	// we count on FP exceptions being turned off to avoid range problems
	izistep = (int)(d_zistepu * 0x8000 * 0x10000);
	izistep2 = izistep*2;
	// mankrip - translucent water - end

	do
	{
		r_turb_pdest = (byte *)((byte *)d_viewbuffer +
				(screenwidth * pspan->v) + pspan->u);
		pz = d_pzbuffer + (d_zwidth * pspan->v) + pspan->u; // mankrip - translucent water

		count = pspan->count;

	// calculate the initial s/z, t/z, 1/z, s, and t and clamp
		du = (float)pspan->u;
		dv = (float)pspan->v;

		sdivz = d_sdivzorigin + dv*d_sdivzstepv + du*d_sdivzstepu;
		tdivz = d_tdivzorigin + dv*d_tdivzstepv + du*d_tdivzstepu;
		zi = d_ziorigin + dv*d_zistepv + du*d_zistepu;
		z = (float)0x10000 / zi;	// prescale to 16.16 fixed-point
		// we count on FP exceptions being turned off to avoid range problems // mankrip - translucent water
		izi = (int) (zi * 0x8000 * 0x10000); // mankrip - translucent water

		r_turb_s = (int)(sdivz * z) + sadjust;
		if (r_turb_s > bbextents)
			r_turb_s = bbextents;
		else if (r_turb_s < 0)
			r_turb_s = 0;

		r_turb_t = (int)(tdivz * z) + tadjust;
		if (r_turb_t > bbextentt)
			r_turb_t = bbextentt;
		else if (r_turb_t < 0)
			r_turb_t = 0;

		do
		{
		// calculate s and t at the far end of the span
			if (count >= 16)
				r_turb_spancount = 16;
			else
				r_turb_spancount = count;

			count -= r_turb_spancount;

			if (count)
			{
			// calculate s/z, t/z, zi->fixed s and t at far end of span,
			// calculate s and t steps across span by shifting
				sdivz += sdivz16stepu;
				tdivz += tdivz16stepu;
				zi += zi16stepu;
				z = (float)0x10000 / zi;	// prescale to 16.16 fixed-point

				snext = (int)(sdivz * z) + sadjust;
				if (snext > bbextents)
					snext = bbextents;
				else if (snext < 16)
					snext = 16;	// prevent round-off error on <0 steps from
								//  from causing overstepping & running off the
								//  edge of the texture

				tnext = (int)(tdivz * z) + tadjust;
				if (tnext > bbextentt)
					tnext = bbextentt;
				else if (tnext < 16)
					tnext = 16;	// guard against round-off error on <0 steps

				r_turb_sstep = (snext - r_turb_s) >> 4;
				r_turb_tstep = (tnext - r_turb_t) >> 4;
			}
			else
			{
			// calculate s/z, t/z, zi->fixed s and t at last pixel in span (so
			// can't step off polygon), clamp, calculate s and t steps across
			// span by division, biasing steps low so we don't run off the
			// texture
				spancountminus1 = (float)(r_turb_spancount - 1);
				sdivz += d_sdivzstepu * spancountminus1;
				tdivz += d_tdivzstepu * spancountminus1;
				zi += d_zistepu * spancountminus1;
				z = (float)0x10000 / zi;	// prescale to 16.16 fixed-point
				snext = (int)(sdivz * z) + sadjust;
				if (snext > bbextents)
					snext = bbextents;
				else if (snext < 16)
					snext = 16;	// prevent round-off error on <0 steps from
								//  from causing overstepping & running off the
								//  edge of the texture

				tnext = (int)(tdivz * z) + tadjust;
				if (tnext > bbextentt)
					tnext = bbextentt;
				else if (tnext < 16)
					tnext = 16;	// guard against round-off error on <0 steps

				if (r_turb_spancount > 1)
				{
					r_turb_sstep = (snext - r_turb_s) / (r_turb_spancount - 1);
					r_turb_tstep = (tnext - r_turb_t) / (r_turb_spancount - 1);
				}
			}

			r_turb_s = r_turb_s & ((CYCLE<<16)-1);
			r_turb_t = r_turb_t & ((CYCLE<<16)-1);

			// mankrip - translucent water - begin
			do
			{
				if (*pz <= (izi >> 16))
				{
					*r_turb_pdest = colorblendingmap[ (
						*(r_turb_pbase + (
							( ( (r_turb_t + r_turb_turb[ (r_turb_s >> 16) & (CYCLE - 1)]) >> 16) & 63) << 6)
						+	( ( (r_turb_s + r_turb_turb[ (r_turb_t >> 16) & (CYCLE - 1)]) >> 16) & 63))
						) * 256 + *r_turb_pdest];
				}
				r_turb_pdest++;
				izi += izistep;
				pz++;
				r_turb_s += r_turb_sstep;
				r_turb_t += r_turb_tstep;
			} while (--r_turb_spancount > 0);
			// mankrip - translucent water - end
			r_turb_s = snext;
			r_turb_t = tnext;
		} while (count > 0);
	} while ((pspan = pspan->pnext) != NULL);
}

void D_DrawTurbulent16_BlendBackwards (espan_t *pspan)
{
	int				count;
	int				izi, izistep, izistep2;//, sturb, tturb; // mankrip - translucent water
	short			*pz; // mankrip - translucent water
	fixed16_t		snext, tnext;
	float			sdivz, tdivz, zi, z, du, dv, spancountminus1;
	float			sdivz16stepu, tdivz16stepu, zi16stepu;

	r_turb_turb = sintable + ((int)(cl.time*SPEED)&(CYCLE-1));

	r_turb_sstep = 0;	// keep compiler happy
	r_turb_tstep = 0;	// ditto

	r_turb_pbase = (unsigned char *)cacheblock;

	sdivz16stepu = d_sdivzstepu * 16;
	tdivz16stepu = d_tdivzstepu * 16;
	zi16stepu = d_zistepu * 16;

	// mankrip - translucent water - begin
	// we count on FP exceptions being turned off to avoid range problems
	izistep = (int)(d_zistepu * 0x8000 * 0x10000);
	izistep2 = izistep*2;
	// mankrip - translucent water - end

	do
	{
		r_turb_pdest = (byte *)((byte *)d_viewbuffer +
				(screenwidth * pspan->v) + pspan->u);
		pz = d_pzbuffer + (d_zwidth * pspan->v) + pspan->u; // mankrip - translucent water

		count = pspan->count;

	// calculate the initial s/z, t/z, 1/z, s, and t and clamp
		du = (float)pspan->u;
		dv = (float)pspan->v;

		sdivz = d_sdivzorigin + dv*d_sdivzstepv + du*d_sdivzstepu;
		tdivz = d_tdivzorigin + dv*d_tdivzstepv + du*d_tdivzstepu;
		zi = d_ziorigin + dv*d_zistepv + du*d_zistepu;
		z = (float)0x10000 / zi;	// prescale to 16.16 fixed-point
		// we count on FP exceptions being turned off to avoid range problems // mankrip - translucent water
		izi = (int) (zi * 0x8000 * 0x10000); // mankrip - translucent water

		r_turb_s = (int)(sdivz * z) + sadjust;
		if (r_turb_s > bbextents)
			r_turb_s = bbextents;
		else if (r_turb_s < 0)
			r_turb_s = 0;

		r_turb_t = (int)(tdivz * z) + tadjust;
		if (r_turb_t > bbextentt)
			r_turb_t = bbextentt;
		else if (r_turb_t < 0)
			r_turb_t = 0;

		do
		{
		// calculate s and t at the far end of the span
			if (count >= 16)
				r_turb_spancount = 16;
			else
				r_turb_spancount = count;

			count -= r_turb_spancount;

			if (count)
			{
			// calculate s/z, t/z, zi->fixed s and t at far end of span,
			// calculate s and t steps across span by shifting
				sdivz += sdivz16stepu;
				tdivz += tdivz16stepu;
				zi += zi16stepu;
				z = (float)0x10000 / zi;	// prescale to 16.16 fixed-point

				snext = (int)(sdivz * z) + sadjust;
				if (snext > bbextents)
					snext = bbextents;
				else if (snext < 16)
					snext = 16;	// prevent round-off error on <0 steps from
								//  from causing overstepping & running off the
								//  edge of the texture

				tnext = (int)(tdivz * z) + tadjust;
				if (tnext > bbextentt)
					tnext = bbextentt;
				else if (tnext < 16)
					tnext = 16;	// guard against round-off error on <0 steps

				r_turb_sstep = (snext - r_turb_s) >> 4;
				r_turb_tstep = (tnext - r_turb_t) >> 4;
			}
			else
			{
			// calculate s/z, t/z, zi->fixed s and t at last pixel in span (so
			// can't step off polygon), clamp, calculate s and t steps across
			// span by division, biasing steps low so we don't run off the
			// texture
				spancountminus1 = (float)(r_turb_spancount - 1);
				sdivz += d_sdivzstepu * spancountminus1;
				tdivz += d_tdivzstepu * spancountminus1;
				zi += d_zistepu * spancountminus1;
				z = (float)0x10000 / zi;	// prescale to 16.16 fixed-point
				snext = (int)(sdivz * z) + sadjust;
				if (snext > bbextents)
					snext = bbextents;
				else if (snext < 16)
					snext = 16;	// prevent round-off error on <0 steps from
								//  from causing overstepping & running off the
								//  edge of the texture

				tnext = (int)(tdivz * z) + tadjust;
				if (tnext > bbextentt)
					tnext = bbextentt;
				else if (tnext < 16)
					tnext = 16;	// guard against round-off error on <0 steps

				if (r_turb_spancount > 1)
				{
					r_turb_sstep = (snext - r_turb_s) / (r_turb_spancount - 1);
					r_turb_tstep = (tnext - r_turb_t) / (r_turb_spancount - 1);
				}
			}

			r_turb_s = r_turb_s & ((CYCLE<<16)-1);
			r_turb_t = r_turb_t & ((CYCLE<<16)-1);

			// mankrip - translucent water - begin
			do
			{
				if (*pz <= (izi >> 16))
				{
					*r_turb_pdest = colorblendingmap[ (
						*(r_turb_pbase + (
							( ( (r_turb_t + r_turb_turb[ (r_turb_s >> 16) & (CYCLE - 1)]) >> 16) & 63) << 6)
						+	( ( (r_turb_s + r_turb_turb[ (r_turb_t >> 16) & (CYCLE - 1)]) >> 16) & 63))
						) + *r_turb_pdest * 256];
				}
				r_turb_pdest++;
				izi += izistep;
				pz++;
				r_turb_s += r_turb_sstep;
				r_turb_t += r_turb_tstep;
			} while (--r_turb_spancount > 0);
			// mankrip - translucent water - end
			r_turb_s = snext;
			r_turb_t = tnext;
		} while (count > 0);
	} while ((pspan = pspan->pnext) != NULL);
}

#if 0
int D_SetTurbulentFromCvar (float mode, float alpha)
{
	if (mode > 0.0f)
	{
		if (alpha < 1.0f)
		{
			if (r_overdraw)
			{
				if (alpha <= 0.0f)
					return true;
			}
			else
			{
				if (alpha > 0.0f)
					r_foundtranslucency = true;
				return true;
			}
		}
		else if (r_overdraw)
			return true;
	}
	else if (r_wateralpha.value < 1.0f)
	{
		if (r_overdraw)
		{
			if (r_wateralpha.value <= 0.0f)
				return true;
		}
		else
		{
			if (r_wateralpha.value > 0.0f)
				r_foundtranslucency = true;
			return true;
		}
	}
	else if (r_overdraw)
		return true;
	return false;
}

void D_SetTurbulent (int type)
{
	if (type & SURF_DRAWTURB)
	{
		if (type & SURF_BLEND_WATER)
		{
			if (D_SetTurbulentFromCvar (r_water_blend_mode.value, r_water_blend_alpha.value))
				return;
		}
		else if (type & SURF_BLEND_SLIME)
		{
			if (D_SetTurbulentFromCvar (r_slime_blend_mode.value, r_slime_blend_alpha.value))
				return;
		}
		else if (type & SURF_BLEND_LAVA)
		{
			if (D_SetTurbulentFromCvar (r_lava_blend_mode.value, r_lava_blend_alpha.value))
				return;
		}
		else if (type & SURF_BLEND_PORTAL)
		{
			if (D_SetTurbulentFromCvar (r_portal_blend_mode.value, r_portal_blend_alpha.value))
				return;
		}
		else if (r_wateralpha.value < 1.0f)
		{
			if (r_overdraw)
			{
				if (r_wateralpha.value <= 0.0f)
					return;
			}
			else
			{
				if (r_wateralpha.value > 0.0f)
					r_foundtranslucency = true;
				return;
			}
		}
		else if (r_overdraw)
			return;
	}
};

static void (*D_DrawTurbulent[COLORBLENDING_MAPS][3]) (void) =
{
	{	//BLEND_WEIGHTEDMEAN
		D_DrawTurbulent16_Blend
	,	D_DrawTurbulent16_BlendBackwards
	,	D_DrawTurbulent16_Solid
	}
,	{	//BLEND_ADDITIVE
		D_DrawTurbulent16_Blend
	,	D_DrawTurbulent16_BlendBackwards
	,	D_DrawTurbulent16_BlendBackwards
	}
,	{	//BLEND_SMOKE
		D_DrawTurbulent16_Blend
	,	D_DrawTurbulent16_BlendBackwards
	,	D_DrawTurbulent16_Solid
	}
};

void Turbulent8 (espan_t *span)
{
	// mankrip - translucent water - begin
	#if 1
	if (r_overdraw)
		D_DrawTurbulent16_Blend (span);
	//	D_DrawTurbulent[minmaxclamp (0, (int) r_water_blend_mode.value, 1)][minmaxclamp (0, (int) r_water_blend_map.value, COLORBLENDING_MAPS - 1)][minmaxclamp (0, (int) (r_water_blend_alpha.value * 3.0f) - 1, 2)] (pspan);
	else
		D_DrawTurbulent16_Solid (span);
	#else
	D_DrawTurbulent16_Stipple (span);
	#endif
}
#endif

/*
=============
D_DrawSpans8
=============
*/
// mankrip - d_dither - begin
static int dither_kernel[2][4] = // Y X 01
{
	{
		 -8192	// !Y !X 0
	,	  8192	// !Y  X 0
	,	-24576	// !Y !X 1
	,	 24576	// !Y  X 1
	}
,	{
		 24576	//  Y !X 0
	,	-24576	//  Y  X 0
	,	 -8192	//  Y !X 1
	,	  8192	//  Y  X 1
	}
};
static int null_kernel[2][4] =
{
	{ 0, 0, 0, 0 }
,	{ 0, 0, 0, 0 }
};

static int
// integers for dithering
	*ykernel
,	*xykernel[2]
,	u
,	v
,	idiths
,	iditht
,	idiths2
,	iditht2
,	X
,	XY0a
,	XY1a
,	XY0b
,	XY1b
,	bbextents_max
,	bbextentt_max
	;

#define TEXELPRECISION	16 // bits of precision per texel coordinate
#define	TEXELSIZE		(1 << TEXELPRECISION) // 128 0x80
#define SCANBITS		4
#define SCANSIZE		(1 << SCANBITS)

#define bbextents_min		TEXELSIZE//(TEXELSIZE * 2)
#define bbextents_min_next	(bbextents_min + SCANSIZE)
#define bbextentt_min		bbextents_min
#define bbextentt_min_next	bbextents_min_next

#define UNFILTER_FULLOPAQUE(i)														pdest[i] = pbase[ (  s         >> TEXELPRECISION) + (  t         >> TEXELPRECISION) * cachewidth];
#define DITHERED_FULLOPAQUE_A(i)													pdest[i] = pbase[ ( (s + XY0a) >> TEXELPRECISION) + ( (t + XY1a) >> TEXELPRECISION) * cachewidth];
#define DITHERED_FULLOPAQUE_B(i)													pdest[i] = pbase[ ( (s + XY0b) >> TEXELPRECISION) + ( (t + XY1b) >> TEXELPRECISION) * cachewidth];
#define DITHERED_COLORKEYED_A(i)	if (pz[i] <= (pdepth = (short)(izi >> 16))) if ( (pcolor = pbase[ ( (s + XY0a) >> TEXELPRECISION) + ( (t + XY1a) >> TEXELPRECISION) * cachewidth]) != TRANSPARENT_COLOR) { pdest[i] = pcolor; pz[i] = pdepth; }
#define DITHERED_COLORKEYED_B(i)	if (pz[i] <= (pdepth = (short)(izi >> 16))) if ( (pcolor = pbase[ ( (s + XY0b) >> TEXELPRECISION) + ( (t + XY1b) >> TEXELPRECISION) * cachewidth]) != TRANSPARENT_COLOR) { pdest[i] = pcolor; pz[i] = pdepth; }
// normal
#define DITHERED_COLORBLEND_A(i)	if (pz[i] <= (izi >> 16) )   pdest[i] = colorblendingmap[  pbase[ ( (s + XY0a) >> TEXELPRECISION) + ( (t + XY1a) >> TEXELPRECISION) * cachewidth] | (pdest[i] << 8)];
#define DITHERED_COLORBLEND_B(i)	if (pz[i] <= (izi >> 16) )   pdest[i] = colorblendingmap[  pbase[ ( (s + XY0b) >> TEXELPRECISION) + ( (t + XY1b) >> TEXELPRECISION) * cachewidth] | (pdest[i] << 8)];
// backwards
#define DITHERED_BLENDCOLOR_A(i)	if (pz[i] <= (izi >> 16) )   pdest[i] = colorblendingmap[ (pbase[ ( (s + XY0a) >> TEXELPRECISION) + ( (t + XY1a) >> TEXELPRECISION) * cachewidth] << 8) | pdest[i]];
#define DITHERED_BLENDCOLOR_B(i)	if (pz[i] <= (izi >> 16) )   pdest[i] = colorblendingmap[ (pbase[ ( (s + XY0b) >> TEXELPRECISION) + ( (t + XY1b) >> TEXELPRECISION) * cachewidth] << 8) | pdest[i]];

// mankrip - d_dither - end
void D_DrawSpans_dither (espan_t *pspan, int dither)
{
	// mankrip - begin
	#undef DRAW_A
	#undef DRAW_B
	#define DRAW_A(i)	DITHERED_FULLOPAQUE_A(i)
	#define DRAW_B(i)	DITHERED_FULLOPAQUE_B(i)
	// mankrip - end

	int			count, spancount;
	byte		*pbase, *pdest;
	fixed16_t	s, t, snext, tnext, sstep, tstep;
	float		sdivz, tdivz, zi, z, du, dv, spancountminus1;
	float		sdivzstepu, tdivzstepu, zistepu; // qbism ( http://forums.inside3d.com/viewtopic.php?t=2717 )

	sstep = 0;	// keep compiler happy
	tstep = 0;	// ditto

	pbase = (byte *)cacheblock;

	// qbism ( http://forums.inside3d.com/viewtopic.php?t=2717 ) - begin
	sdivzstepu = d_sdivzstepu * 16.0f;
	tdivzstepu = d_tdivzstepu * 16.0f;
	zistepu = d_zistepu * 16.0f;
	// qbism ( http://forums.inside3d.com/viewtopic.php?t=2717 ) - end

	// mankrip - begin
	bbextents_max = bbextents - bbextents_min;
	bbextentt_max = bbextentt - bbextentt_min;

	if (dither)
	{
		xykernel[0] = dither_kernel[0];
		xykernel[1] = dither_kernel[1];
	}
	else
	{
		xykernel[0] = null_kernel[0];
		xykernel[1] = null_kernel[1];
	}
	// mankrip - end

	do
	{
		// mankrip - begin
		u = pspan->u;
		v = pspan->v;
		du = (float)u;
		dv = (float)v;

		// prepare dither values
		ykernel = xykernel[v & 1];
		X = ! ( (v + (u + pspan->count)) & 1);
		XY0a = ykernel[0 |  X];
		XY1a = ykernel[2 |  X];
		XY0b = ykernel[0 | !X];
		XY1b = ykernel[2 | !X];

		pdest = (byte *) ( (byte *)d_viewbuffer + (screenwidth * v) + u);
		// mankrip - end

		// calculate the initial s/z, t/z, 1/z, s, and t and clamp
		sdivz = d_sdivzorigin + dv*d_sdivzstepv + du*d_sdivzstepu;
		tdivz = d_tdivzorigin + dv*d_tdivzstepv + du*d_tdivzstepu;
		zi = d_ziorigin + dv*d_zistepv + du*d_zistepu;
		z = (float)0x10000 / zi;   // prescale to 16.16 fixed-point

		s = (int) (sdivz * z) + sadjust;
		t = (int) (tdivz * z) + tadjust;
		// mankrip - begin
		s = minmaxclamp (bbextents_min, s, bbextents_max);
		t = minmaxclamp (bbextentt_min, t, bbextentt_max);

		spancount = pspan->count % 16;
		if (spancount)
		// mankrip - end
		{

			// calculate s/z, t/z, zi->fixed s and t at last pixel in span (so can't step off polygon),
			// clamp, calculate s and t steps across span by division, biasing steps low so we don't run off the texture
			spancountminus1 = (float)(spancount - 1);
			// qbism ( http://forums.inside3d.com/viewtopic.php?t=2717 ) - begin
			sdivz += d_sdivzstepu * spancountminus1;
			tdivz += d_tdivzstepu * spancountminus1;
			zi += d_zistepu * spancountminus1;
			// qbism ( http://forums.inside3d.com/viewtopic.php?t=2717 ) - end
			z = (float)0x10000 / zi;   // prescale to 16.16 fixed-point

			snext = (int)(sdivz * z) + sadjust;
			tnext = (int)(tdivz * z) + tadjust;
			// mankrip - begin
			snext = minmaxclamp (bbextents_min_next, snext, bbextents_max);
			tnext = minmaxclamp (bbextentt_min_next, tnext, bbextentt_max);
			// mankrip - end

			if (spancount > 1)
			{
				sstep = (snext - s) / (spancount - 1);
				tstep = (tnext - t) / (spancount - 1);
			}

		// mankrip - begin
			pdest += spancount;
			switch (spancount)
			{
			//	case 16: DRAW_A(-16); s += sstep; t += tstep;
				case 15: DRAW_B(-15); s += sstep; t += tstep;
				case 14: DRAW_A(-14); s += sstep; t += tstep;
				case 13: DRAW_B(-13); s += sstep; t += tstep;
				case 12: DRAW_A(-12); s += sstep; t += tstep;
				case 11: DRAW_B(-11); s += sstep; t += tstep;
				case 10: DRAW_A(-10); s += sstep; t += tstep;
				case  9: DRAW_B( -9); s += sstep; t += tstep;
				case  8: DRAW_A( -8); s += sstep; t += tstep;
				case  7: DRAW_B( -7); s += sstep; t += tstep;
				case  6: DRAW_A( -6); s += sstep; t += tstep;
				case  5: DRAW_B( -5); s += sstep; t += tstep;
				case  4: DRAW_A( -4); s += sstep; t += tstep;
				case  3: DRAW_B( -3); s += sstep; t += tstep;
				case  2: DRAW_A( -2); s += sstep; t += tstep;
				case  1: DRAW_B( -1); s = snext; t = tnext;
				break;
			}
		}
		count = pspan->count >> 4;
		while (count--)
		// mankrip - end
		{
			// calculate s/z, t/z, zi->fixed s and t at far end of span,
			// calculate s and t steps across span by shifting
			// qbism ( http://forums.inside3d.com/viewtopic.php?t=2717 ) - begin
			sdivz += sdivzstepu;
			tdivz += tdivzstepu;
			zi += zistepu;
			// qbism ( http://forums.inside3d.com/viewtopic.php?t=2717 ) - end
			z = (float)0x10000 / zi;   // prescale to 16.16 fixed-point

			snext = (int) (sdivz * z) + sadjust;
			tnext = (int) (tdivz * z) + tadjust;
			// mankrip - begin
			snext = minmaxclamp (bbextents_min_next, snext, bbextents_max);
			tnext = minmaxclamp (bbextentt_min_next, tnext, bbextentt_max);
			// mankrip - end

			sstep = (snext - s) >> 4;
			tstep = (tnext - t) >> 4;
			// qbism ( http://forums.inside3d.com/viewtopic.php?t=2717 ) - end

			// mankrip - begin
			pdest += 16;
			DRAW_A(-16); s += sstep; t += tstep;
			DRAW_B(-15); s += sstep; t += tstep;
			DRAW_A(-14); s += sstep; t += tstep;
			DRAW_B(-13); s += sstep; t += tstep;
			DRAW_A(-12); s += sstep; t += tstep;
			DRAW_B(-11); s += sstep; t += tstep;
			DRAW_A(-10); s += sstep; t += tstep;
			DRAW_B( -9); s += sstep; t += tstep;
			DRAW_A( -8); s += sstep; t += tstep;
			DRAW_B( -7); s += sstep; t += tstep;
			DRAW_A( -6); s += sstep; t += tstep;
			DRAW_B( -5); s += sstep; t += tstep;
			DRAW_A( -4); s += sstep; t += tstep;
			DRAW_B( -3); s += sstep; t += tstep;
			DRAW_A( -2); s += sstep; t += tstep;
			DRAW_B( -1); s = snext; t = tnext;
			// mankrip - end
		}
	} while ( (pspan = pspan->pnext) != NULL);
}

#if	!id386
void D_DrawSpans16 (espan_t *pspan)
{
	// mankrip - begin
	#undef DRAW_A
	#undef DRAW_B
	#define DRAW_A(i)	UNFILTER_FULLOPAQUE(i)
	#define DRAW_B(i)	UNFILTER_FULLOPAQUE(i)
	// mankrip - end

	int			count, spancount;
	byte		*pbase, *pdest;
	fixed16_t	s, t, snext, tnext, sstep, tstep;
	float		sdivz, tdivz, zi, z, du, dv, spancountminus1;
	float		sdivzstepu, tdivzstepu, zistepu; // qbism ( http://forums.inside3d.com/viewtopic.php?t=2717 )

	sstep = 0;	// keep compiler happy
	tstep = 0;	// ditto

	pbase = (byte *)cacheblock;

	// qbism ( http://forums.inside3d.com/viewtopic.php?t=2717 ) - begin
	sdivzstepu = d_sdivzstepu * 16.0f;
	tdivzstepu = d_tdivzstepu * 16.0f;
	zistepu = d_zistepu * 16.0f;
	// qbism ( http://forums.inside3d.com/viewtopic.php?t=2717 ) - end

	// mankrip - begin
	bbextents_max = bbextents - bbextents_min;
	bbextentt_max = bbextentt - bbextentt_min;
	// mankrip - end

	do
	{
		// mankrip - begin
		u = pspan->u;
		v = pspan->v;
		du = (float)u;
		dv = (float)v;

		pdest = (byte *) ( (byte *)d_viewbuffer + (screenwidth * v) + u);
		// mankrip - end

		// calculate the initial s/z, t/z, 1/z, s, and t and clamp
		sdivz = d_sdivzorigin + dv*d_sdivzstepv + du*d_sdivzstepu;
		tdivz = d_tdivzorigin + dv*d_tdivzstepv + du*d_tdivzstepu;
		zi = d_ziorigin + dv*d_zistepv + du*d_zistepu;
		z = (float)0x10000 / zi;   // prescale to 16.16 fixed-point

		s = (int) (sdivz * z) + sadjust;
		t = (int) (tdivz * z) + tadjust;
		// mankrip - begin
		s = minmaxclamp (bbextents_min, s, bbextents_max);
		t = minmaxclamp (bbextentt_min, t, bbextentt_max);

		spancount = pspan->count % 16;
		if (spancount)
		// mankrip - end
		{

			// calculate s/z, t/z, zi->fixed s and t at last pixel in span (so can't step off polygon),
			// clamp, calculate s and t steps across span by division, biasing steps low so we don't run off the texture
			spancountminus1 = (float)(spancount - 1);
			// qbism ( http://forums.inside3d.com/viewtopic.php?t=2717 ) - begin
			sdivz += d_sdivzstepu * spancountminus1;
			tdivz += d_tdivzstepu * spancountminus1;
			zi += d_zistepu * spancountminus1;
			// qbism ( http://forums.inside3d.com/viewtopic.php?t=2717 ) - end
			z = (float)0x10000 / zi;   // prescale to 16.16 fixed-point

			snext = (int)(sdivz * z) + sadjust;
			tnext = (int)(tdivz * z) + tadjust;
			// mankrip - begin
			snext = minmaxclamp (bbextents_min_next, snext, bbextents_max);
			tnext = minmaxclamp (bbextentt_min_next, tnext, bbextentt_max);
			// mankrip - end

			if (spancount > 1)
			{
				sstep = (snext - s) / (spancount - 1);
				tstep = (tnext - t) / (spancount - 1);
			}

		// mankrip - begin
			pdest += spancount;
			switch (spancount)
			{
			//	case 16: DRAW_A(-16); s += sstep; t += tstep;
				case 15: DRAW_B(-15); s += sstep; t += tstep;
				case 14: DRAW_A(-14); s += sstep; t += tstep;
				case 13: DRAW_B(-13); s += sstep; t += tstep;
				case 12: DRAW_A(-12); s += sstep; t += tstep;
				case 11: DRAW_B(-11); s += sstep; t += tstep;
				case 10: DRAW_A(-10); s += sstep; t += tstep;
				case  9: DRAW_B( -9); s += sstep; t += tstep;
				case  8: DRAW_A( -8); s += sstep; t += tstep;
				case  7: DRAW_B( -7); s += sstep; t += tstep;
				case  6: DRAW_A( -6); s += sstep; t += tstep;
				case  5: DRAW_B( -5); s += sstep; t += tstep;
				case  4: DRAW_A( -4); s += sstep; t += tstep;
				case  3: DRAW_B( -3); s += sstep; t += tstep;
				case  2: DRAW_A( -2); s += sstep; t += tstep;
				case  1: DRAW_B( -1); s = snext; t = tnext;
				break;
			}
		}
		count = pspan->count >> 4;
		while (count--)
		// mankrip - end
		{
			// calculate s/z, t/z, zi->fixed s and t at far end of span,
			// calculate s and t steps across span by shifting
			// qbism ( http://forums.inside3d.com/viewtopic.php?t=2717 ) - begin
			sdivz += sdivzstepu;
			tdivz += tdivzstepu;
			zi += zistepu;
			// qbism ( http://forums.inside3d.com/viewtopic.php?t=2717 ) - end
			z = (float)0x10000 / zi;   // prescale to 16.16 fixed-point

			snext = (int) (sdivz * z) + sadjust;
			tnext = (int) (tdivz * z) + tadjust;
			// mankrip - begin
			snext = minmaxclamp (bbextents_min_next, snext, bbextents_max);
			tnext = minmaxclamp (bbextentt_min_next, tnext, bbextentt_max);
			// mankrip - end

			sstep = (snext - s) >> 4;
			tstep = (tnext - t) >> 4;
			// qbism ( http://forums.inside3d.com/viewtopic.php?t=2717 ) - end

			// mankrip - begin
			pdest += 16;
			DRAW_A(-16); s += sstep; t += tstep;
			DRAW_B(-15); s += sstep; t += tstep;
			DRAW_A(-14); s += sstep; t += tstep;
			DRAW_B(-13); s += sstep; t += tstep;
			DRAW_A(-12); s += sstep; t += tstep;
			DRAW_B(-11); s += sstep; t += tstep;
			DRAW_A(-10); s += sstep; t += tstep;
			DRAW_B( -9); s += sstep; t += tstep;
			DRAW_A( -8); s += sstep; t += tstep;
			DRAW_B( -7); s += sstep; t += tstep;
			DRAW_A( -6); s += sstep; t += tstep;
			DRAW_B( -5); s += sstep; t += tstep;
			DRAW_A( -4); s += sstep; t += tstep;
			DRAW_B( -3); s += sstep; t += tstep;
			DRAW_A( -2); s += sstep; t += tstep;
			DRAW_B( -1); s = snext; t = tnext;
			// mankrip - end
		}
	} while ( (pspan = pspan->pnext) != NULL);
}

void D_DrawSpans8 (espan_t *pspan)
{	D_DrawSpans16 (pspan);	} // mankrip
#endif

void D_DrawSpans16_Blend (espan_t *pspan, int dither) // mankrip
{
	// mankrip - begin
	#undef DRAW_A
	#undef DRAW_B
	#define DRAW_A(i)	DITHERED_COLORBLEND_A(i); izi += izistep;
	#define DRAW_B(i)	DITHERED_COLORBLEND_B(i); izi += izistep;
	// mankrip - end

	int			count, spancount;
	byte		*pbase, *pdest, pcolor; // mankrip
	fixed16_t	s, t, snext, tnext, sstep, tstep;
	float		sdivz, tdivz, zi, z, du, dv, spancountminus1;
	float		sdivzstepu, tdivzstepu, zistepu; // qbism ( http://forums.inside3d.com/viewtopic.php?t=2717 )
	int			izi, izistep; // mankrip
	short		*pz, pdepth; // mankrip

	sstep = 0;	// keep compiler happy
	tstep = 0;	// ditto

	pbase = (byte *)cacheblock;

	// qbism ( http://forums.inside3d.com/viewtopic.php?t=2717 ) - begin
	sdivzstepu = d_sdivzstepu * 16.0f;
	tdivzstepu = d_tdivzstepu * 16.0f;
	zistepu = d_zistepu * 16.0f;
	// qbism ( http://forums.inside3d.com/viewtopic.php?t=2717 ) - end

	// mankrip - begin
	// we count on FP exceptions being turned off to avoid range problems
	izistep = (int)(d_zistepu * 0x8000 * 0x10000);

	bbextents_max = bbextents - bbextents_min;
	bbextentt_max = bbextentt - bbextentt_min;

	if (dither)
	{
		xykernel[0] = dither_kernel[0];
		xykernel[1] = dither_kernel[1];
	}
	else
	{
		xykernel[0] = null_kernel[0];
		xykernel[1] = null_kernel[1];
	}
	// mankrip - end

	do
	{
		// mankrip - begin
		u = pspan->u;
		v = pspan->v;
		du = (float)u;
		dv = (float)v;

		// prepare dither values
		ykernel = xykernel[v & 1];
		X = ! ( (v + (u + pspan->count)) & 1);
		XY0a = ykernel[0 |  X];
		XY1a = ykernel[2 |  X];
		XY0b = ykernel[0 | !X];
		XY1b = ykernel[2 | !X];

		pdest = (byte *) ( (byte *)d_viewbuffer + (screenwidth * v) + u);
		pz = d_pzbuffer + (d_zwidth * v) + u;
		// mankrip - end

		// calculate the initial s/z, t/z, 1/z, s, and t and clamp
		sdivz = d_sdivzorigin + dv*d_sdivzstepv + du*d_sdivzstepu;
		tdivz = d_tdivzorigin + dv*d_tdivzstepv + du*d_tdivzstepu;
		zi = d_ziorigin + dv*d_zistepv + du*d_zistepu;
		z = (float)0x10000 / zi;   // prescale to 16.16 fixed-point
		// we count on FP exceptions being turned off to avoid range problems // mankrip
		izi = (int) (zi * 0x8000 * 0x10000); // mankrip

		s = (int) (sdivz * z) + sadjust;
		t = (int) (tdivz * z) + tadjust;
		// mankrip - begin
		s = minmaxclamp (bbextents_min, s, bbextents_max);
		t = minmaxclamp (bbextentt_min, t, bbextentt_max);

		spancount = pspan->count % 16;
		if (spancount)
		// mankrip - end
		{

			// calculate s/z, t/z, zi->fixed s and t at last pixel in span (so can't step off polygon),
			// clamp, calculate s and t steps across span by division, biasing steps low so we don't run off the texture
			spancountminus1 = (float)(spancount - 1);
			// qbism ( http://forums.inside3d.com/viewtopic.php?t=2717 ) - begin
			sdivz += d_sdivzstepu * spancountminus1;
			tdivz += d_tdivzstepu * spancountminus1;
			zi += d_zistepu * spancountminus1;
			// qbism ( http://forums.inside3d.com/viewtopic.php?t=2717 ) - end
			z = (float)0x10000 / zi;   // prescale to 16.16 fixed-point

			snext = (int)(sdivz * z) + sadjust;
			tnext = (int)(tdivz * z) + tadjust;
			// mankrip - begin
			snext = minmaxclamp (bbextents_min_next, snext, bbextents_max);
			tnext = minmaxclamp (bbextentt_min_next, tnext, bbextentt_max);
			// mankrip - end

			if (spancount > 1)
			{
				sstep = (snext - s) / (spancount - 1);
				tstep = (tnext - t) / (spancount - 1);
			}

		// mankrip - begin
			pdest += spancount;
			pz += spancount;
			switch (spancount)
			{
			//	case 16: DRAW_A(-16); s += sstep; t += tstep;
				case 15: DRAW_B(-15); s += sstep; t += tstep;
				case 14: DRAW_A(-14); s += sstep; t += tstep;
				case 13: DRAW_B(-13); s += sstep; t += tstep;
				case 12: DRAW_A(-12); s += sstep; t += tstep;
				case 11: DRAW_B(-11); s += sstep; t += tstep;
				case 10: DRAW_A(-10); s += sstep; t += tstep;
				case  9: DRAW_B( -9); s += sstep; t += tstep;
				case  8: DRAW_A( -8); s += sstep; t += tstep;
				case  7: DRAW_B( -7); s += sstep; t += tstep;
				case  6: DRAW_A( -6); s += sstep; t += tstep;
				case  5: DRAW_B( -5); s += sstep; t += tstep;
				case  4: DRAW_A( -4); s += sstep; t += tstep;
				case  3: DRAW_B( -3); s += sstep; t += tstep;
				case  2: DRAW_A( -2); s += sstep; t += tstep;
				case  1: DRAW_B( -1); s = snext; t = tnext;
				break;
			}
		}
		count = pspan->count >> 4;
		while (count--)
		// mankrip - end
		{
			// calculate s/z, t/z, zi->fixed s and t at far end of span,
			// calculate s and t steps across span by shifting
			// qbism ( http://forums.inside3d.com/viewtopic.php?t=2717 ) - begin
			sdivz += sdivzstepu;
			tdivz += tdivzstepu;
			zi += zistepu;
			// qbism ( http://forums.inside3d.com/viewtopic.php?t=2717 ) - end
			z = (float)0x10000 / zi;   // prescale to 16.16 fixed-point

			snext = (int) (sdivz * z) + sadjust;
			tnext = (int) (tdivz * z) + tadjust;
			// mankrip - begin
			snext = minmaxclamp (bbextents_min_next, snext, bbextents_max);
			tnext = minmaxclamp (bbextentt_min_next, tnext, bbextentt_max);
			// mankrip - end

			sstep = (snext - s) >> 4;
			tstep = (tnext - t) >> 4;
			// qbism ( http://forums.inside3d.com/viewtopic.php?t=2717 ) - end

			// mankrip - begin
			pdest += 16;
			pz += 16;
			DRAW_A(-16); s += sstep; t += tstep;
			DRAW_B(-15); s += sstep; t += tstep;
			DRAW_A(-14); s += sstep; t += tstep;
			DRAW_B(-13); s += sstep; t += tstep;
			DRAW_A(-12); s += sstep; t += tstep;
			DRAW_B(-11); s += sstep; t += tstep;
			DRAW_A(-10); s += sstep; t += tstep;
			DRAW_B( -9); s += sstep; t += tstep;
			DRAW_A( -8); s += sstep; t += tstep;
			DRAW_B( -7); s += sstep; t += tstep;
			DRAW_A( -6); s += sstep; t += tstep;
			DRAW_B( -5); s += sstep; t += tstep;
			DRAW_A( -4); s += sstep; t += tstep;
			DRAW_B( -3); s += sstep; t += tstep;
			DRAW_A( -2); s += sstep; t += tstep;
			DRAW_B( -1); s = snext; t = tnext;
			// mankrip - end
		}
	} while ( (pspan = pspan->pnext) != NULL);
}

void D_DrawSpans16_BlendBackwards (espan_t *pspan, int dither)
{
	// mankrip - begin
	#undef DRAW_A
	#undef DRAW_B
	#define DRAW_A(i)	DITHERED_BLENDCOLOR_A(i); izi += izistep;
	#define DRAW_B(i)	DITHERED_BLENDCOLOR_B(i); izi += izistep;
	// mankrip - end

	int			count, spancount;
	byte		*pbase, *pdest, pcolor; // mankrip
	fixed16_t	s, t, snext, tnext, sstep, tstep;
	float		sdivz, tdivz, zi, z, du, dv, spancountminus1;
	float		sdivzstepu, tdivzstepu, zistepu; // qbism ( http://forums.inside3d.com/viewtopic.php?t=2717 )
	int			izi, izistep; // mankrip
	short		*pz, pdepth; // mankrip

	sstep = 0;	// keep compiler happy
	tstep = 0;	// ditto

	pbase = (byte *)cacheblock;

	// qbism ( http://forums.inside3d.com/viewtopic.php?t=2717 ) - begin
	sdivzstepu = d_sdivzstepu * 16.0f;
	tdivzstepu = d_tdivzstepu * 16.0f;
	zistepu = d_zistepu * 16.0f;
	// qbism ( http://forums.inside3d.com/viewtopic.php?t=2717 ) - end

	// mankrip - begin
	// we count on FP exceptions being turned off to avoid range problems
	izistep = (int)(d_zistepu * 0x8000 * 0x10000);

	bbextents_max = bbextents - bbextents_min;
	bbextentt_max = bbextentt - bbextentt_min;

	if (dither)
	{
		xykernel[0] = dither_kernel[0];
		xykernel[1] = dither_kernel[1];
	}
	else
	{
		xykernel[0] = null_kernel[0];
		xykernel[1] = null_kernel[1];
	}
	// mankrip - end

	do
	{
		// mankrip - begin
		u = pspan->u;
		v = pspan->v;
		du = (float)u;
		dv = (float)v;

		// prepare dither values
		ykernel = xykernel[v & 1];
		X = ! ( (v + (u + pspan->count)) & 1);
		XY0a = ykernel[0 |  X];
		XY1a = ykernel[2 |  X];
		XY0b = ykernel[0 | !X];
		XY1b = ykernel[2 | !X];

		pdest = (byte *) ( (byte *)d_viewbuffer + (screenwidth * v) + u);
		pz = d_pzbuffer + (d_zwidth * v) + u;
		// mankrip - end

		// calculate the initial s/z, t/z, 1/z, s, and t and clamp
		sdivz = d_sdivzorigin + dv*d_sdivzstepv + du*d_sdivzstepu;
		tdivz = d_tdivzorigin + dv*d_tdivzstepv + du*d_tdivzstepu;
		zi = d_ziorigin + dv*d_zistepv + du*d_zistepu;
		z = (float)0x10000 / zi;   // prescale to 16.16 fixed-point
		// we count on FP exceptions being turned off to avoid range problems // mankrip
		izi = (int) (zi * 0x8000 * 0x10000); // mankrip

		s = (int) (sdivz * z) + sadjust;
		t = (int) (tdivz * z) + tadjust;
		// mankrip - begin
		s = minmaxclamp (bbextents_min, s, bbextents_max);
		t = minmaxclamp (bbextentt_min, t, bbextentt_max);

		spancount = pspan->count % 16;
		if (spancount)
		// mankrip - end
		{

			// calculate s/z, t/z, zi->fixed s and t at last pixel in span (so can't step off polygon),
			// clamp, calculate s and t steps across span by division, biasing steps low so we don't run off the texture
			spancountminus1 = (float)(spancount - 1);
			// qbism ( http://forums.inside3d.com/viewtopic.php?t=2717 ) - begin
			sdivz += d_sdivzstepu * spancountminus1;
			tdivz += d_tdivzstepu * spancountminus1;
			zi += d_zistepu * spancountminus1;
			// qbism ( http://forums.inside3d.com/viewtopic.php?t=2717 ) - end
			z = (float)0x10000 / zi;   // prescale to 16.16 fixed-point

			snext = (int)(sdivz * z) + sadjust;
			tnext = (int)(tdivz * z) + tadjust;
			// mankrip - begin
			snext = minmaxclamp (bbextents_min_next, snext, bbextents_max);
			tnext = minmaxclamp (bbextentt_min_next, tnext, bbextentt_max);
			// mankrip - end

			if (spancount > 1)
			{
				sstep = (snext - s) / (spancount - 1);
				tstep = (tnext - t) / (spancount - 1);
			}

		// mankrip - begin
			pdest += spancount;
			pz += spancount;
			switch (spancount)
			{
			//	case 16: DRAW_A(-16); s += sstep; t += tstep;
				case 15: DRAW_B(-15); s += sstep; t += tstep;
				case 14: DRAW_A(-14); s += sstep; t += tstep;
				case 13: DRAW_B(-13); s += sstep; t += tstep;
				case 12: DRAW_A(-12); s += sstep; t += tstep;
				case 11: DRAW_B(-11); s += sstep; t += tstep;
				case 10: DRAW_A(-10); s += sstep; t += tstep;
				case  9: DRAW_B( -9); s += sstep; t += tstep;
				case  8: DRAW_A( -8); s += sstep; t += tstep;
				case  7: DRAW_B( -7); s += sstep; t += tstep;
				case  6: DRAW_A( -6); s += sstep; t += tstep;
				case  5: DRAW_B( -5); s += sstep; t += tstep;
				case  4: DRAW_A( -4); s += sstep; t += tstep;
				case  3: DRAW_B( -3); s += sstep; t += tstep;
				case  2: DRAW_A( -2); s += sstep; t += tstep;
				case  1: DRAW_B( -1); s = snext; t = tnext;
				break;
			}
		}
		count = pspan->count >> 4;
		while (count--)
		// mankrip - end
		{
			// calculate s/z, t/z, zi->fixed s and t at far end of span,
			// calculate s and t steps across span by shifting
			// qbism ( http://forums.inside3d.com/viewtopic.php?t=2717 ) - begin
			sdivz += sdivzstepu;
			tdivz += tdivzstepu;
			zi += zistepu;
			// qbism ( http://forums.inside3d.com/viewtopic.php?t=2717 ) - end
			z = (float)0x10000 / zi;   // prescale to 16.16 fixed-point

			snext = (int) (sdivz * z) + sadjust;
			tnext = (int) (tdivz * z) + tadjust;
			// mankrip - begin
			snext = minmaxclamp (bbextents_min_next, snext, bbextents_max);
			tnext = minmaxclamp (bbextentt_min_next, tnext, bbextentt_max);
			// mankrip - end

			sstep = (snext - s) >> 4;
			tstep = (tnext - t) >> 4;
			// qbism ( http://forums.inside3d.com/viewtopic.php?t=2717 ) - end

			// mankrip - begin
			pdest += 16;
			pz += 16;
			DRAW_A(-16); s += sstep; t += tstep;
			DRAW_B(-15); s += sstep; t += tstep;
			DRAW_A(-14); s += sstep; t += tstep;
			DRAW_B(-13); s += sstep; t += tstep;
			DRAW_A(-12); s += sstep; t += tstep;
			DRAW_B(-11); s += sstep; t += tstep;
			DRAW_A(-10); s += sstep; t += tstep;
			DRAW_B( -9); s += sstep; t += tstep;
			DRAW_A( -8); s += sstep; t += tstep;
			DRAW_B( -7); s += sstep; t += tstep;
			DRAW_A( -6); s += sstep; t += tstep;
			DRAW_B( -5); s += sstep; t += tstep;
			DRAW_A( -4); s += sstep; t += tstep;
			DRAW_B( -3); s += sstep; t += tstep;
			DRAW_A( -2); s += sstep; t += tstep;
			DRAW_B( -1); s = snext; t = tnext;
			// mankrip - end
		}
	} while ( (pspan = pspan->pnext) != NULL);
}

void D_DrawSpans16_ColorKeyed (espan_t *pspan, int dither)
{
	// mankrip - begin
	#undef DRAW_A
	#undef DRAW_B
	#define DRAW_A(i)	DITHERED_COLORKEYED_A(i); izi += izistep;
	#define DRAW_B(i)	DITHERED_COLORKEYED_B(i); izi += izistep;
	// mankrip - end

	int			count, spancount;
	byte		*pbase, *pdest, pcolor; // mankrip
	fixed16_t	s, t, snext, tnext, sstep, tstep;
	float		sdivz, tdivz, zi, z, du, dv, spancountminus1;
	float		sdivzstepu, tdivzstepu, zistepu; // qbism ( http://forums.inside3d.com/viewtopic.php?t=2717 )
	int			izi, izistep; // mankrip
	short		*pz, pdepth; // mankrip

	sstep = 0;	// keep compiler happy
	tstep = 0;	// ditto

	pbase = (byte *)cacheblock;

	// qbism ( http://forums.inside3d.com/viewtopic.php?t=2717 ) - begin
	sdivzstepu = d_sdivzstepu * 16.0f;
	tdivzstepu = d_tdivzstepu * 16.0f;
	zistepu = d_zistepu * 16.0f;
	// qbism ( http://forums.inside3d.com/viewtopic.php?t=2717 ) - end

	// mankrip - begin
	// we count on FP exceptions being turned off to avoid range problems
	izistep = (int)(d_zistepu * 0x8000 * 0x10000);

	bbextents_max = bbextents - bbextents_min;
	bbextentt_max = bbextentt - bbextentt_min;

	if (dither)
	{
		xykernel[0] = dither_kernel[0];
		xykernel[1] = dither_kernel[1];
	}
	else
	{
		xykernel[0] = null_kernel[0];
		xykernel[1] = null_kernel[1];
	}
	// mankrip - end

	do
	{
		// mankrip - begin
		u = pspan->u;
		v = pspan->v;
		du = (float)u;
		dv = (float)v;

		// prepare dither values
		ykernel = xykernel[v & 1];
		X = ! ( (v + (u + pspan->count)) & 1);
		XY0a = ykernel[0 |  X];
		XY1a = ykernel[2 |  X];
		XY0b = ykernel[0 | !X];
		XY1b = ykernel[2 | !X];

		pdest = (byte *) ( (byte *)d_viewbuffer + (screenwidth * v) + u);
		pz = d_pzbuffer + (d_zwidth * v) + u;
		// mankrip - end

		// calculate the initial s/z, t/z, 1/z, s, and t and clamp
		sdivz = d_sdivzorigin + dv*d_sdivzstepv + du*d_sdivzstepu;
		tdivz = d_tdivzorigin + dv*d_tdivzstepv + du*d_tdivzstepu;
		zi = d_ziorigin + dv*d_zistepv + du*d_zistepu;
		z = (float)0x10000 / zi;   // prescale to 16.16 fixed-point
		// we count on FP exceptions being turned off to avoid range problems // mankrip
		izi = (int) (zi * 0x8000 * 0x10000); // mankrip

		s = (int) (sdivz * z) + sadjust;
		t = (int) (tdivz * z) + tadjust;
		// mankrip - begin
		s = minmaxclamp (bbextents_min, s, bbextents_max);
		t = minmaxclamp (bbextentt_min, t, bbextentt_max);

		spancount = pspan->count % 16;
		if (spancount)
		// mankrip - end
		{

			// calculate s/z, t/z, zi->fixed s and t at last pixel in span (so can't step off polygon),
			// clamp, calculate s and t steps across span by division, biasing steps low so we don't run off the texture
			spancountminus1 = (float)(spancount - 1);
			// qbism ( http://forums.inside3d.com/viewtopic.php?t=2717 ) - begin
			sdivz += d_sdivzstepu * spancountminus1;
			tdivz += d_tdivzstepu * spancountminus1;
			zi += d_zistepu * spancountminus1;
			// qbism ( http://forums.inside3d.com/viewtopic.php?t=2717 ) - end
			z = (float)0x10000 / zi;   // prescale to 16.16 fixed-point

			snext = (int)(sdivz * z) + sadjust;
			tnext = (int)(tdivz * z) + tadjust;
			// mankrip - begin
			snext = minmaxclamp (bbextents_min_next, snext, bbextents_max);
			tnext = minmaxclamp (bbextentt_min_next, tnext, bbextentt_max);
			// mankrip - end

			if (spancount > 1)
			{
				sstep = (snext - s) / (spancount - 1);
				tstep = (tnext - t) / (spancount - 1);
			}

		// mankrip - begin
			pdest += spancount;
			pz += spancount;
			switch (spancount)
			{
			//	case 16: DRAW_A(-16); s += sstep; t += tstep;
				case 15: DRAW_B(-15); s += sstep; t += tstep;
				case 14: DRAW_A(-14); s += sstep; t += tstep;
				case 13: DRAW_B(-13); s += sstep; t += tstep;
				case 12: DRAW_A(-12); s += sstep; t += tstep;
				case 11: DRAW_B(-11); s += sstep; t += tstep;
				case 10: DRAW_A(-10); s += sstep; t += tstep;
				case  9: DRAW_B( -9); s += sstep; t += tstep;
				case  8: DRAW_A( -8); s += sstep; t += tstep;
				case  7: DRAW_B( -7); s += sstep; t += tstep;
				case  6: DRAW_A( -6); s += sstep; t += tstep;
				case  5: DRAW_B( -5); s += sstep; t += tstep;
				case  4: DRAW_A( -4); s += sstep; t += tstep;
				case  3: DRAW_B( -3); s += sstep; t += tstep;
				case  2: DRAW_A( -2); s += sstep; t += tstep;
				case  1: DRAW_B( -1); s = snext; t = tnext;
				break;
			}
		}
		count = pspan->count >> 4;
		while (count--)
		// mankrip - end
		{
			// calculate s/z, t/z, zi->fixed s and t at far end of span,
			// calculate s and t steps across span by shifting
			// qbism ( http://forums.inside3d.com/viewtopic.php?t=2717 ) - begin
			sdivz += sdivzstepu;
			tdivz += tdivzstepu;
			zi += zistepu;
			// qbism ( http://forums.inside3d.com/viewtopic.php?t=2717 ) - end
			z = (float)0x10000 / zi;   // prescale to 16.16 fixed-point

			snext = (int) (sdivz * z) + sadjust;
			tnext = (int) (tdivz * z) + tadjust;
			// mankrip - begin
			snext = minmaxclamp (bbextents_min_next, snext, bbextents_max);
			tnext = minmaxclamp (bbextentt_min_next, tnext, bbextentt_max);
			// mankrip - end

			sstep = (snext - s) >> 4;
			tstep = (tnext - t) >> 4;
			// qbism ( http://forums.inside3d.com/viewtopic.php?t=2717 ) - end

			// mankrip - begin
			pdest += 16;
			pz += 16;
			DRAW_A(-16); s += sstep; t += tstep;
			DRAW_B(-15); s += sstep; t += tstep;
			DRAW_A(-14); s += sstep; t += tstep;
			DRAW_B(-13); s += sstep; t += tstep;
			DRAW_A(-12); s += sstep; t += tstep;
			DRAW_B(-11); s += sstep; t += tstep;
			DRAW_A(-10); s += sstep; t += tstep;
			DRAW_B( -9); s += sstep; t += tstep;
			DRAW_A( -8); s += sstep; t += tstep;
			DRAW_B( -7); s += sstep; t += tstep;
			DRAW_A( -6); s += sstep; t += tstep;
			DRAW_B( -5); s += sstep; t += tstep;
			DRAW_A( -4); s += sstep; t += tstep;
			DRAW_B( -3); s += sstep; t += tstep;
			DRAW_A( -2); s += sstep; t += tstep;
			DRAW_B( -1); s = snext; t = tnext;
			// mankrip - end
		}
	} while ( (pspan = pspan->pnext) != NULL);
}


// mankrip - begin
void D_DrawZSpans (espan_t *pspan)
{
	double
		zi
		;
	unsigned short
		*pdest
	,	*izi_p
		;
	int
		* intpdest
	,	count
	,	doublecount
	,	remainder
	,	izistep
	,	izi
	,	izistep2
	,	izi2
		;

	izi_p  = (unsigned short *)&izi;
	izi_p++;

	// FIXME: check for clamping/range problems
	// we count on FP exceptions being turned off to avoid range problems
	izistep = (int) (d_zistepu * 0x8000 * 0x10000);
	izistep2 = izistep << 1; // mankrip

	do
	{
		pdest = zspantable[pspan->v] + pspan->u; // mankrip

		count = pspan->count;

		// calculate the initial 1/z
		zi = d_ziorigin + d_zistepv * (float)pspan->v + d_zistepu * (float)pspan->u; // mankrip - edited

		// we count on FP exceptions being turned off to avoid range problems
		izi = (int) (zi * 0x8000 * 0x10000);

		if ( (long)pdest & 0x02)
		{
		//	*pdest++ = (short) (izi >> 16);
			*pdest++ = (short)*izi_p;
			izi += izistep;
			count--;
		}
		// mankrip - begin
		izi2 = izi + izistep;

		if ( (doublecount = count >> 1))
		{
			remainder = doublecount & 31;
			doublecount >>= 5;
			while (doublecount--)
			{
				pdest += 64;
				intpdest = (int *)pdest;
				intpdest[-32] = *izi_p | (izi2 & 0xFFFF0000); izi += izistep2; izi2 += izistep2;
				intpdest[-31] = *izi_p | (izi2 & 0xFFFF0000); izi += izistep2; izi2 += izistep2;
				intpdest[-30] = *izi_p | (izi2 & 0xFFFF0000); izi += izistep2; izi2 += izistep2;
				intpdest[-29] = *izi_p | (izi2 & 0xFFFF0000); izi += izistep2; izi2 += izistep2;
				intpdest[-28] = *izi_p | (izi2 & 0xFFFF0000); izi += izistep2; izi2 += izistep2;
				intpdest[-27] = *izi_p | (izi2 & 0xFFFF0000); izi += izistep2; izi2 += izistep2;
				intpdest[-26] = *izi_p | (izi2 & 0xFFFF0000); izi += izistep2; izi2 += izistep2;
				intpdest[-25] = *izi_p | (izi2 & 0xFFFF0000); izi += izistep2; izi2 += izistep2;
				intpdest[-24] = *izi_p | (izi2 & 0xFFFF0000); izi += izistep2; izi2 += izistep2;
				intpdest[-23] = *izi_p | (izi2 & 0xFFFF0000); izi += izistep2; izi2 += izistep2;
				intpdest[-22] = *izi_p | (izi2 & 0xFFFF0000); izi += izistep2; izi2 += izistep2;
				intpdest[-21] = *izi_p | (izi2 & 0xFFFF0000); izi += izistep2; izi2 += izistep2;
				intpdest[-20] = *izi_p | (izi2 & 0xFFFF0000); izi += izistep2; izi2 += izistep2;
				intpdest[-19] = *izi_p | (izi2 & 0xFFFF0000); izi += izistep2; izi2 += izistep2;
				intpdest[-18] = *izi_p | (izi2 & 0xFFFF0000); izi += izistep2; izi2 += izistep2;
				intpdest[-17] = *izi_p | (izi2 & 0xFFFF0000); izi += izistep2; izi2 += izistep2;
				intpdest[-16] = *izi_p | (izi2 & 0xFFFF0000); izi += izistep2; izi2 += izistep2;
				intpdest[-15] = *izi_p | (izi2 & 0xFFFF0000); izi += izistep2; izi2 += izistep2;
				intpdest[-14] = *izi_p | (izi2 & 0xFFFF0000); izi += izistep2; izi2 += izistep2;
				intpdest[-13] = *izi_p | (izi2 & 0xFFFF0000); izi += izistep2; izi2 += izistep2;
				intpdest[-12] = *izi_p | (izi2 & 0xFFFF0000); izi += izistep2; izi2 += izistep2;
				intpdest[-11] = *izi_p | (izi2 & 0xFFFF0000); izi += izistep2; izi2 += izistep2;
				intpdest[-10] = *izi_p | (izi2 & 0xFFFF0000); izi += izistep2; izi2 += izistep2;
				intpdest[ -9] = *izi_p | (izi2 & 0xFFFF0000); izi += izistep2; izi2 += izistep2;
				intpdest[ -8] = *izi_p | (izi2 & 0xFFFF0000); izi += izistep2; izi2 += izistep2;
				intpdest[ -7] = *izi_p | (izi2 & 0xFFFF0000); izi += izistep2; izi2 += izistep2;
				intpdest[ -6] = *izi_p | (izi2 & 0xFFFF0000); izi += izistep2; izi2 += izistep2;
				intpdest[ -5] = *izi_p | (izi2 & 0xFFFF0000); izi += izistep2; izi2 += izistep2;
				intpdest[ -4] = *izi_p | (izi2 & 0xFFFF0000); izi += izistep2; izi2 += izistep2;
				intpdest[ -3] = *izi_p | (izi2 & 0xFFFF0000); izi += izistep2; izi2 += izistep2;
				intpdest[ -2] = *izi_p | (izi2 & 0xFFFF0000); izi += izistep2; izi2 += izistep2;
				intpdest[ -1] = *izi_p | (izi2 & 0xFFFF0000); izi += izistep2; izi2 += izistep2;
			}
			if (remainder)
			{
				pdest += (remainder << 1);
				intpdest = (int *)pdest;
				switch (remainder)
				{
				//	case 32: intpdest[-32] = *izi_p | (izi2 & 0xFFFF0000); izi += izistep2; izi2 += izistep2;
					case 31: intpdest[-31] = *izi_p | (izi2 & 0xFFFF0000); izi += izistep2; izi2 += izistep2;
					case 30: intpdest[-30] = *izi_p | (izi2 & 0xFFFF0000); izi += izistep2; izi2 += izistep2;
					case 29: intpdest[-29] = *izi_p | (izi2 & 0xFFFF0000); izi += izistep2; izi2 += izistep2;
					case 28: intpdest[-28] = *izi_p | (izi2 & 0xFFFF0000); izi += izistep2; izi2 += izistep2;
					case 27: intpdest[-27] = *izi_p | (izi2 & 0xFFFF0000); izi += izistep2; izi2 += izistep2;
					case 26: intpdest[-26] = *izi_p | (izi2 & 0xFFFF0000); izi += izistep2; izi2 += izistep2;
					case 25: intpdest[-25] = *izi_p | (izi2 & 0xFFFF0000); izi += izistep2; izi2 += izistep2;
					case 24: intpdest[-24] = *izi_p | (izi2 & 0xFFFF0000); izi += izistep2; izi2 += izistep2;
					case 23: intpdest[-23] = *izi_p | (izi2 & 0xFFFF0000); izi += izistep2; izi2 += izistep2;
					case 22: intpdest[-22] = *izi_p | (izi2 & 0xFFFF0000); izi += izistep2; izi2 += izistep2;
					case 21: intpdest[-21] = *izi_p | (izi2 & 0xFFFF0000); izi += izistep2; izi2 += izistep2;
					case 20: intpdest[-20] = *izi_p | (izi2 & 0xFFFF0000); izi += izistep2; izi2 += izistep2;
					case 19: intpdest[-19] = *izi_p | (izi2 & 0xFFFF0000); izi += izistep2; izi2 += izistep2;
					case 18: intpdest[-18] = *izi_p | (izi2 & 0xFFFF0000); izi += izistep2; izi2 += izistep2;
					case 17: intpdest[-17] = *izi_p | (izi2 & 0xFFFF0000); izi += izistep2; izi2 += izistep2;
					case 16: intpdest[-16] = *izi_p | (izi2 & 0xFFFF0000); izi += izistep2; izi2 += izistep2;
					case 15: intpdest[-15] = *izi_p | (izi2 & 0xFFFF0000); izi += izistep2; izi2 += izistep2;
					case 14: intpdest[-14] = *izi_p | (izi2 & 0xFFFF0000); izi += izistep2; izi2 += izistep2;
					case 13: intpdest[-13] = *izi_p | (izi2 & 0xFFFF0000); izi += izistep2; izi2 += izistep2;
					case 12: intpdest[-12] = *izi_p | (izi2 & 0xFFFF0000); izi += izistep2; izi2 += izistep2;
					case 11: intpdest[-11] = *izi_p | (izi2 & 0xFFFF0000); izi += izistep2; izi2 += izistep2;
					case 10: intpdest[-10] = *izi_p | (izi2 & 0xFFFF0000); izi += izistep2; izi2 += izistep2;
					case  9: intpdest[ -9] = *izi_p | (izi2 & 0xFFFF0000); izi += izistep2; izi2 += izistep2;
					case  8: intpdest[ -8] = *izi_p | (izi2 & 0xFFFF0000); izi += izistep2; izi2 += izistep2;
					case  7: intpdest[ -7] = *izi_p | (izi2 & 0xFFFF0000); izi += izistep2; izi2 += izistep2;
					case  6: intpdest[ -6] = *izi_p | (izi2 & 0xFFFF0000); izi += izistep2; izi2 += izistep2;
					case  5: intpdest[ -5] = *izi_p | (izi2 & 0xFFFF0000); izi += izistep2; izi2 += izistep2;
					case  4: intpdest[ -4] = *izi_p | (izi2 & 0xFFFF0000); izi += izistep2; izi2 += izistep2;
					case  3: intpdest[ -3] = *izi_p | (izi2 & 0xFFFF0000); izi += izistep2; izi2 += izistep2;
					case  2: intpdest[ -2] = *izi_p | (izi2 & 0xFFFF0000); izi += izistep2; izi2 += izistep2;
					case  1: intpdest[ -1] = *izi_p | (izi2 & 0xFFFF0000); izi += izistep2; izi2 += izistep2;
					default:
					break;
				}
			}
		}
		if (count & 1)
			*pdest = *izi_p;
	} while ( (pspan = pspan->pnext));
}


void D_ClearZSpans (espan_t *pspan)
{
	short
		* pdest
		;
	int
		* intpdest
	,	count
	,	doublecount
	,	remainder
		;

	// place the surface depth effectively at infinite distance from the viewpoint
	#define IZI (-1 * 0x8000 * 0x10000)
	#define IZI1 (IZI >> 16)
	#define IZI2 ( (0xFFFF0000 & IZI) | IZI1)

	do
	{
		pdest = zspantable[pspan->v] + pspan->u;

		count = pspan->count;

		if ( (long)pdest & 0x02)
		{
			*pdest++ = IZI1;
			count--;
		}

		if ( (doublecount = count >> 1))
		{
			remainder = doublecount & 31;
			doublecount >>= 5;
			while (doublecount--)
			{
				pdest += 64;
				intpdest = (int *)pdest;
				intpdest[-32] = IZI2;
				intpdest[-31] = IZI2;
				intpdest[-30] = IZI2;
				intpdest[-29] = IZI2;
				intpdest[-28] = IZI2;
				intpdest[-27] = IZI2;
				intpdest[-26] = IZI2;
				intpdest[-25] = IZI2;
				intpdest[-24] = IZI2;
				intpdest[-23] = IZI2;
				intpdest[-22] = IZI2;
				intpdest[-21] = IZI2;
				intpdest[-20] = IZI2;
				intpdest[-19] = IZI2;
				intpdest[-18] = IZI2;
				intpdest[-17] = IZI2;
				intpdest[-16] = IZI2;
				intpdest[-15] = IZI2;
				intpdest[-14] = IZI2;
				intpdest[-13] = IZI2;
				intpdest[-12] = IZI2;
				intpdest[-11] = IZI2;
				intpdest[-10] = IZI2;
				intpdest[ -9] = IZI2;
				intpdest[ -8] = IZI2;
				intpdest[ -7] = IZI2;
				intpdest[ -6] = IZI2;
				intpdest[ -5] = IZI2;
				intpdest[ -4] = IZI2;
				intpdest[ -3] = IZI2;
				intpdest[ -2] = IZI2;
				intpdest[ -1] = IZI2;
			}
			if (remainder)
			{
				pdest += (remainder << 1);
				intpdest = (int *)pdest;
				switch (remainder)
				{
				//	case 32: intpdest[-32] = IZI2;
					case 31: intpdest[-31] = IZI2;
					case 30: intpdest[-30] = IZI2;
					case 29: intpdest[-29] = IZI2;
					case 28: intpdest[-28] = IZI2;
					case 27: intpdest[-27] = IZI2;
					case 26: intpdest[-26] = IZI2;
					case 25: intpdest[-25] = IZI2;
					case 24: intpdest[-24] = IZI2;
					case 23: intpdest[-23] = IZI2;
					case 22: intpdest[-22] = IZI2;
					case 21: intpdest[-21] = IZI2;
					case 20: intpdest[-20] = IZI2;
					case 19: intpdest[-19] = IZI2;
					case 18: intpdest[-18] = IZI2;
					case 17: intpdest[-17] = IZI2;
					case 16: intpdest[-16] = IZI2;
					case 15: intpdest[-15] = IZI2;
					case 14: intpdest[-14] = IZI2;
					case 13: intpdest[-13] = IZI2;
					case 12: intpdest[-12] = IZI2;
					case 11: intpdest[-11] = IZI2;
					case 10: intpdest[-10] = IZI2;
					case  9: intpdest[ -9] = IZI2;
					case  8: intpdest[ -8] = IZI2;
					case  7: intpdest[ -7] = IZI2;
					case  6: intpdest[ -6] = IZI2;
					case  5: intpdest[ -5] = IZI2;
					case  4: intpdest[ -4] = IZI2;
					case  3: intpdest[ -3] = IZI2;
					case  2: intpdest[ -2] = IZI2;
					case  1: intpdest[ -1] = IZI2;
					default:
					break;
				}
			}
		}
		if (count & 1)
			*pdest = IZI1;

	} while ( (pspan = pspan->pnext));
	#undef IZI
	#undef IZI1
	#undef IZI2
}
// mankrip - end
