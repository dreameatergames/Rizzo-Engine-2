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
// d_sky.c

#include "quakedef.h"
#include "r_local.h"
#include "d_local.h"

#define SKY_SPAN_SHIFT	5
#define SKY_SPAN_MAX	(1 << SKY_SPAN_SHIFT) // 32

// mankrip - begin
extern byte
	* skyunderlay
,	* skyoverlay
	;
static byte
	* pdest
,	over
	;
static float
	timespeed1
,	timespeed2
	;
static int
	count
,	spancount
,	spancountminus1
,	u
,	v
,	stipple
	;
static fixed16_t
	s
,	t
,	snext
,	tnext
,	sstep
,	tstep
,	s2
,	t2
,	snext2
,	tnext2
,	sstep2
,	tstep2
	;
static espan_t
	* pspan
	;
// mankrip - end



void D_Sky_uv_To_st (int u, int v, fixed16_t *s, fixed16_t *t, fixed16_t *s2, fixed16_t *t2) // mankrip - smooth sky - edited
{
	float
		wu
	,	wv
		;
	vec3_t
		end
		;

	// ToChriS - begin
	wu = (u - xcenter) / xscale;
	wv = (ycenter - v) / yscale;

	end[0] =  vpn[0] + wu*vright[0] + wv*vup[0];
	end[1] =  vpn[1] + wu*vright[1] + wv*vup[1];
	end[2] = (vpn[2] + wu*vright[2] + wv*vup[2]) * 3.0f; // 3 // mankrip - edited
	// ToChriS - end
	VectorNormalize (end);
	*s  = (int) ( (timespeed1 + 6 * (SKYSIZE / 2 - 1) * end[0]) * 0x10000); // mankrip - smooth sky - edited
	*t  = (int) ( (timespeed1 + 6 * (SKYSIZE / 2 - 1) * end[1]) * 0x10000); // mankrip - smooth sky - edited
	*s2 = (int) ( (timespeed2 + 6 * (SKYSIZE / 2 - 1) * end[0]) * 0x10000); // mankrip - smooth sky
	*t2 = (int) ( (timespeed2 + 6 * (SKYSIZE / 2 - 1) * end[1]) * 0x10000); // mankrip - smooth sky
}



/*
=================
D_DrawSkyScans8
=================
*/
void D_DrawSkyScans_Solid (void)
{
	do
	{
		pdest = (byte *) ( (byte *) d_viewbuffer + (screenwidth * pspan->v) + pspan->u);
		count	  = pspan->count >> SKY_SPAN_SHIFT;	// mankrip
		spancount = pspan->count %  SKY_SPAN_MAX;	// mankrip

		// calculate the initial s & t
		u = pspan->u;
		v = pspan->v;
		D_Sky_uv_To_st (u, v, &s, &t, &s2, &t2); // mankrip - smooth sky - edited

		while (count--) // mankrip
		{
			u += SKY_SPAN_MAX; // mankrip

			// calculate s and t at far end of span,
			// calculate s and t steps across span by shifting
			D_Sky_uv_To_st (u, v, &snext, &tnext, &snext2, &tnext2); // mankrip - smooth sky - edited

			sstep  = (snext  - s ) >> SKY_SPAN_SHIFT;
			tstep  = (tnext  - t ) >> SKY_SPAN_SHIFT;
			sstep2 = (snext2 - s2) >> SKY_SPAN_SHIFT; // mankrip - smooth sky
			tstep2 = (tnext2 - t2) >> SKY_SPAN_SHIFT; // mankrip - smooth sky

			// mankrip - begin
			pdest += SKY_SPAN_MAX;

			// from -SKY_SPAN_MAX to -1
			pdest[-32] = ( (over = skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)]) == TRANSPARENT_COLOR) ? skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] : over; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
			pdest[-31] = ( (over = skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)]) == TRANSPARENT_COLOR) ? skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] : over; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
			pdest[-30] = ( (over = skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)]) == TRANSPARENT_COLOR) ? skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] : over; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
			pdest[-29] = ( (over = skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)]) == TRANSPARENT_COLOR) ? skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] : over; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
			pdest[-28] = ( (over = skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)]) == TRANSPARENT_COLOR) ? skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] : over; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
			pdest[-27] = ( (over = skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)]) == TRANSPARENT_COLOR) ? skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] : over; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
			pdest[-26] = ( (over = skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)]) == TRANSPARENT_COLOR) ? skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] : over; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
			pdest[-25] = ( (over = skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)]) == TRANSPARENT_COLOR) ? skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] : over; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
			pdest[-24] = ( (over = skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)]) == TRANSPARENT_COLOR) ? skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] : over; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
			pdest[-23] = ( (over = skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)]) == TRANSPARENT_COLOR) ? skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] : over; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
			pdest[-22] = ( (over = skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)]) == TRANSPARENT_COLOR) ? skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] : over; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
			pdest[-21] = ( (over = skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)]) == TRANSPARENT_COLOR) ? skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] : over; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
			pdest[-20] = ( (over = skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)]) == TRANSPARENT_COLOR) ? skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] : over; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
			pdest[-19] = ( (over = skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)]) == TRANSPARENT_COLOR) ? skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] : over; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
			pdest[-18] = ( (over = skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)]) == TRANSPARENT_COLOR) ? skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] : over; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
			pdest[-17] = ( (over = skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)]) == TRANSPARENT_COLOR) ? skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] : over; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
			pdest[-16] = ( (over = skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)]) == TRANSPARENT_COLOR) ? skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] : over; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
			pdest[-15] = ( (over = skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)]) == TRANSPARENT_COLOR) ? skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] : over; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
			pdest[-14] = ( (over = skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)]) == TRANSPARENT_COLOR) ? skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] : over; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
			pdest[-13] = ( (over = skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)]) == TRANSPARENT_COLOR) ? skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] : over; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
			pdest[-12] = ( (over = skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)]) == TRANSPARENT_COLOR) ? skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] : over; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
			pdest[-11] = ( (over = skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)]) == TRANSPARENT_COLOR) ? skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] : over; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
			pdest[-10] = ( (over = skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)]) == TRANSPARENT_COLOR) ? skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] : over; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
			pdest[ -9] = ( (over = skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)]) == TRANSPARENT_COLOR) ? skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] : over; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
			pdest[ -8] = ( (over = skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)]) == TRANSPARENT_COLOR) ? skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] : over; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
			pdest[ -7] = ( (over = skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)]) == TRANSPARENT_COLOR) ? skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] : over; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
			pdest[ -6] = ( (over = skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)]) == TRANSPARENT_COLOR) ? skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] : over; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
			pdest[ -5] = ( (over = skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)]) == TRANSPARENT_COLOR) ? skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] : over; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
			pdest[ -4] = ( (over = skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)]) == TRANSPARENT_COLOR) ? skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] : over; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
			pdest[ -3] = ( (over = skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)]) == TRANSPARENT_COLOR) ? skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] : over; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
			pdest[ -2] = ( (over = skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)]) == TRANSPARENT_COLOR) ? skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] : over; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
			pdest[ -1] = ( (over = skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)]) == TRANSPARENT_COLOR) ? skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] : over;
			// mankrip - end

			s  = snext ;
			t  = tnext ;
			s2 = snext2; // mankrip - smooth sky
			t2 = tnext2; // mankrip - smooth sky
		}
		if (spancount) // mankrip
		{
			// calculate s and t at last pixel in span,
			// calculate s and t steps across span by division
			spancountminus1 = (float)(spancount - 1);

			if (spancountminus1 > 0)
			{
				u += spancountminus1;
				D_Sky_uv_To_st (u, v, &snext, &tnext, &snext2, &tnext2); // mankrip - smooth sky - edited

				sstep  = (snext  - s ) / spancountminus1;
				tstep  = (tnext  - t ) / spancountminus1;
				sstep2 = (snext2 - s2) / spancountminus1; // mankrip - smooth sky
				tstep2 = (tnext2 - t2) / spancountminus1; // mankrip - smooth sky
			}

			// mankrip - begin
			pdest += spancount;

			switch (spancount)
			{
				// from -SKY_SPAN_MAX to -1
				case 32: pdest[-32] = ( (over = skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)]) == TRANSPARENT_COLOR) ? skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] : over; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				case 31: pdest[-31] = ( (over = skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)]) == TRANSPARENT_COLOR) ? skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] : over; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				case 30: pdest[-30] = ( (over = skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)]) == TRANSPARENT_COLOR) ? skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] : over; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				case 29: pdest[-29] = ( (over = skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)]) == TRANSPARENT_COLOR) ? skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] : over; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				case 28: pdest[-28] = ( (over = skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)]) == TRANSPARENT_COLOR) ? skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] : over; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				case 27: pdest[-27] = ( (over = skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)]) == TRANSPARENT_COLOR) ? skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] : over; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				case 26: pdest[-26] = ( (over = skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)]) == TRANSPARENT_COLOR) ? skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] : over; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				case 25: pdest[-25] = ( (over = skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)]) == TRANSPARENT_COLOR) ? skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] : over; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				case 24: pdest[-24] = ( (over = skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)]) == TRANSPARENT_COLOR) ? skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] : over; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				case 23: pdest[-23] = ( (over = skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)]) == TRANSPARENT_COLOR) ? skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] : over; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				case 22: pdest[-22] = ( (over = skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)]) == TRANSPARENT_COLOR) ? skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] : over; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				case 21: pdest[-21] = ( (over = skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)]) == TRANSPARENT_COLOR) ? skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] : over; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				case 20: pdest[-20] = ( (over = skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)]) == TRANSPARENT_COLOR) ? skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] : over; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				case 19: pdest[-19] = ( (over = skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)]) == TRANSPARENT_COLOR) ? skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] : over; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				case 18: pdest[-18] = ( (over = skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)]) == TRANSPARENT_COLOR) ? skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] : over; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				case 17: pdest[-17] = ( (over = skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)]) == TRANSPARENT_COLOR) ? skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] : over; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				case 16: pdest[-16] = ( (over = skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)]) == TRANSPARENT_COLOR) ? skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] : over; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				case 15: pdest[-15] = ( (over = skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)]) == TRANSPARENT_COLOR) ? skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] : over; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				case 14: pdest[-14] = ( (over = skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)]) == TRANSPARENT_COLOR) ? skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] : over; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				case 13: pdest[-13] = ( (over = skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)]) == TRANSPARENT_COLOR) ? skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] : over; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				case 12: pdest[-12] = ( (over = skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)]) == TRANSPARENT_COLOR) ? skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] : over; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				case 11: pdest[-11] = ( (over = skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)]) == TRANSPARENT_COLOR) ? skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] : over; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				case 10: pdest[-10] = ( (over = skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)]) == TRANSPARENT_COLOR) ? skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] : over; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				case  9: pdest[ -9] = ( (over = skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)]) == TRANSPARENT_COLOR) ? skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] : over; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				case  8: pdest[ -8] = ( (over = skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)]) == TRANSPARENT_COLOR) ? skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] : over; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				case  7: pdest[ -7] = ( (over = skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)]) == TRANSPARENT_COLOR) ? skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] : over; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				case  6: pdest[ -6] = ( (over = skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)]) == TRANSPARENT_COLOR) ? skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] : over; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				case  5: pdest[ -5] = ( (over = skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)]) == TRANSPARENT_COLOR) ? skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] : over; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				case  4: pdest[ -4] = ( (over = skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)]) == TRANSPARENT_COLOR) ? skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] : over; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				case  3: pdest[ -3] = ( (over = skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)]) == TRANSPARENT_COLOR) ? skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] : over; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				case  2: pdest[ -2] = ( (over = skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)]) == TRANSPARENT_COLOR) ? skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] : over; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				case  1: pdest[ -1] = ( (over = skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)]) == TRANSPARENT_COLOR) ? skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] : over;
				break;
			}
			// mankrip - end
		}
	} while ( (pspan = pspan->pnext) != NULL);
}



void D_DrawSkyScans_Stipple (void)
{
	do
	{
		pdest = (byte *) ( (byte *) d_viewbuffer + (screenwidth * pspan->v) + pspan->u);

		// calculate the initial s & t
		u = pspan->u;
		v = pspan->v;
		D_Sky_uv_To_st (u, v, &s, &t, &s2, &t2); // mankrip - smooth sky - edited

		// mankrip - begin
		stipple = !((v + u) & 1); // stippled sky
		count	  = pspan->count >> SKY_SPAN_SHIFT;
		spancount = pspan->count %  SKY_SPAN_MAX;
		// mankrip - end

		while (count--) // mankrip
		{
			u += SKY_SPAN_MAX; // mankrip

			// calculate s and t at far end of span,
			// calculate s and t steps across span by shifting
			D_Sky_uv_To_st (u, v, &snext, &tnext, &snext2, &tnext2); // mankrip - smooth sky - edited

			sstep  = (snext  - s ) >> SKY_SPAN_SHIFT;
			tstep  = (tnext  - t ) >> SKY_SPAN_SHIFT;
			sstep2 = (snext2 - s2) >> SKY_SPAN_SHIFT; // mankrip - smooth sky
			tstep2 = (tnext2 - t2) >> SKY_SPAN_SHIFT; // mankrip - smooth sky

			// mankrip - begin
			pdest += SKY_SPAN_MAX;

			if (stipple)
			{
				// from -SKY_SPAN_MAX to -1
				pdest[-32] =																										   skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)]		; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				pdest[-31] = ( (over = skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)]) == TRANSPARENT_COLOR) ? skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] : over; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				pdest[-30] =																										   skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)]		; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				pdest[-29] = ( (over = skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)]) == TRANSPARENT_COLOR) ? skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] : over; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				pdest[-28] =																										   skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)]		; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				pdest[-27] = ( (over = skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)]) == TRANSPARENT_COLOR) ? skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] : over; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				pdest[-26] =																										   skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)]		; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				pdest[-25] = ( (over = skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)]) == TRANSPARENT_COLOR) ? skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] : over; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				pdest[-24] =																										   skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)]		; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				pdest[-23] = ( (over = skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)]) == TRANSPARENT_COLOR) ? skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] : over; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				pdest[-22] =																										   skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)]		; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				pdest[-21] = ( (over = skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)]) == TRANSPARENT_COLOR) ? skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] : over; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				pdest[-20] =																										   skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)]		; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				pdest[-19] = ( (over = skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)]) == TRANSPARENT_COLOR) ? skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] : over; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				pdest[-18] =																										   skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)]		; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				pdest[-17] = ( (over = skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)]) == TRANSPARENT_COLOR) ? skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] : over; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				pdest[-16] =																										   skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)]		; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				pdest[-15] = ( (over = skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)]) == TRANSPARENT_COLOR) ? skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] : over; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				pdest[-14] =																										   skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)]		; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				pdest[-13] = ( (over = skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)]) == TRANSPARENT_COLOR) ? skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] : over; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				pdest[-12] =																										   skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)]		; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				pdest[-11] = ( (over = skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)]) == TRANSPARENT_COLOR) ? skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] : over; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				pdest[-10] =																										   skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)]		; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				pdest[ -9] = ( (over = skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)]) == TRANSPARENT_COLOR) ? skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] : over; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				pdest[ -8] =																										   skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)]		; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				pdest[ -7] = ( (over = skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)]) == TRANSPARENT_COLOR) ? skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] : over; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				pdest[ -6] =																										   skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)]		; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				pdest[ -5] = ( (over = skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)]) == TRANSPARENT_COLOR) ? skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] : over; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				pdest[ -4] =																										   skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)]		; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				pdest[ -3] = ( (over = skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)]) == TRANSPARENT_COLOR) ? skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] : over; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				pdest[ -2] =																										   skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)]		; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				pdest[ -1] = ( (over = skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)]) == TRANSPARENT_COLOR) ? skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] : over;
			}
			else
			{
				// from -SKY_SPAN_MAX to -1
				pdest[-32] = ( (over = skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)]) == TRANSPARENT_COLOR) ? skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] : over; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				pdest[-31] =																										   skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)]		; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				pdest[-30] = ( (over = skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)]) == TRANSPARENT_COLOR) ? skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] : over; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				pdest[-29] =																										   skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)]		; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				pdest[-28] = ( (over = skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)]) == TRANSPARENT_COLOR) ? skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] : over; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				pdest[-27] =																										   skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)]		; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				pdest[-26] = ( (over = skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)]) == TRANSPARENT_COLOR) ? skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] : over; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				pdest[-25] =																										   skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)]		; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				pdest[-24] = ( (over = skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)]) == TRANSPARENT_COLOR) ? skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] : over; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				pdest[-23] =																										   skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)]		; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				pdest[-22] = ( (over = skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)]) == TRANSPARENT_COLOR) ? skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] : over; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				pdest[-21] =																										   skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)]		; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				pdest[-20] = ( (over = skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)]) == TRANSPARENT_COLOR) ? skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] : over; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				pdest[-19] =																										   skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)]		; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				pdest[-18] = ( (over = skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)]) == TRANSPARENT_COLOR) ? skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] : over; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				pdest[-17] =																										   skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)]		; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				pdest[-16] = ( (over = skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)]) == TRANSPARENT_COLOR) ? skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] : over; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				pdest[-15] =																										   skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)]		; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				pdest[-14] = ( (over = skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)]) == TRANSPARENT_COLOR) ? skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] : over; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				pdest[-13] =																										   skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)]		; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				pdest[-12] = ( (over = skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)]) == TRANSPARENT_COLOR) ? skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] : over; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				pdest[-11] =																										   skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)]		; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				pdest[-10] = ( (over = skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)]) == TRANSPARENT_COLOR) ? skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] : over; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				pdest[ -9] =																										   skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)]		; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				pdest[ -8] = ( (over = skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)]) == TRANSPARENT_COLOR) ? skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] : over; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				pdest[ -7] =																										   skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)]		; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				pdest[ -6] = ( (over = skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)]) == TRANSPARENT_COLOR) ? skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] : over; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				pdest[ -5] =																										   skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)]		; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				pdest[ -4] = ( (over = skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)]) == TRANSPARENT_COLOR) ? skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] : over; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				pdest[ -3] =																										   skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)]		; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				pdest[ -2] = ( (over = skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)]) == TRANSPARENT_COLOR) ? skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] : over; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				pdest[ -1] =																										   skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)]		;
			}
			// mankrip - end

			s  = snext ;
			t  = tnext ;
			s2 = snext2; // mankrip - smooth sky
			t2 = tnext2; // mankrip - smooth sky
		}
		if (spancount) // mankrip
		{
			// calculate s and t at last pixel in span,
			// calculate s and t steps across span by division
			spancountminus1 = (float)(spancount - 1);

			if (spancountminus1 > 0)
			{
				u += spancountminus1; // this also affects the stippling offset
				D_Sky_uv_To_st (u, v, &snext, &tnext, &snext2, &tnext2); // mankrip - smooth sky - edited

				sstep  = (snext  - s ) / spancountminus1;
				tstep  = (tnext  - t ) / spancountminus1;
				sstep2 = (snext2 - s2) / spancountminus1; // mankrip - smooth sky
				tstep2 = (tnext2 - t2) / spancountminus1; // mankrip - smooth sky
			}

			// mankrip - begin
			pdest += spancount;

			if ( (v + u) & 1) // stipple
				switch (spancount)
				{
					// from -SKY_SPAN_MAX to -1
					case 32: pdest[-32] =																											skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)]		 ; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
					case 31: pdest[-31] = ( (over = skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)]) == TRANSPARENT_COLOR) ? skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] : over; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
					case 30: pdest[-30] =																											skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)]		 ; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
					case 29: pdest[-29] = ( (over = skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)]) == TRANSPARENT_COLOR) ? skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] : over; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
					case 28: pdest[-28] =																											skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)]		 ; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
					case 27: pdest[-27] = ( (over = skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)]) == TRANSPARENT_COLOR) ? skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] : over; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
					case 26: pdest[-26] =																											skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)]		 ; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
					case 25: pdest[-25] = ( (over = skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)]) == TRANSPARENT_COLOR) ? skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] : over; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
					case 24: pdest[-24] =																											skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)]		 ; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
					case 23: pdest[-23] = ( (over = skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)]) == TRANSPARENT_COLOR) ? skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] : over; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
					case 22: pdest[-22] =																											skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)]		 ; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
					case 21: pdest[-21] = ( (over = skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)]) == TRANSPARENT_COLOR) ? skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] : over; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
					case 20: pdest[-20] =																											skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)]		 ; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
					case 19: pdest[-19] = ( (over = skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)]) == TRANSPARENT_COLOR) ? skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] : over; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
					case 18: pdest[-18] =																											skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)]		 ; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
					case 17: pdest[-17] = ( (over = skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)]) == TRANSPARENT_COLOR) ? skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] : over; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
					case 16: pdest[-16] =																											skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)]		 ; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
					case 15: pdest[-15] = ( (over = skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)]) == TRANSPARENT_COLOR) ? skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] : over; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
					case 14: pdest[-14] =																											skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)]		 ; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
					case 13: pdest[-13] = ( (over = skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)]) == TRANSPARENT_COLOR) ? skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] : over; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
					case 12: pdest[-12] =																											skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)]		 ; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
					case 11: pdest[-11] = ( (over = skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)]) == TRANSPARENT_COLOR) ? skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] : over; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
					case 10: pdest[-10] =																											skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)]		 ; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
					case  9: pdest[ -9] = ( (over = skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)]) == TRANSPARENT_COLOR) ? skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] : over; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
					case  8: pdest[ -8] =																											skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)]		 ; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
					case  7: pdest[ -7] = ( (over = skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)]) == TRANSPARENT_COLOR) ? skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] : over; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
					case  6: pdest[ -6] =																											skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)]		 ; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
					case  5: pdest[ -5] = ( (over = skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)]) == TRANSPARENT_COLOR) ? skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] : over; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
					case  4: pdest[ -4] =																											skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)]		 ; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
					case  3: pdest[ -3] = ( (over = skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)]) == TRANSPARENT_COLOR) ? skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] : over; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
					case  2: pdest[ -2] =																											skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)]		 ; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
					case  1: pdest[ -1] = ( (over = skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)]) == TRANSPARENT_COLOR) ? skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] : over;
					break;
				}
			else
				switch (spancount)
				{
					// from -SKY_SPAN_MAX to -1
					case 32: pdest[-32] = ( (over = skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)]) == TRANSPARENT_COLOR) ? skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] : over; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
					case 31: pdest[-31] =																											skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)]		 ; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
					case 30: pdest[-30] = ( (over = skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)]) == TRANSPARENT_COLOR) ? skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] : over; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
					case 29: pdest[-29] =																											skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)]		 ; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
					case 28: pdest[-28] = ( (over = skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)]) == TRANSPARENT_COLOR) ? skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] : over; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
					case 27: pdest[-27] =																											skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)]		 ; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
					case 26: pdest[-26] = ( (over = skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)]) == TRANSPARENT_COLOR) ? skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] : over; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
					case 25: pdest[-25] =																											skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)]		 ; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
					case 24: pdest[-24] = ( (over = skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)]) == TRANSPARENT_COLOR) ? skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] : over; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
					case 23: pdest[-23] =																											skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)]		 ; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
					case 22: pdest[-22] = ( (over = skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)]) == TRANSPARENT_COLOR) ? skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] : over; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
					case 21: pdest[-21] =																											skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)]		 ; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
					case 20: pdest[-20] = ( (over = skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)]) == TRANSPARENT_COLOR) ? skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] : over; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
					case 19: pdest[-19] =																											skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)]		 ; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
					case 18: pdest[-18] = ( (over = skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)]) == TRANSPARENT_COLOR) ? skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] : over; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
					case 17: pdest[-17] =																											skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)]		 ; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
					case 16: pdest[-16] = ( (over = skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)]) == TRANSPARENT_COLOR) ? skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] : over; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
					case 15: pdest[-15] =																											skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)]		 ; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
					case 14: pdest[-14] = ( (over = skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)]) == TRANSPARENT_COLOR) ? skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] : over; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
					case 13: pdest[-13] =																											skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)]		 ; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
					case 12: pdest[-12] = ( (over = skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)]) == TRANSPARENT_COLOR) ? skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] : over; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
					case 11: pdest[-11] =																											skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)]		 ; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
					case 10: pdest[-10] = ( (over = skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)]) == TRANSPARENT_COLOR) ? skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] : over; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
					case  9: pdest[ -9] =																											skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)]		 ; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
					case  8: pdest[ -8] = ( (over = skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)]) == TRANSPARENT_COLOR) ? skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] : over; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
					case  7: pdest[ -7] =																											skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)]		 ; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
					case  6: pdest[ -6] = ( (over = skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)]) == TRANSPARENT_COLOR) ? skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] : over; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
					case  5: pdest[ -5] =																											skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)]		 ; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
					case  4: pdest[ -4] = ( (over = skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)]) == TRANSPARENT_COLOR) ? skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] : over; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
					case  3: pdest[ -3] =																											skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)]		 ; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
					case  2: pdest[ -2] = ( (over = skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)]) == TRANSPARENT_COLOR) ? skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] : over; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
					case  1: pdest[ -1] =																											skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)]		 ;
					break;
				}
			// mankrip - end
		}
	} while ( (pspan = pspan->pnext) != NULL);
}



void D_DrawSkyScans_Transparent (void)
{
	do
	{
		pdest = (byte *) ( (byte *) d_viewbuffer + (screenwidth * pspan->v) + pspan->u);
		count	  = pspan->count >> SKY_SPAN_SHIFT;	// mankrip
		spancount = pspan->count %  SKY_SPAN_MAX;	// mankrip

		// calculate the initial s & t
		u = pspan->u;
		v = pspan->v;
		D_Sky_uv_To_st (u, v, &s, &t, &s2, &t2); // mankrip - smooth sky - edited

		while (count--) // mankrip
		{
			u += SKY_SPAN_MAX; // mankrip

			// calculate s and t at far end of span,
			// calculate s and t steps across span by shifting
			D_Sky_uv_To_st (u, v, &snext, &tnext, &snext2, &tnext2); // mankrip - smooth sky - edited

			sstep = (snext - s) >> SKY_SPAN_SHIFT;
			tstep = (tnext - t) >> SKY_SPAN_SHIFT;

			// mankrip - begin
			pdest += SKY_SPAN_MAX;

			// from -SKY_SPAN_MAX to -1
			pdest[-32] = skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)]; s += sstep; t += tstep;
			pdest[-31] = skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)]; s += sstep; t += tstep;
			pdest[-30] = skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)]; s += sstep; t += tstep;
			pdest[-29] = skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)]; s += sstep; t += tstep;
			pdest[-28] = skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)]; s += sstep; t += tstep;
			pdest[-27] = skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)]; s += sstep; t += tstep;
			pdest[-26] = skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)]; s += sstep; t += tstep;
			pdest[-25] = skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)]; s += sstep; t += tstep;
			pdest[-24] = skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)]; s += sstep; t += tstep;
			pdest[-23] = skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)]; s += sstep; t += tstep;
			pdest[-22] = skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)]; s += sstep; t += tstep;
			pdest[-21] = skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)]; s += sstep; t += tstep;
			pdest[-20] = skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)]; s += sstep; t += tstep;
			pdest[-19] = skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)]; s += sstep; t += tstep;
			pdest[-18] = skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)]; s += sstep; t += tstep;
			pdest[-17] = skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)]; s += sstep; t += tstep;
			pdest[-16] = skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)]; s += sstep; t += tstep;
			pdest[-15] = skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)]; s += sstep; t += tstep;
			pdest[-14] = skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)]; s += sstep; t += tstep;
			pdest[-13] = skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)]; s += sstep; t += tstep;
			pdest[-12] = skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)]; s += sstep; t += tstep;
			pdest[-11] = skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)]; s += sstep; t += tstep;
			pdest[-10] = skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)]; s += sstep; t += tstep;
			pdest[ -9] = skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)]; s += sstep; t += tstep;
			pdest[ -8] = skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)]; s += sstep; t += tstep;
			pdest[ -7] = skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)]; s += sstep; t += tstep;
			pdest[ -6] = skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)]; s += sstep; t += tstep;
			pdest[ -5] = skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)]; s += sstep; t += tstep;
			pdest[ -4] = skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)]; s += sstep; t += tstep;
			pdest[ -3] = skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)]; s += sstep; t += tstep;
			pdest[ -2] = skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)]; s += sstep; t += tstep;
			pdest[ -1] = skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)];
			// mankrip - end

			s = snext;
			t = tnext;
		}
		if (spancount) // mankrip
		{
			// calculate s and t at last pixel in span,
			// calculate s and t steps across span by division
			spancountminus1 = (float)(spancount - 1);

			if (spancountminus1 > 0)
			{
				u += spancountminus1;
				D_Sky_uv_To_st (u, v, &snext, &tnext, &snext2, &tnext2); // mankrip - smooth sky - edited

				sstep = (snext - s) / spancountminus1;
				tstep = (tnext - t) / spancountminus1;
			}

			// mankrip - begin
			pdest += spancount;

			switch (spancount)
			{
				// from -SKY_SPAN_MAX to -1
				case 32: pdest[-32] = skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)]; s += sstep; t += tstep;
				case 31: pdest[-31] = skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)]; s += sstep; t += tstep;
				case 30: pdest[-30] = skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)]; s += sstep; t += tstep;
				case 29: pdest[-29] = skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)]; s += sstep; t += tstep;
				case 28: pdest[-28] = skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)]; s += sstep; t += tstep;
				case 27: pdest[-27] = skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)]; s += sstep; t += tstep;
				case 26: pdest[-26] = skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)]; s += sstep; t += tstep;
				case 25: pdest[-25] = skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)]; s += sstep; t += tstep;
				case 24: pdest[-24] = skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)]; s += sstep; t += tstep;
				case 23: pdest[-23] = skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)]; s += sstep; t += tstep;
				case 22: pdest[-22] = skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)]; s += sstep; t += tstep;
				case 21: pdest[-21] = skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)]; s += sstep; t += tstep;
				case 20: pdest[-20] = skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)]; s += sstep; t += tstep;
				case 19: pdest[-19] = skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)]; s += sstep; t += tstep;
				case 18: pdest[-18] = skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)]; s += sstep; t += tstep;
				case 17: pdest[-17] = skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)]; s += sstep; t += tstep;
				case 16: pdest[-16] = skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)]; s += sstep; t += tstep;
				case 15: pdest[-15] = skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)]; s += sstep; t += tstep;
				case 14: pdest[-14] = skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)]; s += sstep; t += tstep;
				case 13: pdest[-13] = skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)]; s += sstep; t += tstep;
				case 12: pdest[-12] = skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)]; s += sstep; t += tstep;
				case 11: pdest[-11] = skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)]; s += sstep; t += tstep;
				case 10: pdest[-10] = skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)]; s += sstep; t += tstep;
				case  9: pdest[ -9] = skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)]; s += sstep; t += tstep;
				case  8: pdest[ -8] = skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)]; s += sstep; t += tstep;
				case  7: pdest[ -7] = skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)]; s += sstep; t += tstep;
				case  6: pdest[ -6] = skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)]; s += sstep; t += tstep;
				case  5: pdest[ -5] = skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)]; s += sstep; t += tstep;
				case  4: pdest[ -4] = skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)]; s += sstep; t += tstep;
				case  3: pdest[ -3] = skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)]; s += sstep; t += tstep;
				case  2: pdest[ -2] = skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)]; s += sstep; t += tstep;
				case  1: pdest[ -1] = skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)];
				break;
			}
			// mankrip - end
		}
	} while ( (pspan = pspan->pnext) != NULL);
}



void D_DrawSkyScans_Blend (void)
{
	colorblendingmap = colorblendingmode[minmaxclamp (0, (int) r_sky_blend_map.value, COLORBLENDING_MAPS - 1)];
	do
	{
		pdest = (byte *) ( (byte *) d_viewbuffer + (screenwidth * pspan->v) + pspan->u);
		count	  = pspan->count >> SKY_SPAN_SHIFT;	// mankrip
		spancount = pspan->count %  SKY_SPAN_MAX;	// mankrip

		// calculate the initial s & t
		u = pspan->u;
		v = pspan->v;
		D_Sky_uv_To_st (u, v, &s, &t, &s2, &t2); // mankrip - smooth sky - edited

		while (count--) // mankrip
		{
			u += SKY_SPAN_MAX; // mankrip

			// calculate s and t at far end of span,
			// calculate s and t steps across span by shifting
			D_Sky_uv_To_st (u, v, &snext, &tnext, &snext2, &tnext2); // mankrip - smooth sky - edited

			sstep  = (snext  - s ) >> SKY_SPAN_SHIFT;
			tstep  = (tnext  - t ) >> SKY_SPAN_SHIFT;
			sstep2 = (snext2 - s2) >> SKY_SPAN_SHIFT; // mankrip - smooth sky
			tstep2 = (tnext2 - t2) >> SKY_SPAN_SHIFT; // mankrip - smooth sky

			// mankrip - begin
			pdest += SKY_SPAN_MAX;

			// from -SKY_SPAN_MAX to -1
			pdest[-32] = colorblendingmap[skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)] + skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] * 256]; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
			pdest[-31] = colorblendingmap[skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)] + skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] * 256]; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
			pdest[-30] = colorblendingmap[skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)] + skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] * 256]; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
			pdest[-29] = colorblendingmap[skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)] + skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] * 256]; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
			pdest[-28] = colorblendingmap[skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)] + skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] * 256]; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
			pdest[-27] = colorblendingmap[skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)] + skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] * 256]; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
			pdest[-26] = colorblendingmap[skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)] + skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] * 256]; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
			pdest[-25] = colorblendingmap[skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)] + skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] * 256]; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
			pdest[-24] = colorblendingmap[skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)] + skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] * 256]; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
			pdest[-23] = colorblendingmap[skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)] + skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] * 256]; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
			pdest[-22] = colorblendingmap[skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)] + skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] * 256]; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
			pdest[-21] = colorblendingmap[skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)] + skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] * 256]; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
			pdest[-20] = colorblendingmap[skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)] + skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] * 256]; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
			pdest[-19] = colorblendingmap[skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)] + skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] * 256]; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
			pdest[-18] = colorblendingmap[skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)] + skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] * 256]; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
			pdest[-17] = colorblendingmap[skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)] + skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] * 256]; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
			pdest[-16] = colorblendingmap[skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)] + skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] * 256]; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
			pdest[-15] = colorblendingmap[skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)] + skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] * 256]; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
			pdest[-14] = colorblendingmap[skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)] + skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] * 256]; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
			pdest[-13] = colorblendingmap[skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)] + skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] * 256]; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
			pdest[-12] = colorblendingmap[skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)] + skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] * 256]; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
			pdest[-11] = colorblendingmap[skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)] + skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] * 256]; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
			pdest[-10] = colorblendingmap[skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)] + skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] * 256]; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
			pdest[ -9] = colorblendingmap[skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)] + skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] * 256]; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
			pdest[ -8] = colorblendingmap[skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)] + skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] * 256]; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
			pdest[ -7] = colorblendingmap[skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)] + skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] * 256]; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
			pdest[ -6] = colorblendingmap[skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)] + skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] * 256]; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
			pdest[ -5] = colorblendingmap[skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)] + skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] * 256]; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
			pdest[ -4] = colorblendingmap[skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)] + skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] * 256]; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
			pdest[ -3] = colorblendingmap[skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)] + skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] * 256]; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
			pdest[ -2] = colorblendingmap[skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)] + skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] * 256]; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
			pdest[ -1] = colorblendingmap[skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)] + skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] * 256];
			// mankrip - end

			s  = snext ;
			t  = tnext ;
			s2 = snext2; // mankrip - smooth sky
			t2 = tnext2; // mankrip - smooth sky
		}
		if (spancount) // mankrip
		{
			// calculate s and t at last pixel in span,
			// calculate s and t steps across span by division
			spancountminus1 = (float)(spancount - 1);

			if (spancountminus1 > 0)
			{
				u += spancountminus1;
				D_Sky_uv_To_st (u, v, &snext, &tnext, &snext2, &tnext2); // mankrip - smooth sky - edited

				sstep  = (snext  - s ) / spancountminus1;
				tstep  = (tnext  - t ) / spancountminus1;
				sstep2 = (snext2 - s2) / spancountminus1; // mankrip - smooth sky
				tstep2 = (tnext2 - t2) / spancountminus1; // mankrip - smooth sky
			}

			// mankrip - begin
			pdest += spancount;

			switch (spancount)
			{
				// from -SKY_SPAN_MAX to -1
				case 32: pdest[-32] = colorblendingmap[skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)] + skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] * 256]; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				case 31: pdest[-31] = colorblendingmap[skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)] + skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] * 256]; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				case 30: pdest[-30] = colorblendingmap[skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)] + skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] * 256]; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				case 29: pdest[-29] = colorblendingmap[skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)] + skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] * 256]; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				case 28: pdest[-28] = colorblendingmap[skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)] + skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] * 256]; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				case 27: pdest[-27] = colorblendingmap[skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)] + skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] * 256]; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				case 26: pdest[-26] = colorblendingmap[skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)] + skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] * 256]; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				case 25: pdest[-25] = colorblendingmap[skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)] + skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] * 256]; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				case 24: pdest[-24] = colorblendingmap[skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)] + skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] * 256]; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				case 23: pdest[-23] = colorblendingmap[skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)] + skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] * 256]; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				case 22: pdest[-22] = colorblendingmap[skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)] + skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] * 256]; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				case 21: pdest[-21] = colorblendingmap[skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)] + skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] * 256]; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				case 20: pdest[-20] = colorblendingmap[skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)] + skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] * 256]; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				case 19: pdest[-19] = colorblendingmap[skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)] + skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] * 256]; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				case 18: pdest[-18] = colorblendingmap[skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)] + skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] * 256]; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				case 17: pdest[-17] = colorblendingmap[skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)] + skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] * 256]; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				case 16: pdest[-16] = colorblendingmap[skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)] + skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] * 256]; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				case 15: pdest[-15] = colorblendingmap[skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)] + skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] * 256]; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				case 14: pdest[-14] = colorblendingmap[skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)] + skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] * 256]; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				case 13: pdest[-13] = colorblendingmap[skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)] + skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] * 256]; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				case 12: pdest[-12] = colorblendingmap[skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)] + skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] * 256]; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				case 11: pdest[-11] = colorblendingmap[skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)] + skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] * 256]; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				case 10: pdest[-10] = colorblendingmap[skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)] + skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] * 256]; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				case  9: pdest[ -9] = colorblendingmap[skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)] + skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] * 256]; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				case  8: pdest[ -8] = colorblendingmap[skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)] + skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] * 256]; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				case  7: pdest[ -7] = colorblendingmap[skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)] + skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] * 256]; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				case  6: pdest[ -6] = colorblendingmap[skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)] + skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] * 256]; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				case  5: pdest[ -5] = colorblendingmap[skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)] + skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] * 256]; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				case  4: pdest[ -4] = colorblendingmap[skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)] + skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] * 256]; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				case  3: pdest[ -3] = colorblendingmap[skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)] + skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] * 256]; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				case  2: pdest[ -2] = colorblendingmap[skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)] + skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] * 256]; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				case  1: pdest[ -1] = colorblendingmap[skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)] + skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] * 256];
				break;
			}
			// mankrip - end
		}
	} while ( (pspan = pspan->pnext) != NULL);
}



void D_DrawSkyScans_BlendBackwards (void)
{
	colorblendingmap = colorblendingmode[minmaxclamp (0, (int) r_sky_blend_map.value, COLORBLENDING_MAPS - 1)];
	do
	{
		pdest = (byte *) ( (byte *) d_viewbuffer + (screenwidth * pspan->v) + pspan->u);
		count	  = pspan->count >> SKY_SPAN_SHIFT;	// mankrip
		spancount = pspan->count %  SKY_SPAN_MAX;	// mankrip

		// calculate the initial s & t
		u = pspan->u;
		v = pspan->v;
		D_Sky_uv_To_st (u, v, &s, &t, &s2, &t2); // mankrip - smooth sky - edited

		while (count--) // mankrip
		{
			u += SKY_SPAN_MAX; // mankrip

			// calculate s and t at far end of span,
			// calculate s and t steps across span by shifting
			D_Sky_uv_To_st (u, v, &snext, &tnext, &snext2, &tnext2); // mankrip - smooth sky - edited

			sstep  = (snext  - s ) >> SKY_SPAN_SHIFT;
			tstep  = (tnext  - t ) >> SKY_SPAN_SHIFT;
			sstep2 = (snext2 - s2) >> SKY_SPAN_SHIFT; // mankrip - smooth sky
			tstep2 = (tnext2 - t2) >> SKY_SPAN_SHIFT; // mankrip - smooth sky

			// mankrip - begin
			pdest += SKY_SPAN_MAX;

			// from -SKY_SPAN_MAX to -1
			pdest[-32] = colorblendingmap[skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] + skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)] * 256]; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
			pdest[-31] = colorblendingmap[skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] + skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)] * 256]; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
			pdest[-30] = colorblendingmap[skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] + skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)] * 256]; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
			pdest[-29] = colorblendingmap[skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] + skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)] * 256]; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
			pdest[-28] = colorblendingmap[skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] + skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)] * 256]; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
			pdest[-27] = colorblendingmap[skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] + skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)] * 256]; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
			pdest[-26] = colorblendingmap[skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] + skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)] * 256]; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
			pdest[-25] = colorblendingmap[skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] + skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)] * 256]; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
			pdest[-24] = colorblendingmap[skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] + skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)] * 256]; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
			pdest[-23] = colorblendingmap[skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] + skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)] * 256]; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
			pdest[-22] = colorblendingmap[skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] + skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)] * 256]; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
			pdest[-21] = colorblendingmap[skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] + skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)] * 256]; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
			pdest[-20] = colorblendingmap[skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] + skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)] * 256]; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
			pdest[-19] = colorblendingmap[skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] + skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)] * 256]; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
			pdest[-18] = colorblendingmap[skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] + skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)] * 256]; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
			pdest[-17] = colorblendingmap[skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] + skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)] * 256]; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
			pdest[-16] = colorblendingmap[skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] + skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)] * 256]; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
			pdest[-15] = colorblendingmap[skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] + skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)] * 256]; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
			pdest[-14] = colorblendingmap[skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] + skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)] * 256]; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
			pdest[-13] = colorblendingmap[skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] + skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)] * 256]; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
			pdest[-12] = colorblendingmap[skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] + skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)] * 256]; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
			pdest[-11] = colorblendingmap[skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] + skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)] * 256]; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
			pdest[-10] = colorblendingmap[skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] + skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)] * 256]; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
			pdest[ -9] = colorblendingmap[skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] + skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)] * 256]; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
			pdest[ -8] = colorblendingmap[skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] + skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)] * 256]; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
			pdest[ -7] = colorblendingmap[skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] + skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)] * 256]; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
			pdest[ -6] = colorblendingmap[skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] + skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)] * 256]; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
			pdest[ -5] = colorblendingmap[skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] + skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)] * 256]; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
			pdest[ -4] = colorblendingmap[skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] + skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)] * 256]; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
			pdest[ -3] = colorblendingmap[skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] + skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)] * 256]; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
			pdest[ -2] = colorblendingmap[skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] + skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)] * 256]; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
			pdest[ -1] = colorblendingmap[skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] + skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)] * 256];
			// mankrip - end

			s  = snext ;
			t  = tnext ;
			s2 = snext2; // mankrip - smooth sky
			t2 = tnext2; // mankrip - smooth sky
		}
		if (spancount) // mankrip
		{
			// calculate s and t at last pixel in span,
			// calculate s and t steps across span by division
			spancountminus1 = (float)(spancount - 1);

			if (spancountminus1 > 0)
			{
				u += spancountminus1;
				D_Sky_uv_To_st (u, v, &snext, &tnext, &snext2, &tnext2); // mankrip - smooth sky - edited

				sstep  = (snext  - s ) / spancountminus1;
				tstep  = (tnext  - t ) / spancountminus1;
				sstep2 = (snext2 - s2) / spancountminus1; // mankrip - smooth sky
				tstep2 = (tnext2 - t2) / spancountminus1; // mankrip - smooth sky
			}

			// mankrip - begin
			pdest += spancount;

			switch (spancount)
			{
				// from -SKY_SPAN_MAX to -1
				case 32: pdest[-32] = colorblendingmap[skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] + skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)] * 256]; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				case 31: pdest[-31] = colorblendingmap[skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] + skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)] * 256]; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				case 30: pdest[-30] = colorblendingmap[skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] + skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)] * 256]; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				case 29: pdest[-29] = colorblendingmap[skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] + skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)] * 256]; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				case 28: pdest[-28] = colorblendingmap[skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] + skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)] * 256]; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				case 27: pdest[-27] = colorblendingmap[skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] + skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)] * 256]; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				case 26: pdest[-26] = colorblendingmap[skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] + skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)] * 256]; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				case 25: pdest[-25] = colorblendingmap[skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] + skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)] * 256]; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				case 24: pdest[-24] = colorblendingmap[skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] + skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)] * 256]; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				case 23: pdest[-23] = colorblendingmap[skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] + skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)] * 256]; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				case 22: pdest[-22] = colorblendingmap[skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] + skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)] * 256]; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				case 21: pdest[-21] = colorblendingmap[skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] + skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)] * 256]; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				case 20: pdest[-20] = colorblendingmap[skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] + skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)] * 256]; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				case 19: pdest[-19] = colorblendingmap[skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] + skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)] * 256]; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				case 18: pdest[-18] = colorblendingmap[skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] + skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)] * 256]; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				case 17: pdest[-17] = colorblendingmap[skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] + skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)] * 256]; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				case 16: pdest[-16] = colorblendingmap[skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] + skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)] * 256]; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				case 15: pdest[-15] = colorblendingmap[skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] + skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)] * 256]; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				case 14: pdest[-14] = colorblendingmap[skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] + skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)] * 256]; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				case 13: pdest[-13] = colorblendingmap[skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] + skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)] * 256]; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				case 12: pdest[-12] = colorblendingmap[skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] + skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)] * 256]; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				case 11: pdest[-11] = colorblendingmap[skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] + skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)] * 256]; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				case 10: pdest[-10] = colorblendingmap[skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] + skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)] * 256]; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				case  9: pdest[ -9] = colorblendingmap[skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] + skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)] * 256]; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				case  8: pdest[ -8] = colorblendingmap[skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] + skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)] * 256]; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				case  7: pdest[ -7] = colorblendingmap[skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] + skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)] * 256]; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				case  6: pdest[ -6] = colorblendingmap[skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] + skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)] * 256]; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				case  5: pdest[ -5] = colorblendingmap[skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] + skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)] * 256]; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				case  4: pdest[ -4] = colorblendingmap[skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] + skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)] * 256]; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				case  3: pdest[ -3] = colorblendingmap[skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] + skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)] * 256]; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				case  2: pdest[ -2] = colorblendingmap[skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] + skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)] * 256]; s += sstep; t += tstep; s2 += sstep2; t2 += tstep2;
				case  1: pdest[ -1] = colorblendingmap[skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)] + skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)] * 256];
				break;
			}
			// mankrip - end
		}
	} while ( (pspan = pspan->pnext) != NULL);
}


#if 0
// mankrip - begin
#define DRAW_BACKGROUND	0
#define DRAW_FOREGROUND	1<<0
#define DRAW_STIPPLED	1<<1
#define DRAW_BLENDED	1<<2
#define DRAW_BACKWARDS	1<<3
// mankrip - end
void D_DrawSkyScans_Tinted (int mode) // mankrip - only used for the menu background, so fully unoptimized
{
	byte under; // mankrip
	colorblendingmap = colorblendingmode[minmaxclamp (0, (int) r_sky_blend_map.value, COLORBLENDING_MAPS - 1)]; // mankrip - blended sky
	do
	{
		pdest = (byte *) ( (byte *) d_viewbuffer + (screenwidth * pspan->v) + pspan->u);
		// mankrip - stippled sky - begin
		stipple = (mode & DRAW_FOREGROUND)
		?	( (mode & DRAW_STIPPLED)
			?	( ( ( (long) pdest - ( (long) pdest - (long) d_viewbuffer) / (long) screenwidth)) & 1)
			:	false
			)
		:	true
			;
		// mankrip - stippled sky - end
		count = pspan->count;

		// calculate the initial s & t
		u = pspan->u;
		v = pspan->v;
		D_Sky_uv_To_st (u, v, &s, &t, &s2, &t2); // mankrip - smooth sky - edited

		do
		{
			spancount = (count >= SKY_SPAN_MAX) ? SKY_SPAN_MAX : count; // mankrip

			count -= spancount;

			if (count)
			{
				u += spancount;

			// calculate s and t at far end of span,
			// calculate s and t steps across span by shifting
				D_Sky_uv_To_st (u, v, &snext, &tnext, &snext2, &tnext2); // mankrip - smooth sky - edited

				sstep  = (snext  - s ) >> SKY_SPAN_SHIFT;
				tstep  = (tnext  - t ) >> SKY_SPAN_SHIFT;
				sstep2 = (snext2 - s2) >> SKY_SPAN_SHIFT; // mankrip - smooth sky
				tstep2 = (tnext2 - t2) >> SKY_SPAN_SHIFT; // mankrip - smooth sky
			}
			else
			{
			// calculate s and t at last pixel in span,
			// calculate s and t steps across span by division
				spancountminus1 = (float)(spancount - 1);

				if (spancountminus1 > 0)
				{
					u += spancountminus1;
					D_Sky_uv_To_st (u, v, &snext, &tnext, &snext2, &tnext2); // mankrip - smooth sky - edited

					sstep  = (snext  - s ) / spancountminus1;
					tstep  = (tnext  - t ) / spancountminus1;
					sstep2 = (snext2 - s2) / spancountminus1; // mankrip - smooth sky
					tstep2 = (tnext2 - t2) / spancountminus1; // mankrip - smooth sky
				}
			}

			// mankrip - begin
			do
			{
				if (mode & DRAW_STIPPLED)
					stipple = !stipple;
				over  = tintTable[skyoverlay [ ( (t2 & R_SKY_TMASK) >> 8) + ( (s2 & R_SKY_SMASK) >> 16)]];
				under = tintTable[skyunderlay[ ( (t  & R_SKY_TMASK) >> 8) + ( (s  & R_SKY_SMASK) >> 16)]];
				*pdest++ = (stipple)
				?	under
				:	( (mode & DRAW_BLENDED)
					?	colorblendingmap[ (mode & DRAW_BACKWARDS)
						?	(under + over * 256)
						:	(over + under * 256)
						]
					:	( (over == TRANSPARENT_COLOR) ? under : over)
					)
					;
				s  += sstep ;
				t  += tstep ;
				s2 += sstep2; // smooth sky
				t2 += tstep2; // smooth sky
			} while (--spancount > 0);
			// mankrip - end

			s  = snext ;
			t  = tnext ;
			s2 = snext2; // mankrip - smooth sky
			t2 = tnext2; // mankrip - smooth sky
		} while (count > 0);
	} while ( (pspan = pspan->pnext) != NULL);
}



void D_DrawSkyScans_Tinted_Transparent		(void) { D_DrawSkyScans_Tinted (DRAW_BACKGROUND									); }
void D_DrawSkyScans_Tinted_Solid			(void) { D_DrawSkyScans_Tinted (DRAW_FOREGROUND									); }
void D_DrawSkyScans_Tinted_Stipple			(void) { D_DrawSkyScans_Tinted (DRAW_FOREGROUND | DRAW_STIPPLED					); }
void D_DrawSkyScans_Tinted_Blend			(void) { D_DrawSkyScans_Tinted (DRAW_FOREGROUND | DRAW_BLENDED					); }
void D_DrawSkyScans_Tinted_BlendBackwards	(void) { D_DrawSkyScans_Tinted (DRAW_FOREGROUND | DRAW_BLENDED | DRAW_BACKWARDS	); }



static void (*D_DrawSkyScans_For[2][2][COLORBLENDING_MAPS][4]) (void) = {
	{
		{
			{	//BLEND_WEIGHTEDMEAN
				D_DrawSkyScans_Transparent
			,	D_DrawSkyScans_Stipple
			,	D_DrawSkyScans_Stipple
			,	D_DrawSkyScans_Solid
			}
		,	{	//BLEND_ADDITIVE
				D_DrawSkyScans_Transparent
			,	D_DrawSkyScans_Stipple
			,	D_DrawSkyScans_Stipple
			,	D_DrawSkyScans_Solid
			}
		,	{	//BLEND_SMOKE
				D_DrawSkyScans_Transparent
			,	D_DrawSkyScans_Stipple
			,	D_DrawSkyScans_Stipple
			,	D_DrawSkyScans_Solid
			}
		}
	,	{
			{	//BLEND_WEIGHTEDMEAN
				D_DrawSkyScans_Transparent
			,	D_DrawSkyScans_Blend
			,	D_DrawSkyScans_BlendBackwards
			,	D_DrawSkyScans_Solid
			}
		,	{	//BLEND_ADDITIVE
				D_DrawSkyScans_Transparent
			,	D_DrawSkyScans_Blend
			,	D_DrawSkyScans_BlendBackwards
			,	D_DrawSkyScans_BlendBackwards
			}
		,	{	//BLEND_SMOKE
				D_DrawSkyScans_Transparent
			,	D_DrawSkyScans_Blend
			,	D_DrawSkyScans_BlendBackwards
			,	D_DrawSkyScans_Solid
			}
		}
	}
,	{	// tinted variants
		{
			{	//BLEND_WEIGHTEDMEAN
				D_DrawSkyScans_Tinted_Transparent
			,	D_DrawSkyScans_Tinted_Stipple
			,	D_DrawSkyScans_Tinted_Stipple
			,	D_DrawSkyScans_Tinted_Solid
			}
		,	{	//BLEND_ADDITIVE
				D_DrawSkyScans_Tinted_Transparent
			,	D_DrawSkyScans_Tinted_Stipple
			,	D_DrawSkyScans_Tinted_Stipple
			,	D_DrawSkyScans_Tinted_Solid
			}
		,	{	//BLEND_SMOKE
				D_DrawSkyScans_Tinted_Transparent
			,	D_DrawSkyScans_Tinted_Stipple
			,	D_DrawSkyScans_Tinted_Stipple
			,	D_DrawSkyScans_Tinted_Solid
			}
		}
	,	{
			{	//BLEND_WEIGHTEDMEAN
				D_DrawSkyScans_Tinted_Transparent
			,	D_DrawSkyScans_Tinted_Blend
			,	D_DrawSkyScans_Tinted_BlendBackwards
			,	D_DrawSkyScans_Tinted_Solid
			}
		,	{	//BLEND_ADDITIVE
				D_DrawSkyScans_Tinted_Transparent
			,	D_DrawSkyScans_Tinted_Blend
			,	D_DrawSkyScans_Tinted_BlendBackwards
			,	D_DrawSkyScans_Tinted_BlendBackwards
			}
		,	{	//BLEND_SMOKE
				D_DrawSkyScans_Tinted_Transparent
			,	D_DrawSkyScans_Tinted_Blend
			,	D_DrawSkyScans_Tinted_BlendBackwards
			,	D_DrawSkyScans_Tinted_Solid
			}
		}
	}
};
#else
static void (*D_DrawSkyScans_For[2][COLORBLENDING_MAPS][4]) (void) = {
	{
		{	//BLEND_WEIGHTEDMEAN
			D_DrawSkyScans_Transparent
		,	D_DrawSkyScans_Stipple
		,	D_DrawSkyScans_Stipple
		,	D_DrawSkyScans_Solid
		}
	,	{	//BLEND_ADDITIVE
			D_DrawSkyScans_Transparent
		,	D_DrawSkyScans_Stipple
		,	D_DrawSkyScans_Stipple
		,	D_DrawSkyScans_Solid
		}
	,	{	//BLEND_SMOKE
			D_DrawSkyScans_Transparent
		,	D_DrawSkyScans_Stipple
		,	D_DrawSkyScans_Stipple
		,	D_DrawSkyScans_Solid
		}
	}
,	{
		{	//BLEND_WEIGHTEDMEAN
			D_DrawSkyScans_Transparent
		,	D_DrawSkyScans_Blend
		,	D_DrawSkyScans_BlendBackwards
		,	D_DrawSkyScans_Solid
		}
	,	{	//BLEND_ADDITIVE
			D_DrawSkyScans_Transparent
		,	D_DrawSkyScans_Blend
		,	D_DrawSkyScans_BlendBackwards
		,	D_DrawSkyScans_BlendBackwards
		}
	,	{	//BLEND_SMOKE
			D_DrawSkyScans_Transparent
		,	D_DrawSkyScans_Blend
		,	D_DrawSkyScans_BlendBackwards
		,	D_DrawSkyScans_Solid
		}
	}
};
#endif



void D_DrawSkyScans (espan_t *pspans)
{
	// mankrip - begin
	#if 0
	extern int
		m_state
		;
	#endif
	pspan = pspans;
	timespeed1 = skytime * skyspeed * r_skyspeed1.value;
	timespeed2 = skytime * skyspeed * r_skyspeed2.value;
	// keep compiler happy - begin
	sstep  = 0;
	tstep  = 0;
	sstep2 = 0;
	tstep2 = 0;
	// keep compiler happy - end

	#if 0
	D_DrawSkyScans_For[ (m_state != 0 && key_dest == key_menu)][minmaxclamp (0, (int) r_sky_blend_mode.value, 1)][minmaxclamp (0, (int) r_sky_blend_map.value, COLORBLENDING_MAPS - 1)][minmaxclamp (0, (int) (r_sky_blend_alpha.value * 3.0f), 3)] ();
	#else
	D_DrawSkyScans_For[minmaxclamp (0, (int) r_sky_blend_mode.value, 1)][minmaxclamp (0, (int) r_sky_blend_map.value, COLORBLENDING_MAPS - 1)][minmaxclamp (0, (int) (r_sky_blend_alpha.value * 3.0f), 3)] ();
	#endif
	// mankrip - end

}
