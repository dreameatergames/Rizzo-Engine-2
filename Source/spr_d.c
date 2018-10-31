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
// d_sprite.c: software top-level rasterization driver module for drawing
// sprites

#include "quakedef.h"
#include "r_local.h" // mankrip
#include "d_local.h"
#include "spr_file.h" // mankrip

static int
	sprite_height
,	minindex
,	maxindex
	;
static sspan_t
	* sprite_spans
	;



/*
=====================
D_SpriteDrawSpans
=====================
*/
void D_SpriteDrawSpans16_ColorKeyed (void)
{
	sspan_t
		* pspan = sprite_spans
		;
	int			count, spancount, izistep;
	byte		*pbase, pcolor, *pdest;
	fixed16_t	s, t, snext, tnext, sstep, tstep;
	float		sdivz, tdivz, zi, z, du, dv, spancountminus1;
	float		sdivzstepu, tdivzstepu, zistepu;
	int			izi;
	short		*pz;

	sstep = 0;	// keep compiler happy
	tstep = 0;	// ditto

	pbase = cacheblock;

	sdivzstepu = d_sdivzstepu * 16;
	tdivzstepu = d_tdivzstepu * 16;
	zistepu = d_zistepu * 16;

// we count on FP exceptions being turned off to avoid range problems
	izistep = (int)(d_zistepu * 0x8000 * 0x10000);

	do
	{
		pdest = (byte *)d_viewbuffer + (screenwidth * pspan->v) + pspan->u;
		pz = d_pzbuffer + (d_zwidth * pspan->v) + pspan->u;

		count = pspan->count >> 4; // mh
		spancount = pspan->count % 16; // mankrip

		// calculate the initial s/z, t/z, 1/z, s, and t and clamp
		du = (float)pspan->u;
		dv = (float)pspan->v;

		sdivz = d_sdivzorigin + dv*d_sdivzstepv + du*d_sdivzstepu;
		tdivz = d_tdivzorigin + dv*d_tdivzstepv + du*d_tdivzstepu;
		zi = d_ziorigin + dv*d_zistepv + du*d_zistepu;
		z = (float)0x10000 / zi;	// prescale to 16.16 fixed-point
		// we count on FP exceptions being turned off to avoid range problems
		izi = (int) (zi * 0x8000 * 0x10000);

		s = (int)(sdivz * z) + sadjust;
		if (s > bbextents)
			s = bbextents;
		else if (s < 0)
			s = 0;

		t = (int)(tdivz * z) + tadjust;
		if (t > bbextentt)
			t = bbextentt;
		else if (t < 0)
			t = 0;

		while (count--) // mankrip
		{
			// calculate s/z, t/z, zi->fixed s and t at far end of span,
			// calculate s and t steps across span by shifting
			// qbism ( http://forums.inside3d.com/viewtopic.php?t=2717 ) - begin
			sdivz += sdivzstepu;
			tdivz += tdivzstepu;
			zi += zistepu;
			// qbism ( http://forums.inside3d.com/viewtopic.php?t=2717 ) - end
			z = (float)0x10000 / zi;	// prescale to 16.16 fixed-point

			snext = (int)(sdivz * z) + sadjust;
			if (snext > bbextents)
				snext = bbextents;
			// qbism ( http://forums.inside3d.com/viewtopic.php?t=2717 ) - begin
			else if (snext < 16)
				snext = 16;   // prevent round-off error on <0 steps from causing overstepping & running off the edge of the texture
			// qbism ( http://forums.inside3d.com/viewtopic.php?t=2717 ) - end

			tnext = (int)(tdivz * z) + tadjust;
			if (tnext > bbextentt)
				tnext = bbextentt;
			// qbism ( http://forums.inside3d.com/viewtopic.php?t=2717 ) - begin
			else if (tnext < 16)
				tnext = 16;   // guard against round-off error on <0 steps

			sstep = (snext - s) >> 4;
			tstep = (tnext - t) >> 4;
			// qbism ( http://forums.inside3d.com/viewtopic.php?t=2717 ) - end

			// mankrip - begin
			pdest += 16;
			pz += 16;
			if (pz[-16] <= (izi >> 16)) if ( (pcolor = *(pbase + (s >> 16) + (t >> 16) * cachewidth)) != TRANSPARENT_COLOR) { pdest[-16] = translationTable[pcolor]; pz[-16] = izi >> 16; } izi += izistep; s += sstep; t += tstep;
			if (pz[-15] <= (izi >> 16)) if ( (pcolor = *(pbase + (s >> 16) + (t >> 16) * cachewidth)) != TRANSPARENT_COLOR) { pdest[-15] = translationTable[pcolor]; pz[-15] = izi >> 16; } izi += izistep; s += sstep; t += tstep;
			if (pz[-14] <= (izi >> 16)) if ( (pcolor = *(pbase + (s >> 16) + (t >> 16) * cachewidth)) != TRANSPARENT_COLOR) { pdest[-14] = translationTable[pcolor]; pz[-14] = izi >> 16; } izi += izistep; s += sstep; t += tstep;
			if (pz[-13] <= (izi >> 16)) if ( (pcolor = *(pbase + (s >> 16) + (t >> 16) * cachewidth)) != TRANSPARENT_COLOR) { pdest[-13] = translationTable[pcolor]; pz[-13] = izi >> 16; } izi += izistep; s += sstep; t += tstep;
			if (pz[-12] <= (izi >> 16)) if ( (pcolor = *(pbase + (s >> 16) + (t >> 16) * cachewidth)) != TRANSPARENT_COLOR) { pdest[-12] = translationTable[pcolor]; pz[-12] = izi >> 16; } izi += izistep; s += sstep; t += tstep;
			if (pz[-11] <= (izi >> 16)) if ( (pcolor = *(pbase + (s >> 16) + (t >> 16) * cachewidth)) != TRANSPARENT_COLOR) { pdest[-11] = translationTable[pcolor]; pz[-11] = izi >> 16; } izi += izistep; s += sstep; t += tstep;
			if (pz[-10] <= (izi >> 16)) if ( (pcolor = *(pbase + (s >> 16) + (t >> 16) * cachewidth)) != TRANSPARENT_COLOR) { pdest[-10] = translationTable[pcolor]; pz[-10] = izi >> 16; } izi += izistep; s += sstep; t += tstep;
			if (pz[ -9] <= (izi >> 16)) if ( (pcolor = *(pbase + (s >> 16) + (t >> 16) * cachewidth)) != TRANSPARENT_COLOR) { pdest[ -9] = translationTable[pcolor]; pz[ -9] = izi >> 16; } izi += izistep; s += sstep; t += tstep;
			if (pz[ -8] <= (izi >> 16)) if ( (pcolor = *(pbase + (s >> 16) + (t >> 16) * cachewidth)) != TRANSPARENT_COLOR) { pdest[ -8] = translationTable[pcolor]; pz[ -8] = izi >> 16; } izi += izistep; s += sstep; t += tstep;
			if (pz[ -7] <= (izi >> 16)) if ( (pcolor = *(pbase + (s >> 16) + (t >> 16) * cachewidth)) != TRANSPARENT_COLOR) { pdest[ -7] = translationTable[pcolor]; pz[ -7] = izi >> 16; } izi += izistep; s += sstep; t += tstep;
			if (pz[ -6] <= (izi >> 16)) if ( (pcolor = *(pbase + (s >> 16) + (t >> 16) * cachewidth)) != TRANSPARENT_COLOR) { pdest[ -6] = translationTable[pcolor]; pz[ -6] = izi >> 16; } izi += izistep; s += sstep; t += tstep;
			if (pz[ -5] <= (izi >> 16)) if ( (pcolor = *(pbase + (s >> 16) + (t >> 16) * cachewidth)) != TRANSPARENT_COLOR) { pdest[ -5] = translationTable[pcolor]; pz[ -5] = izi >> 16; } izi += izistep; s += sstep; t += tstep;
			if (pz[ -4] <= (izi >> 16)) if ( (pcolor = *(pbase + (s >> 16) + (t >> 16) * cachewidth)) != TRANSPARENT_COLOR) { pdest[ -4] = translationTable[pcolor]; pz[ -4] = izi >> 16; } izi += izistep; s += sstep; t += tstep;
			if (pz[ -3] <= (izi >> 16)) if ( (pcolor = *(pbase + (s >> 16) + (t >> 16) * cachewidth)) != TRANSPARENT_COLOR) { pdest[ -3] = translationTable[pcolor]; pz[ -3] = izi >> 16; } izi += izistep; s += sstep; t += tstep;
			if (pz[ -2] <= (izi >> 16)) if ( (pcolor = *(pbase + (s >> 16) + (t >> 16) * cachewidth)) != TRANSPARENT_COLOR) { pdest[ -2] = translationTable[pcolor]; pz[ -2] = izi >> 16; } izi += izistep; s += sstep; t += tstep;
			if (pz[ -1] <= (izi >> 16)) if ( (pcolor = *(pbase + (s >> 16) + (t >> 16) * cachewidth)) != TRANSPARENT_COLOR) { pdest[ -1] = translationTable[pcolor]; pz[ -1] = izi >> 16; } izi += izistep;
			// mankrip - end

			s = snext;
			t = tnext;
			// mankrip - begin
		}
		if (spancount > 0)
		{
			// mankrip - end

			// calculate s/z, t/z, zi->fixed s and t at last pixel in span (so can't step off polygon),
			// clamp, calculate s and t steps across span by division, biasing steps low so we don't run off the texture
			spancountminus1 = (float)(spancount - 1);
			// qbism ( http://forums.inside3d.com/viewtopic.php?t=2717 ) - begin
			sdivz += d_sdivzstepu * spancountminus1;
			tdivz += d_tdivzstepu * spancountminus1;
			zi += d_zistepu * spancountminus1;
			// qbism ( http://forums.inside3d.com/viewtopic.php?t=2717 ) - end
			z = (float)0x10000 / zi;	// prescale to 16.16 fixed-point
			snext = (int)(sdivz * z) + sadjust;
			if (snext > bbextents)
				snext = bbextents;
			// qbism ( http://forums.inside3d.com/viewtopic.php?t=2717 ) - begin
			else if (snext < 16)
				snext = 16;   // prevent round-off error on <0 steps from causing overstepping & running off the edge of the texture
			// qbism ( http://forums.inside3d.com/viewtopic.php?t=2717 ) - end

			tnext = (int)(tdivz * z) + tadjust;
			if (tnext > bbextentt)
				tnext = bbextentt;
			// qbism ( http://forums.inside3d.com/viewtopic.php?t=2717 ) - begin
			else if (tnext < 16)
				tnext = 16;   // guard against round-off error on <0 steps
			// qbism ( http://forums.inside3d.com/viewtopic.php?t=2717 ) - end

			if (spancount > 1)
			{
				sstep = (snext - s) / (spancount - 1);
				tstep = (tnext - t) / (spancount - 1);
			}

			//qbism- Duff's Device loop unroll per mh.
			pdest += spancount;
			// mankrip - begin
			pz += spancount;
			switch (spancount)
			{
				case 16: if (pz[-16] <= (izi >> 16)) if ( (pcolor = *(pbase + (s >> 16) + (t >> 16) * cachewidth)) != TRANSPARENT_COLOR) { pdest[-16] = translationTable[pcolor]; pz[-16] = izi >> 16; } izi += izistep; s += sstep; t += tstep;
				case 15: if (pz[-15] <= (izi >> 16)) if ( (pcolor = *(pbase + (s >> 16) + (t >> 16) * cachewidth)) != TRANSPARENT_COLOR) { pdest[-15] = translationTable[pcolor]; pz[-15] = izi >> 16; } izi += izistep; s += sstep; t += tstep;
				case 14: if (pz[-14] <= (izi >> 16)) if ( (pcolor = *(pbase + (s >> 16) + (t >> 16) * cachewidth)) != TRANSPARENT_COLOR) { pdest[-14] = translationTable[pcolor]; pz[-14] = izi >> 16; } izi += izistep; s += sstep; t += tstep;
				case 13: if (pz[-13] <= (izi >> 16)) if ( (pcolor = *(pbase + (s >> 16) + (t >> 16) * cachewidth)) != TRANSPARENT_COLOR) { pdest[-13] = translationTable[pcolor]; pz[-13] = izi >> 16; } izi += izistep; s += sstep; t += tstep;
				case 12: if (pz[-12] <= (izi >> 16)) if ( (pcolor = *(pbase + (s >> 16) + (t >> 16) * cachewidth)) != TRANSPARENT_COLOR) { pdest[-12] = translationTable[pcolor]; pz[-12] = izi >> 16; } izi += izistep; s += sstep; t += tstep;
				case 11: if (pz[-11] <= (izi >> 16)) if ( (pcolor = *(pbase + (s >> 16) + (t >> 16) * cachewidth)) != TRANSPARENT_COLOR) { pdest[-11] = translationTable[pcolor]; pz[-11] = izi >> 16; } izi += izistep; s += sstep; t += tstep;
				case 10: if (pz[-10] <= (izi >> 16)) if ( (pcolor = *(pbase + (s >> 16) + (t >> 16) * cachewidth)) != TRANSPARENT_COLOR) { pdest[-10] = translationTable[pcolor]; pz[-10] = izi >> 16; } izi += izistep; s += sstep; t += tstep;
				case  9: if (pz[ -9] <= (izi >> 16)) if ( (pcolor = *(pbase + (s >> 16) + (t >> 16) * cachewidth)) != TRANSPARENT_COLOR) { pdest[ -9] = translationTable[pcolor]; pz[ -9] = izi >> 16; } izi += izistep; s += sstep; t += tstep;
				case  8: if (pz[ -8] <= (izi >> 16)) if ( (pcolor = *(pbase + (s >> 16) + (t >> 16) * cachewidth)) != TRANSPARENT_COLOR) { pdest[ -8] = translationTable[pcolor]; pz[ -8] = izi >> 16; } izi += izistep; s += sstep; t += tstep;
				case  7: if (pz[ -7] <= (izi >> 16)) if ( (pcolor = *(pbase + (s >> 16) + (t >> 16) * cachewidth)) != TRANSPARENT_COLOR) { pdest[ -7] = translationTable[pcolor]; pz[ -7] = izi >> 16; } izi += izistep; s += sstep; t += tstep;
				case  6: if (pz[ -6] <= (izi >> 16)) if ( (pcolor = *(pbase + (s >> 16) + (t >> 16) * cachewidth)) != TRANSPARENT_COLOR) { pdest[ -6] = translationTable[pcolor]; pz[ -6] = izi >> 16; } izi += izistep; s += sstep; t += tstep;
				case  5: if (pz[ -5] <= (izi >> 16)) if ( (pcolor = *(pbase + (s >> 16) + (t >> 16) * cachewidth)) != TRANSPARENT_COLOR) { pdest[ -5] = translationTable[pcolor]; pz[ -5] = izi >> 16; } izi += izistep; s += sstep; t += tstep;
				case  4: if (pz[ -4] <= (izi >> 16)) if ( (pcolor = *(pbase + (s >> 16) + (t >> 16) * cachewidth)) != TRANSPARENT_COLOR) { pdest[ -4] = translationTable[pcolor]; pz[ -4] = izi >> 16; } izi += izistep; s += sstep; t += tstep;
				case  3: if (pz[ -3] <= (izi >> 16)) if ( (pcolor = *(pbase + (s >> 16) + (t >> 16) * cachewidth)) != TRANSPARENT_COLOR) { pdest[ -3] = translationTable[pcolor]; pz[ -3] = izi >> 16; } izi += izistep; s += sstep; t += tstep;
				case  2: if (pz[ -2] <= (izi >> 16)) if ( (pcolor = *(pbase + (s >> 16) + (t >> 16) * cachewidth)) != TRANSPARENT_COLOR) { pdest[ -2] = translationTable[pcolor]; pz[ -2] = izi >> 16; } izi += izistep; s += sstep; t += tstep;
				case  1: if (pz[ -1] <= (izi >> 16)) if ( (pcolor = *(pbase + (s >> 16) + (t >> 16) * cachewidth)) != TRANSPARENT_COLOR) { pdest[ -1] = translationTable[pcolor]; pz[ -1] = izi >> 16; }
				break;
			}
		}
		pspan++;
	} while (pspan->count != DS_SPAN_LIST_END);
}



void D_SpriteDrawSpans_Blend (void) // mankrip - transparencies
{
	sspan_t
		* pspan = sprite_spans
		;
	int			count, spancount, izistep;
	int			izi;
	byte		*pbase, *pdest;
	fixed16_t	s, t, snext, tnext, sstep, tstep;
	float		sdivz, tdivz, zi, z, du, dv, spancountminus1;
	float		sdivzstepu, tdivzstepu, zistepu;
	short		*pz;


	sstep = 0;	// keep compiler happy
	tstep = 0;	// ditto

	pbase = cacheblock;

	sdivzstepu = d_sdivzstepu * 16;
	tdivzstepu = d_tdivzstepu * 16;
	zistepu = d_zistepu * 16;

// we count on FP exceptions being turned off to avoid range problems
	izistep = (int)(d_zistepu * 0x8000 * 0x10000);

	do
	{
		pdest = (byte *)d_viewbuffer + (screenwidth * pspan->v) + pspan->u;
		pz = d_pzbuffer + (d_zwidth * pspan->v) + pspan->u;

		count = pspan->count >> 4; // mh
		spancount = pspan->count % 16; // mankrip

	//	if (count <= 0)
	//		goto NextSpan;

	// calculate the initial s/z, t/z, 1/z, s, and t and clamp
		du = (float)pspan->u;
		dv = (float)pspan->v;

		sdivz = d_sdivzorigin + dv*d_sdivzstepv + du*d_sdivzstepu;
		tdivz = d_tdivzorigin + dv*d_tdivzstepv + du*d_tdivzstepu;
		zi = d_ziorigin + dv*d_zistepv + du*d_zistepu;
		z = (float)0x10000 / zi;	// prescale to 16.16 fixed-point
		// we count on FP exceptions being turned off to avoid range problems
		izi = (int)(zi * 0x8000 * 0x10000);

		s = (int)(sdivz * z) + sadjust;
		if (s > bbextents)
			s = bbextents;
		else if (s < 0)
			s = 0;

		t = (int)(tdivz * z) + tadjust;
		if (t > bbextentt)
			t = bbextentt;
		else if (t < 0)
			t = 0;

		while (count--) // mankrip
		{
			// calculate s/z, t/z, zi->fixed s and t at far end of span,
			// calculate s and t steps across span by shifting
			// qbism ( http://forums.inside3d.com/viewtopic.php?t=2717 ) - begin
			sdivz += sdivzstepu;
			tdivz += tdivzstepu;
			zi += zistepu;
			// qbism ( http://forums.inside3d.com/viewtopic.php?t=2717 ) - end
			z = (float)0x10000 / zi;	// prescale to 16.16 fixed-point

			snext = (int)(sdivz * z) + sadjust;
			if (snext > bbextents)
				snext = bbextents;
			// qbism ( http://forums.inside3d.com/viewtopic.php?t=2717 ) - begin
			else if (snext < 16)
				snext = 16;   // prevent round-off error on <0 steps from causing overstepping & running off the edge of the texture
			// qbism ( http://forums.inside3d.com/viewtopic.php?t=2717 ) - end

			tnext = (int)(tdivz * z) + tadjust;
			if (tnext > bbextentt)
				tnext = bbextentt;
			// qbism ( http://forums.inside3d.com/viewtopic.php?t=2717 ) - begin
			else if (tnext < 16)
				tnext = 16;   // guard against round-off error on <0 steps

			sstep = (snext - s) >> 4;
			tstep = (tnext - t) >> 4;
			// qbism ( http://forums.inside3d.com/viewtopic.php?t=2717 ) - end

			// mankrip - begin
			pdest += 16;
			pz += 16;
			if (pz[-16] <= (izi >> 16)) { pdest[-16] = colorblendingmap[translationTable[*(pbase + (s >> 16) + (t >> 16) * cachewidth)] + pdest[-16] * 256]; } izi += izistep; s += sstep; t += tstep;
			if (pz[-15] <= (izi >> 16)) { pdest[-15] = colorblendingmap[translationTable[*(pbase + (s >> 16) + (t >> 16) * cachewidth)] + pdest[-15] * 256]; } izi += izistep; s += sstep; t += tstep;
			if (pz[-14] <= (izi >> 16)) { pdest[-14] = colorblendingmap[translationTable[*(pbase + (s >> 16) + (t >> 16) * cachewidth)] + pdest[-14] * 256]; } izi += izistep; s += sstep; t += tstep;
			if (pz[-13] <= (izi >> 16)) { pdest[-13] = colorblendingmap[translationTable[*(pbase + (s >> 16) + (t >> 16) * cachewidth)] + pdest[-13] * 256]; } izi += izistep; s += sstep; t += tstep;
			if (pz[-12] <= (izi >> 16)) { pdest[-12] = colorblendingmap[translationTable[*(pbase + (s >> 16) + (t >> 16) * cachewidth)] + pdest[-12] * 256]; } izi += izistep; s += sstep; t += tstep;
			if (pz[-11] <= (izi >> 16)) { pdest[-11] = colorblendingmap[translationTable[*(pbase + (s >> 16) + (t >> 16) * cachewidth)] + pdest[-11] * 256]; } izi += izistep; s += sstep; t += tstep;
			if (pz[-10] <= (izi >> 16)) { pdest[-10] = colorblendingmap[translationTable[*(pbase + (s >> 16) + (t >> 16) * cachewidth)] + pdest[-10] * 256]; } izi += izistep; s += sstep; t += tstep;
			if (pz[ -9] <= (izi >> 16)) { pdest[ -9] = colorblendingmap[translationTable[*(pbase + (s >> 16) + (t >> 16) * cachewidth)] + pdest[ -9] * 256]; } izi += izistep; s += sstep; t += tstep;
			if (pz[ -8] <= (izi >> 16)) { pdest[ -8] = colorblendingmap[translationTable[*(pbase + (s >> 16) + (t >> 16) * cachewidth)] + pdest[ -8] * 256]; } izi += izistep; s += sstep; t += tstep;
			if (pz[ -7] <= (izi >> 16)) { pdest[ -7] = colorblendingmap[translationTable[*(pbase + (s >> 16) + (t >> 16) * cachewidth)] + pdest[ -7] * 256]; } izi += izistep; s += sstep; t += tstep;
			if (pz[ -6] <= (izi >> 16)) { pdest[ -6] = colorblendingmap[translationTable[*(pbase + (s >> 16) + (t >> 16) * cachewidth)] + pdest[ -6] * 256]; } izi += izistep; s += sstep; t += tstep;
			if (pz[ -5] <= (izi >> 16)) { pdest[ -5] = colorblendingmap[translationTable[*(pbase + (s >> 16) + (t >> 16) * cachewidth)] + pdest[ -5] * 256]; } izi += izistep; s += sstep; t += tstep;
			if (pz[ -4] <= (izi >> 16)) { pdest[ -4] = colorblendingmap[translationTable[*(pbase + (s >> 16) + (t >> 16) * cachewidth)] + pdest[ -4] * 256]; } izi += izistep; s += sstep; t += tstep;
			if (pz[ -3] <= (izi >> 16)) { pdest[ -3] = colorblendingmap[translationTable[*(pbase + (s >> 16) + (t >> 16) * cachewidth)] + pdest[ -3] * 256]; } izi += izistep; s += sstep; t += tstep;
			if (pz[ -2] <= (izi >> 16)) { pdest[ -2] = colorblendingmap[translationTable[*(pbase + (s >> 16) + (t >> 16) * cachewidth)] + pdest[ -2] * 256]; } izi += izistep; s += sstep; t += tstep;
			if (pz[ -1] <= (izi >> 16)) { pdest[ -1] = colorblendingmap[translationTable[*(pbase + (s >> 16) + (t >> 16) * cachewidth)] + pdest[ -1] * 256]; } izi += izistep;
			// mankrip - end

			s = snext;
			t = tnext;
			// mankrip - begin
		}
		if (spancount > 0)
		{
			// mankrip - end

			// calculate s/z, t/z, zi->fixed s and t at last pixel in span (so can't step off polygon),
			// clamp, calculate s and t steps across span by division, biasing steps low so we don't run off the texture
			spancountminus1 = (float)(spancount - 1);
			// qbism ( http://forums.inside3d.com/viewtopic.php?t=2717 ) - begin
			sdivz += d_sdivzstepu * spancountminus1;
			tdivz += d_tdivzstepu * spancountminus1;
			zi += d_zistepu * spancountminus1;
			// qbism ( http://forums.inside3d.com/viewtopic.php?t=2717 ) - end
			z = (float)0x10000 / zi;	// prescale to 16.16 fixed-point
			snext = (int)(sdivz * z) + sadjust;
			if (snext > bbextents)
				snext = bbextents;
			// qbism ( http://forums.inside3d.com/viewtopic.php?t=2717 ) - begin
			else if (snext < 16)
				snext = 16;   // prevent round-off error on <0 steps from causing overstepping & running off the edge of the texture
			// qbism ( http://forums.inside3d.com/viewtopic.php?t=2717 ) - end

			tnext = (int)(tdivz * z) + tadjust;
			if (tnext > bbextentt)
				tnext = bbextentt;
			// qbism ( http://forums.inside3d.com/viewtopic.php?t=2717 ) - begin
			else if (tnext < 16)
				tnext = 16;   // guard against round-off error on <0 steps
			// qbism ( http://forums.inside3d.com/viewtopic.php?t=2717 ) - end

			if (spancount > 1)
			{
				sstep = (snext - s) / (spancount - 1);
				tstep = (tnext - t) / (spancount - 1);
			}

			//qbism- Duff's Device loop unroll per mh.
			pdest += spancount;
			// mankrip - begin
			pz += spancount;
			switch (spancount)
			{
				case 16: if (pz[-16] <= (izi >> 16)) { pdest[-16] = colorblendingmap[translationTable[*(pbase + (s >> 16) + (t >> 16) * cachewidth)] + pdest[-16] * 256]; } izi += izistep; s += sstep; t += tstep;
				case 15: if (pz[-15] <= (izi >> 16)) { pdest[-15] = colorblendingmap[translationTable[*(pbase + (s >> 16) + (t >> 16) * cachewidth)] + pdest[-15] * 256]; } izi += izistep; s += sstep; t += tstep;
				case 14: if (pz[-14] <= (izi >> 16)) { pdest[-14] = colorblendingmap[translationTable[*(pbase + (s >> 16) + (t >> 16) * cachewidth)] + pdest[-14] * 256]; } izi += izistep; s += sstep; t += tstep;
				case 13: if (pz[-13] <= (izi >> 16)) { pdest[-13] = colorblendingmap[translationTable[*(pbase + (s >> 16) + (t >> 16) * cachewidth)] + pdest[-13] * 256]; } izi += izistep; s += sstep; t += tstep;
				case 12: if (pz[-12] <= (izi >> 16)) { pdest[-12] = colorblendingmap[translationTable[*(pbase + (s >> 16) + (t >> 16) * cachewidth)] + pdest[-12] * 256]; } izi += izistep; s += sstep; t += tstep;
				case 11: if (pz[-11] <= (izi >> 16)) { pdest[-11] = colorblendingmap[translationTable[*(pbase + (s >> 16) + (t >> 16) * cachewidth)] + pdest[-11] * 256]; } izi += izistep; s += sstep; t += tstep;
				case 10: if (pz[-10] <= (izi >> 16)) { pdest[-10] = colorblendingmap[translationTable[*(pbase + (s >> 16) + (t >> 16) * cachewidth)] + pdest[-10] * 256]; } izi += izistep; s += sstep; t += tstep;
				case  9: if (pz[ -9] <= (izi >> 16)) { pdest[ -9] = colorblendingmap[translationTable[*(pbase + (s >> 16) + (t >> 16) * cachewidth)] + pdest[ -9] * 256]; } izi += izistep; s += sstep; t += tstep;
				case  8: if (pz[ -8] <= (izi >> 16)) { pdest[ -8] = colorblendingmap[translationTable[*(pbase + (s >> 16) + (t >> 16) * cachewidth)] + pdest[ -8] * 256]; } izi += izistep; s += sstep; t += tstep;
				case  7: if (pz[ -7] <= (izi >> 16)) { pdest[ -7] = colorblendingmap[translationTable[*(pbase + (s >> 16) + (t >> 16) * cachewidth)] + pdest[ -7] * 256]; } izi += izistep; s += sstep; t += tstep;
				case  6: if (pz[ -6] <= (izi >> 16)) { pdest[ -6] = colorblendingmap[translationTable[*(pbase + (s >> 16) + (t >> 16) * cachewidth)] + pdest[ -6] * 256]; } izi += izistep; s += sstep; t += tstep;
				case  5: if (pz[ -5] <= (izi >> 16)) { pdest[ -5] = colorblendingmap[translationTable[*(pbase + (s >> 16) + (t >> 16) * cachewidth)] + pdest[ -5] * 256]; } izi += izistep; s += sstep; t += tstep;
				case  4: if (pz[ -4] <= (izi >> 16)) { pdest[ -4] = colorblendingmap[translationTable[*(pbase + (s >> 16) + (t >> 16) * cachewidth)] + pdest[ -4] * 256]; } izi += izistep; s += sstep; t += tstep;
				case  3: if (pz[ -3] <= (izi >> 16)) { pdest[ -3] = colorblendingmap[translationTable[*(pbase + (s >> 16) + (t >> 16) * cachewidth)] + pdest[ -3] * 256]; } izi += izistep; s += sstep; t += tstep;
				case  2: if (pz[ -2] <= (izi >> 16)) { pdest[ -2] = colorblendingmap[translationTable[*(pbase + (s >> 16) + (t >> 16) * cachewidth)] + pdest[ -2] * 256]; } izi += izistep; s += sstep; t += tstep;
				case  1: if (pz[ -1] <= (izi >> 16)) { pdest[ -1] = colorblendingmap[translationTable[*(pbase + (s >> 16) + (t >> 16) * cachewidth)] + pdest[ -1] * 256]; }
				break;
			}
		}
//NextSpan:
		pspan++;

	} while (pspan->count != DS_SPAN_LIST_END);
}



void D_SpriteDrawSpans_BlendBackwards (void) // mankrip - transparencies
{
	sspan_t
		* pspan = sprite_spans
		;
	int			count, spancount, izistep;
	int			izi;
	byte		*pbase, *pdest;
	fixed16_t	s, t, snext, tnext, sstep, tstep;
	float		sdivz, tdivz, zi, z, du, dv, spancountminus1;
	float		sdivzstepu, tdivzstepu, zistepu;
	short		*pz;


	sstep = 0;	// keep compiler happy
	tstep = 0;	// ditto

	pbase = cacheblock;

	sdivzstepu = d_sdivzstepu * 16;
	tdivzstepu = d_tdivzstepu * 16;
	zistepu = d_zistepu * 16;

// we count on FP exceptions being turned off to avoid range problems
	izistep = (int)(d_zistepu * 0x8000 * 0x10000);

	do
	{
		pdest = (byte *)d_viewbuffer + (screenwidth * pspan->v) + pspan->u;
		pz = d_pzbuffer + (d_zwidth * pspan->v) + pspan->u;

		count = pspan->count >> 4; // mh
		spancount = pspan->count % 16; // mankrip

	//	if (count <= 0)
	//		goto NextSpan;

	// calculate the initial s/z, t/z, 1/z, s, and t and clamp
		du = (float)pspan->u;
		dv = (float)pspan->v;

		sdivz = d_sdivzorigin + dv*d_sdivzstepv + du*d_sdivzstepu;
		tdivz = d_tdivzorigin + dv*d_tdivzstepv + du*d_tdivzstepu;
		zi = d_ziorigin + dv*d_zistepv + du*d_zistepu;
		z = (float)0x10000 / zi;	// prescale to 16.16 fixed-point
		// we count on FP exceptions being turned off to avoid range problems
		izi = (int)(zi * 0x8000 * 0x10000);

		s = (int)(sdivz * z) + sadjust;
		if (s > bbextents)
			s = bbextents;
		else if (s < 0)
			s = 0;

		t = (int)(tdivz * z) + tadjust;
		if (t > bbextentt)
			t = bbextentt;
		else if (t < 0)
			t = 0;

		while (count--) // mankrip
		{
			// calculate s/z, t/z, zi->fixed s and t at far end of span,
			// calculate s and t steps across span by shifting
			// qbism ( http://forums.inside3d.com/viewtopic.php?t=2717 ) - begin
			sdivz += sdivzstepu;
			tdivz += tdivzstepu;
			zi += zistepu;
			// qbism ( http://forums.inside3d.com/viewtopic.php?t=2717 ) - end
			z = (float)0x10000 / zi;	// prescale to 16.16 fixed-point

			snext = (int)(sdivz * z) + sadjust;
			if (snext > bbextents)
				snext = bbextents;
			// qbism ( http://forums.inside3d.com/viewtopic.php?t=2717 ) - begin
			else if (snext < 16)
				snext = 16;   // prevent round-off error on <0 steps from causing overstepping & running off the edge of the texture
			// qbism ( http://forums.inside3d.com/viewtopic.php?t=2717 ) - end

			tnext = (int)(tdivz * z) + tadjust;
			if (tnext > bbextentt)
				tnext = bbextentt;
			// qbism ( http://forums.inside3d.com/viewtopic.php?t=2717 ) - begin
			else if (tnext < 16)
				tnext = 16;   // guard against round-off error on <0 steps

			sstep = (snext - s) >> 4;
			tstep = (tnext - t) >> 4;
			// qbism ( http://forums.inside3d.com/viewtopic.php?t=2717 ) - end

			// mankrip - begin
			pdest += 16;
			pz += 16;
			if (pz[-16] <= (izi >> 16)) { pdest[-16] = colorblendingmap[pdest[-16] + translationTable[*(pbase + (s >> 16) + (t >> 16) * cachewidth)] * 256]; } izi += izistep; s += sstep; t += tstep;
			if (pz[-15] <= (izi >> 16)) { pdest[-15] = colorblendingmap[pdest[-15] + translationTable[*(pbase + (s >> 16) + (t >> 16) * cachewidth)] * 256]; } izi += izistep; s += sstep; t += tstep;
			if (pz[-14] <= (izi >> 16)) { pdest[-14] = colorblendingmap[pdest[-14] + translationTable[*(pbase + (s >> 16) + (t >> 16) * cachewidth)] * 256]; } izi += izistep; s += sstep; t += tstep;
			if (pz[-13] <= (izi >> 16)) { pdest[-13] = colorblendingmap[pdest[-13] + translationTable[*(pbase + (s >> 16) + (t >> 16) * cachewidth)] * 256]; } izi += izistep; s += sstep; t += tstep;
			if (pz[-12] <= (izi >> 16)) { pdest[-12] = colorblendingmap[pdest[-12] + translationTable[*(pbase + (s >> 16) + (t >> 16) * cachewidth)] * 256]; } izi += izistep; s += sstep; t += tstep;
			if (pz[-11] <= (izi >> 16)) { pdest[-11] = colorblendingmap[pdest[-11] + translationTable[*(pbase + (s >> 16) + (t >> 16) * cachewidth)] * 256]; } izi += izistep; s += sstep; t += tstep;
			if (pz[-10] <= (izi >> 16)) { pdest[-10] = colorblendingmap[pdest[-10] + translationTable[*(pbase + (s >> 16) + (t >> 16) * cachewidth)] * 256]; } izi += izistep; s += sstep; t += tstep;
			if (pz[ -9] <= (izi >> 16)) { pdest[ -9] = colorblendingmap[pdest[ -9] + translationTable[*(pbase + (s >> 16) + (t >> 16) * cachewidth)] * 256]; } izi += izistep; s += sstep; t += tstep;
			if (pz[ -8] <= (izi >> 16)) { pdest[ -8] = colorblendingmap[pdest[ -8] + translationTable[*(pbase + (s >> 16) + (t >> 16) * cachewidth)] * 256]; } izi += izistep; s += sstep; t += tstep;
			if (pz[ -7] <= (izi >> 16)) { pdest[ -7] = colorblendingmap[pdest[ -7] + translationTable[*(pbase + (s >> 16) + (t >> 16) * cachewidth)] * 256]; } izi += izistep; s += sstep; t += tstep;
			if (pz[ -6] <= (izi >> 16)) { pdest[ -6] = colorblendingmap[pdest[ -6] + translationTable[*(pbase + (s >> 16) + (t >> 16) * cachewidth)] * 256]; } izi += izistep; s += sstep; t += tstep;
			if (pz[ -5] <= (izi >> 16)) { pdest[ -5] = colorblendingmap[pdest[ -5] + translationTable[*(pbase + (s >> 16) + (t >> 16) * cachewidth)] * 256]; } izi += izistep; s += sstep; t += tstep;
			if (pz[ -4] <= (izi >> 16)) { pdest[ -4] = colorblendingmap[pdest[ -4] + translationTable[*(pbase + (s >> 16) + (t >> 16) * cachewidth)] * 256]; } izi += izistep; s += sstep; t += tstep;
			if (pz[ -3] <= (izi >> 16)) { pdest[ -3] = colorblendingmap[pdest[ -3] + translationTable[*(pbase + (s >> 16) + (t >> 16) * cachewidth)] * 256]; } izi += izistep; s += sstep; t += tstep;
			if (pz[ -2] <= (izi >> 16)) { pdest[ -2] = colorblendingmap[pdest[ -2] + translationTable[*(pbase + (s >> 16) + (t >> 16) * cachewidth)] * 256]; } izi += izistep; s += sstep; t += tstep;
			if (pz[ -1] <= (izi >> 16)) { pdest[ -1] = colorblendingmap[pdest[ -1] + translationTable[*(pbase + (s >> 16) + (t >> 16) * cachewidth)] * 256]; }
			// mankrip - end

			s = snext;
			t = tnext;
			// mankrip - begin
		}
		if (spancount > 0)
		{
			// mankrip - end

			// calculate s/z, t/z, zi->fixed s and t at last pixel in span (so can't step off polygon),
			// clamp, calculate s and t steps across span by division, biasing steps low so we don't run off the texture
			spancountminus1 = (float)(spancount - 1);
			// qbism ( http://forums.inside3d.com/viewtopic.php?t=2717 ) - begin
			sdivz += d_sdivzstepu * spancountminus1;
			tdivz += d_tdivzstepu * spancountminus1;
			zi += d_zistepu * spancountminus1;
			// qbism ( http://forums.inside3d.com/viewtopic.php?t=2717 ) - end
			z = (float)0x10000 / zi;	// prescale to 16.16 fixed-point
			snext = (int)(sdivz * z) + sadjust;
			if (snext > bbextents)
				snext = bbextents;
			// qbism ( http://forums.inside3d.com/viewtopic.php?t=2717 ) - begin
			else if (snext < 16)
				snext = 16;   // prevent round-off error on <0 steps from causing overstepping & running off the edge of the texture
			// qbism ( http://forums.inside3d.com/viewtopic.php?t=2717 ) - end

			tnext = (int)(tdivz * z) + tadjust;
			if (tnext > bbextentt)
				tnext = bbextentt;
			// qbism ( http://forums.inside3d.com/viewtopic.php?t=2717 ) - begin
			else if (tnext < 16)
				tnext = 16;   // guard against round-off error on <0 steps
			// qbism ( http://forums.inside3d.com/viewtopic.php?t=2717 ) - end

			if (spancount > 1)
			{
				sstep = (snext - s) / (spancount - 1);
				tstep = (tnext - t) / (spancount - 1);
			}

			//qbism- Duff's Device loop unroll per mh.
			pdest += spancount;
			// mankrip - begin
			pz += spancount;
			switch (spancount)
			{
				case 16: if (pz[-16] <= (izi >> 16)) { pdest[-16] = colorblendingmap[pdest[-16] + translationTable[*(pbase + (s >> 16) + (t >> 16) * cachewidth)] * 256]; } izi += izistep; s += sstep; t += tstep;
				case 15: if (pz[-15] <= (izi >> 16)) { pdest[-15] = colorblendingmap[pdest[-15] + translationTable[*(pbase + (s >> 16) + (t >> 16) * cachewidth)] * 256]; } izi += izistep; s += sstep; t += tstep;
				case 14: if (pz[-14] <= (izi >> 16)) { pdest[-14] = colorblendingmap[pdest[-14] + translationTable[*(pbase + (s >> 16) + (t >> 16) * cachewidth)] * 256]; } izi += izistep; s += sstep; t += tstep;
				case 13: if (pz[-13] <= (izi >> 16)) { pdest[-13] = colorblendingmap[pdest[-13] + translationTable[*(pbase + (s >> 16) + (t >> 16) * cachewidth)] * 256]; } izi += izistep; s += sstep; t += tstep;
				case 12: if (pz[-12] <= (izi >> 16)) { pdest[-12] = colorblendingmap[pdest[-12] + translationTable[*(pbase + (s >> 16) + (t >> 16) * cachewidth)] * 256]; } izi += izistep; s += sstep; t += tstep;
				case 11: if (pz[-11] <= (izi >> 16)) { pdest[-11] = colorblendingmap[pdest[-11] + translationTable[*(pbase + (s >> 16) + (t >> 16) * cachewidth)] * 256]; } izi += izistep; s += sstep; t += tstep;
				case 10: if (pz[-10] <= (izi >> 16)) { pdest[-10] = colorblendingmap[pdest[-10] + translationTable[*(pbase + (s >> 16) + (t >> 16) * cachewidth)] * 256]; } izi += izistep; s += sstep; t += tstep;
				case  9: if (pz[ -9] <= (izi >> 16)) { pdest[ -9] = colorblendingmap[pdest[ -9] + translationTable[*(pbase + (s >> 16) + (t >> 16) * cachewidth)] * 256]; } izi += izistep; s += sstep; t += tstep;
				case  8: if (pz[ -8] <= (izi >> 16)) { pdest[ -8] = colorblendingmap[pdest[ -8] + translationTable[*(pbase + (s >> 16) + (t >> 16) * cachewidth)] * 256]; } izi += izistep; s += sstep; t += tstep;
				case  7: if (pz[ -7] <= (izi >> 16)) { pdest[ -7] = colorblendingmap[pdest[ -7] + translationTable[*(pbase + (s >> 16) + (t >> 16) * cachewidth)] * 256]; } izi += izistep; s += sstep; t += tstep;
				case  6: if (pz[ -6] <= (izi >> 16)) { pdest[ -6] = colorblendingmap[pdest[ -6] + translationTable[*(pbase + (s >> 16) + (t >> 16) * cachewidth)] * 256]; } izi += izistep; s += sstep; t += tstep;
				case  5: if (pz[ -5] <= (izi >> 16)) { pdest[ -5] = colorblendingmap[pdest[ -5] + translationTable[*(pbase + (s >> 16) + (t >> 16) * cachewidth)] * 256]; } izi += izistep; s += sstep; t += tstep;
				case  4: if (pz[ -4] <= (izi >> 16)) { pdest[ -4] = colorblendingmap[pdest[ -4] + translationTable[*(pbase + (s >> 16) + (t >> 16) * cachewidth)] * 256]; } izi += izistep; s += sstep; t += tstep;
				case  3: if (pz[ -3] <= (izi >> 16)) { pdest[ -3] = colorblendingmap[pdest[ -3] + translationTable[*(pbase + (s >> 16) + (t >> 16) * cachewidth)] * 256]; } izi += izistep; s += sstep; t += tstep;
				case  2: if (pz[ -2] <= (izi >> 16)) { pdest[ -2] = colorblendingmap[pdest[ -2] + translationTable[*(pbase + (s >> 16) + (t >> 16) * cachewidth)] * 256]; } izi += izistep; s += sstep; t += tstep;
				case  1: if (pz[ -1] <= (izi >> 16)) { pdest[ -1] = colorblendingmap[pdest[ -1] + translationTable[*(pbase + (s >> 16) + (t >> 16) * cachewidth)] * 256]; }
				break;
			}
		}
//NextSpan:
		pspan++;

	} while (pspan->count != DS_SPAN_LIST_END);
}

void D_SpriteDrawSpans_Shadow (void) // mankrip - transparencies
{
	sspan_t
		* pspan = sprite_spans
		;
	int			count, spancount, izistep;
	int			izi;
	byte		*pbase, *pdest;
	fixed16_t	s, t, snext, tnext, sstep, tstep;
	float		sdivz, tdivz, zi, z, du, dv, spancountminus1;
	float		sdivz8stepu, tdivz8stepu, zi8stepu;
	byte		btemp;
	short		*pz;
	float intensity; // mankrip


	sstep = 0;	// keep compiler happy
	tstep = 0;	// ditto

	pbase = cacheblock;

	sdivz8stepu = d_sdivzstepu * 8;
	tdivz8stepu = d_tdivzstepu * 8;
	zi8stepu = d_zistepu * 8;

// we count on FP exceptions being turned off to avoid range problems
	izistep = (int)(d_zistepu * 0x8000 * 0x10000);

	do
	{
		pdest = (byte *)d_viewbuffer + (screenwidth * pspan->v) + pspan->u;
		pz = d_pzbuffer + (d_zwidth * pspan->v) + pspan->u;

		count = pspan->count;

		if (count <= 0)
			goto NextSpan;

	// calculate the initial s/z, t/z, 1/z, s, and t and clamp
		du = (float)pspan->u;
		dv = (float)pspan->v;

		sdivz = d_sdivzorigin + dv*d_sdivzstepv + du*d_sdivzstepu;
		tdivz = d_tdivzorigin + dv*d_tdivzstepv + du*d_tdivzstepu;
		zi = d_ziorigin + dv*d_zistepv + du*d_zistepu;
		z = (float)0x10000 / zi;	// prescale to 16.16 fixed-point
	// we count on FP exceptions being turned off to avoid range problems
		izi = (int)(zi * 0x8000 * 0x10000);

		s = (int)(sdivz * z) + sadjust;
		if (s > bbextents)
			s = bbextents;
		else if (s < 0)
			s = 0;

		t = (int)(tdivz * z) + tadjust;
		if (t > bbextentt)
			t = bbextentt;
		else if (t < 0)
			t = 0;

		do
		{
		// calculate s and t at the far end of the span
			if (count >= 8)
				spancount = 8;
			else
				spancount = count;

			count -= spancount;

			if (count)
			{
			// calculate s/z, t/z, zi->fixed s and t at far end of span,
			// calculate s and t steps across span by shifting
				sdivz += sdivz8stepu;
				tdivz += tdivz8stepu;
				zi += zi8stepu;
				z = (float)0x10000 / zi;	// prescale to 16.16 fixed-point

				snext = (int)(sdivz * z) + sadjust;
				if (snext > bbextents)
					snext = bbextents;
				else if (snext < 8)
					snext = 8;	// prevent round-off error on <0 steps from
								//  from causing overstepping & running off the
								//  edge of the texture

				tnext = (int)(tdivz * z) + tadjust;
				if (tnext > bbextentt)
					tnext = bbextentt;
				else if (tnext < 8)
					tnext = 8;	// guard against round-off error on <0 steps

				sstep = (snext - s) >> 3;
				tstep = (tnext - t) >> 3;
			}
			else
			{
			// calculate s/z, t/z, zi->fixed s and t at last pixel in span (so
			// can't step off polygon), clamp, calculate s and t steps across
			// span by division, biasing steps low so we don't run off the
			// texture
				spancountminus1 = (float)(spancount - 1);
				sdivz += d_sdivzstepu * spancountminus1;
				tdivz += d_tdivzstepu * spancountminus1;
				zi += d_zistepu * spancountminus1;
				z = (float)0x10000 / zi;	// prescale to 16.16 fixed-point
				snext = (int)(sdivz * z) + sadjust;
				if (snext > bbextents)
					snext = bbextents;
				else if (snext < 8)
					snext = 8;	// prevent round-off error on <0 steps from
								//  from causing overstepping & running off the
								//  edge of the texture

				tnext = (int)(tdivz * z) + tadjust;
				if (tnext > bbextentt)
					tnext = bbextentt;
				else if (tnext < 8)
					tnext = 8;	// guard against round-off error on <0 steps

				if (spancount > 1)
				{
					sstep = (snext - s) / (spancount - 1);
					tstep = (tnext - t) / (spancount - 1);
				}
			}

			do
			{
				btemp = *(pbase + (s >> 16) + (t >> 16) * cachewidth);
				if (btemp != TRANSPARENT_COLOR)
				// mankrip - begin
				{
					intensity = (1.0 - ((float)((izi >> 16)- *pz)/20.0));
					if (intensity == 1 && currententity->alpha == 1)
						*pdest = 0;
					else if (intensity > 0 && intensity <= 1)
						*pdest = (byte)colorshadingmap[WITH_BRIGHTS][256 + *pdest + ((32 + (int)(31.0*intensity*currententity->alpha)) * 256)];
				}
				// mankrip - end

				izi += izistep;
				pdest++;
				pz++;
				s += sstep;
				t += tstep;
			} while (--spancount > 0);

			s = snext;
			t = tnext;

		} while (count > 0);

NextSpan:
		pspan++;

	} while (pspan->count != DS_SPAN_LIST_END);
}


/*
=====================
D_SpriteScanLeftEdge
=====================
*/
void D_SpriteScanLeftEdge (void)
{
	int
		i = (minindex) ? minindex : r_spritedesc.nump
	,	v
	,	ibottom
	,	lmaxindex = (maxindex) ? maxindex : r_spritedesc.nump
		;
	emitpoint_t
		* pvert
	,	* pnext
		;
	sspan_t
		* pspan = sprite_spans
		;
	float
		vtop = ceil (r_spritedesc.pverts[i].v)
	,	vbottom
	,	slope
		;
	fixed16_t
		u
	,	u_step
		;

	do
	{
		pvert = &r_spritedesc.pverts[i];
		pnext = pvert - 1;

		vbottom = ceil (pnext->v);

		if (vtop < vbottom)
		{
			slope = (pnext->u - pvert->u) / (pnext->v - pvert->v); // mankrip - du / dv (delta u / delta v)
			u_step = (int) (slope * 0x10000);
			// adjust u to ceil the integer portion
			u = (int) ( (pvert->u + (slope * (vtop - pvert->v))) * 0x10000) + (0x10000 - 1);
			ibottom = (int)vbottom;

			for (v = (int)vtop ; v < ibottom ; v++) // mankrip - edited
			{
				pspan->u = u >> 16;
				pspan->v = v;
				u += u_step;
				pspan++;
			}
		}

		vtop = vbottom;

		if (--i == 0) // mankrip - edited
			i = r_spritedesc.nump;

	} while (i != lmaxindex);
}


/*
=====================
D_SpriteScanRightEdge
=====================
*/
void D_SpriteScanRightEdge (void)
{
	int
		i = minindex
	,	v
	,	ibottom
		;
	emitpoint_t
		*pvert
	,	*pnext
		;
	sspan_t
		* pspan = sprite_spans
		;
	float
		vtop
	,	vbottom
	,	slope
	,	uvert
	,	unext
	,	vvert
	,	vnext
		;
	fixed16_t
		u
	,	u_step
		;

	vvert = r_spritedesc.pverts[i].v;
	if (vvert < r_refdef.fvrecty_adj)
		vvert = r_refdef.fvrecty_adj;
	else if (vvert > r_refdef.fvrectbottom_adj)
		vvert = r_refdef.fvrectbottom_adj;

	vtop = ceil (vvert);

	do
	{
		pvert = &r_spritedesc.pverts[i];
		pnext = pvert + 1;

		vnext = pnext->v;
		if (vnext < r_refdef.fvrecty_adj)
			vnext = r_refdef.fvrecty_adj;
		else if (vnext > r_refdef.fvrectbottom_adj)
			vnext = r_refdef.fvrectbottom_adj;

		vbottom = ceil (vnext);

		if (vtop < vbottom)
		{
			uvert = pvert->u;
			if (uvert < r_refdef.fvrectx_adj)
				uvert = r_refdef.fvrectx_adj;
			else // mankrip
			if (uvert > r_refdef.fvrectright_adj)
				uvert = r_refdef.fvrectright_adj;

			unext = pnext->u;
			if (unext < r_refdef.fvrectx_adj)
				unext = r_refdef.fvrectx_adj;
			else // mankrip
			if (unext > r_refdef.fvrectright_adj)
				unext = r_refdef.fvrectright_adj;

			slope = (unext - uvert) / (vnext - vvert); // mankrip - du / dv (delta u / delta v)
			u_step = (int)(slope * 0x10000);
			// adjust u to ceil the integer portion
			u = (int)((uvert + (slope * (vtop - vvert))) * 0x10000) + (0x10000 - 1);
			ibottom = (int)vbottom;

			for (v = (int)vtop ; v < ibottom ; v++) // mankrip - edited
			{
				pspan->count = (u >> 16) - pspan->u;
				u += u_step;
				pspan++;
			}
		}

		vtop = vbottom;
		vvert = vnext;

		if (++i == r_spritedesc.nump)
			i = 0;

	} while (i != maxindex);

	pspan->count = DS_SPAN_LIST_END;	// mark the end of the span list
}


/*
=====================
D_SpriteCalculateGradients
=====================
*/
void D_SpriteCalculateGradients (void)
{
	vec3_t
		p_normal
	,	p_saxis
	,	p_taxis
	,	p_temp1
		;
	float
		distinv
	,	scale // mankrip - QC Scale
		;

	TransformVector (r_spritedesc.vpn	, p_normal);
	TransformVector (r_spritedesc.vright, p_saxis);
	TransformVector (r_spritedesc.vup	, p_taxis);
	VectorInverse (p_taxis);

	distinv = 1.0f / (-DotProduct (modelorg, r_spritedesc.vpn));

	// mankrip - QC Scale - begin
	scale = (currententity->scale2 * currententity->scalev[0]);
	p_saxis[0] /= scale;
	p_saxis[1] /= scale;
	p_saxis[2] /= scale;

	scale = (currententity->scale2 * currententity->scalev[1]);
	p_taxis[0] /= scale;
	p_taxis[1] /= scale;
	p_taxis[2] /= scale;
	// mankrip - QC Scale - end

	d_sdivzstepu = p_saxis[0] * xscaleinv;
	d_tdivzstepu = p_taxis[0] * xscaleinv;

	d_sdivzstepv = -p_saxis[1] * yscaleinv;
	d_tdivzstepv = -p_taxis[1] * yscaleinv;

	d_zistepu =  p_normal[0] * xscaleinv * distinv;
	d_zistepv = -p_normal[1] * yscaleinv * distinv;

	d_sdivzorigin = p_saxis[2] - xcenter * d_sdivzstepu - ycenter * d_sdivzstepv;
	d_tdivzorigin = p_taxis[2] - xcenter * d_tdivzstepu - ycenter * d_tdivzstepv;
	d_ziorigin = p_normal[2] * distinv - xcenter * d_zistepu - ycenter * d_zistepv;

	TransformVector (modelorg, p_temp1);

	sadjust = ( (fixed16_t) (DotProduct (p_temp1, p_saxis) * 0x10000 + 0.5)) - (- (cachewidth	 >> 1) << 16);
	tadjust = ( (fixed16_t) (DotProduct (p_temp1, p_taxis) * 0x10000 + 0.5)) - (- (sprite_height >> 1) << 16);

	// -1 (-epsilon) so we never wander off the edge of the texture
	bbextents = (cachewidth		<< 16) - 1;
	bbextentt = (sprite_height	<< 16) - 1;
}


/*
=====================
D_DrawSprite
=====================
*/
void D_DrawSprite (void)
{
	sspan_t
		spans[MAXHEIGHT + 1]
		;
	emitpoint_t*
		pverts = r_spritedesc.pverts
		;
	int
		i = 0
	,	nump
		;
	float
		ymin = 999999.9f
	,	ymax = -999999.9f
		;

	// find the top and bottom vertices, and make sure there's at least one scan to draw
	for ( ; i < r_spritedesc.nump ; i++)
	{
		if (pverts->v < ymin)
		{
			ymin = pverts->v;
			minindex = i;
		}

		else // mankrip
		if (pverts->v > ymax)
		{
			ymax = pverts->v;
			maxindex = i;
		}

		pverts++;
	}

	ymin = ceil (ymin);
	ymax = ceil (ymax);

	if (ymin < ymax) // if it crosses any scans (front-faced?)
	{
		// mankrip - begin
		if (r_sprite_lit.value)
		{
			int
				r_ambientlight
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
			// minimum of (255 minus) 12 (12 less than in the viewmodel), maximum of (255 minus) 192 (same as MDL models)
			currententity->lightlevel = (r_ambientlight > 192) ? 1.0f : ( (r_ambientlight < 12) ? (12.0f / 192.0f) : ( (float)r_ambientlight / 192.0f));
		//	Q_memcpy (translationTable, currententity->colormap + ( ( ( (shadelight > 192) ? 63 : ( (shadelight < 6) ? 249 : 255 - shadelight)) << VID_CBITS) & 0xFF00), 256);
			Q_memcpy (translationTable, currententity->colormap + ( ( (255 - (int) (128.0f * currententity->lightlevel)) << VID_CBITS) & 0xFF00), 256);
		//	Q_memcpy (translationTable, currententity->colormap + 63 * (int) ceil ( (shadelight > 192) ? 1.0f : ( (shadelight < 6) ? (6.0f / 192.0f) : ( (float)shadelight / 192.0f))), 256);
		}
		else // fullbright
			Q_memcpy (translationTable, currentTable, 256);
		// mankrip - end

		cachewidth		= r_spritedesc.pspriteframe->width;
		sprite_height	= r_spritedesc.pspriteframe->height;
		cacheblock = (byte *)&r_spritedesc.pspriteframe->pixels[0];

		nump	= r_spritedesc.nump;
		pverts	= r_spritedesc.pverts;
		// copy the first vertex to the last vertex, so we don't have to deal with wrapping
		pverts[nump] = pverts[0];

		D_SpriteCalculateGradients ();
		sprite_spans = spans; // only used by the three functions below
		D_SpriteScanLeftEdge ();
		D_SpriteScanRightEdge ();
		currententity->D_SpriteDrawSpans (); // mankrip
	}
}

