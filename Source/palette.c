/*
Copyright (C) 2010 mankrip ( mankrip@gmail.com )

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

#include "quakedef.h"
#include "r_local.h"

qboolean
	palette_initialized = false
	;
byte
	* host_basepal
,	* shifted_basepal
,	* currentTable
,	* identityTable
,	* tintTable
,	* translationTable
	;



//================================================================
//================== Id Software GPL code begin ==================
//================================================================
byte BestColor (int r, int g, int b, int start, int stop)
{
	int	i;
	int	dr, dg, db;
	int	bestdistortion, distortion;
	int	bestcolor;
	byte	*pal;

//
// let any color go to 0 as a last resort
//
	bestdistortion = ( (int)r*r + (int)g*g + (int)b*b )*2;
	bestcolor = 0;

	pal = host_basepal + start*3;//lbmpalette
	for (i=start ; i<= stop ; i++)
	{
		dr = r - (int)pal[0];
		dg = g - (int)pal[1];
		db = b - (int)pal[2];
		pal += 3;
		distortion = dr*dr + dg*dg + db*db;
		if (distortion < bestdistortion)
		{
			if (!distortion)
				return i;		// perfect match

			bestdistortion = distortion;
			bestcolor = i;
		}
	}

	return bestcolor;
}



/*
==============================================================================

GAMMA

==============================================================================
*/

cvar_t		v_gamma = {"gamma", "1", true};

byte		gammatable[256];	// palette is sent through this



void BuildGammaTable (float g)
{
	int		i, inf;

	if (g == 1.0)
	{
		for (i=0 ; i<256 ; i++)
			gammatable[i] = i;
		return;
	}

	for (i=0 ; i<256 ; i++)
	{
		inf = 255 * pow ( (i+0.5)/255.5 , g ) + 0.5;
		gammatable[i] = minmaxclamp (0, inf, 255); // mankrip - edited
	}
}



qboolean V_CheckGamma (void)
{
	static float oldgammavalue;

	if (v_gamma.value == oldgammavalue)
		return false;
	oldgammavalue = v_gamma.value;

	BuildGammaTable (v_gamma.value);
	vid.recalc_refdef = 1;				// force a surface cache flush

	return true;
}
//================================================================
//=================== Id Software GPL code end ===================
//================================================================



/*
=============================================================================

BACKWARDS RANGES

=============================================================================
*/



void RemapBackwards_Palette (byte *p)
{
	byte
		newcolors[16*3*6];
	int
		c
	,	i;

	p += 16*3*8;
	for (c=0 ; c<16*3*6 ; c+=16*3)
		for (i=0 ; i<=15*3 ; i+=3)
		{
			newcolors[c+i  ] = p[c+15*3-i  ];
			newcolors[c+i+1] = p[c+15*3-i+1];
			newcolors[c+i+2] = p[c+15*3-i+2];
		}
	Q_memcpy (p, newcolors, 16*3*6);
}



void RemapBackwards_Pixels (byte *i, int size)
{
	// TO DO: optimize this
	int p=0;

	for ( ; p<size ; p++)
		if (i[p] >= 16*8 && i[p] < 16*14)
			i[p] = (i[p] / 16) * 16 + 15 - i[p]%16;
}



void RemapColorKey_Pixels (byte *i, int size)
{
	int p=0;

	for ( ; p<size ; p++)
		if (!i[p])
			i[p] = 255;
}



/*
=============================================================================

COLOR REMAPPING

=============================================================================
*/



void BuildColorTranslationTable (int top, int bottom, byte *source, byte *dest)
{
	memcpy (dest, source, 256);

	memcpy (dest + TOP_RANGE   , source + top    * 16, 16);
	memcpy (dest + BOTTOM_RANGE, source + bottom * 16, 16);
}



/*
=============================================================================

COLOR SHADING

=============================================================================
*/

cvar_t
	r_fullbright_colors		= {"r_fullbright_colors"	, "32", false}
,	r_fullbright_range		= {"r_fullbright_range"		,  "2", false}
,	r_fullbright_cel_range	= {"r_fullbright_cel_range"	,  "2", false}
,	r_fullbright_cel_factor	= {"r_fullbright_cel_factor", "16", false}
,	r_brights_world		= {"r_brights_world"	, "1", true}
,	r_brights_staticent	= {"r_brights_staticent", "1", true}
,	r_brights_beam		= {"r_brights_beam"		, "1", true}
,	r_brights_entity	= {"r_brights_entity"	, "1", true}
,	r_brights_player	= {"r_brights_player"	, "1", true}
,	r_brights_viewmodel	= {"r_brights_viewmodel", "1", true}
,	r_brights_particle	= {"r_brights_particle"	, "1", true}
	;
void
	* colormap
	;
byte
	** colorshadingmap
,	* colorshadingmap_world
,	* colorshadingmap_staticent
,	* colorshadingmap_beam
,	* colorshadingmap_entity
,	* colorshadingmap_player
,	* colorshadingmap_viewmodel
,	* colorshadingmap_particle
	;



void ApplyBrights (void)
{
	int
		i
//	,	e
		;
	for (i = 0 ; i < cl.num_statics ; i++)
		cl_static_entities[i].colormap = (cl_static_entities[i].model->name[0] == '*') ? colorshadingmap_world : colorshadingmap_staticent;
	for (i=0 ; i<cl.maxclients ; i++)
		CL_NewTranslation (i);
	#if 0
	if (cl.worldmodel)
		for (i=0 ; i<cl.worldmodel->numtextures ; i++) // cl_entitites[0].model
			cl.worldmodel->textures[i]->colorshadingmap = colorshadingmap_world;
	// start on the entity after the world
	for (e=1 ; e<cl.num_entities ; e++)
		if (cl_entities[e].model)
			for (i=0 ; i<cl_entities[e].model->numtextures ; i++)
					cl_entities[e].model->textures[i]->colorshadingmap = (cl_entities[e].model->name[0] == '*') ? colorshadingmap_world : colorshadingmap_entity;
//	for (i=0 ; i<cl.worldmodel->numsurfaces ; i++)
//		cl.worldmodel->surfaces->colorshadingmap = colorshadingmap_world;
	#endif

	vid.recalc_refdef = true;
	D_FlushCaches ();
}



// TO DO: replace these ApplyBright calls with a r_applybrights = true;
void SetBrights_world		(void)	{	colorshadingmap_world		= &colorshadingmap[minmaxclamp(0, (int) r_brights_world		.value, COLORSHADINGMAP_STYLES-1)][256]; ApplyBrights ();	}
void SetBrights_staticent	(void)	{	colorshadingmap_staticent	= &colorshadingmap[minmaxclamp(0, (int) r_brights_staticent	.value, COLORSHADINGMAP_STYLES-1)][256]; ApplyBrights ();	}
void SetBrights_beam		(void)	{	colorshadingmap_beam		= &colorshadingmap[minmaxclamp(0, (int) r_brights_beam		.value, COLORSHADINGMAP_STYLES-1)][256]; ApplyBrights ();	}
void SetBrights_entity		(void)	{	colorshadingmap_entity		= &colorshadingmap[minmaxclamp(0, (int) r_brights_entity		.value, COLORSHADINGMAP_STYLES-1)][256]; ApplyBrights ();	}
void SetBrights_player		(void)	{	colorshadingmap_player		= &colorshadingmap[minmaxclamp(0, (int) r_brights_player		.value, COLORSHADINGMAP_STYLES-1)][256]; ApplyBrights ();	}
void SetBrights_viewmodel	(void)	{	colorshadingmap_viewmodel	= &colorshadingmap[minmaxclamp(0, (int) r_brights_viewmodel	.value, COLORSHADINGMAP_STYLES-1)][256]; ApplyBrights ();	}
void SetBrights_particle	(void)	{	colorshadingmap_particle	= &colorshadingmap[minmaxclamp(0, (int) r_brights_particle	.value, COLORSHADINGMAP_STYLES-1)][256]; ApplyBrights ();	}

/*
Id Software notes:

fullbright colors start at the top of the palette.
Range can be greater than 1 to allow overbright color tables.

the first map is all 0
the last (VID_GRADES-1) map is at range
*/
#if 0
void GrabColormap (byte *map, byte *map_wbrights, byte *tintmap, byte *tintmap_wbrights, float range, int factor)
#else
void GrabColormap (byte *map, byte *map_wbrights, float range, int factor)
#endif
{
	float
		frac
	,	frac1
		;
	int
		i
	,	l
	,	c
	,	r
	,	g
	,	b
	,	brights = minmaxclamp (1, (int) r_fullbright_colors.value, 256)
		;

	for (l=0 ; l<VID_GRADES ; l++) // shaded levels
	{
		if (factor > 0)
			frac = range - range * (float) ((l/factor)*factor) / (float) (VID_GRADES-1);
		else
			frac = range - range * (float) l / (float) (VID_GRADES-1);
		for (c=0 ; c<256 ; c++)
		{
			i = (l+1)*256+c;
			r = host_basepal[c*3  ];
			g = host_basepal[c*3+1];
			b = host_basepal[c*3+2];

			// tinting - begin
			#if 0
			frac1 = (0.5f + frac
			*	( (float) (r + g + b) / 3.0f)
				)
			*	(765.0f / (143.0f + 111.0f + 35.0f)) // compensação da intensidade dos valores RGB no menu tinting
		//	*	0.25f // darken
		//	*	0.5f + 0.25f // darken
				;
			// menu tinting
			if (c != TRANSPARENT_COLOR) // opaque
			{
				tintmap_wbrights[i] = tintmap[i] = BestColor (
					(143.0f / 255.0f) * frac1
				,	(111.0f / 255.0f) * frac1
				,	( 35.0f / 255.0f) * frac1
				,	0, 254);
			#endif
				// fullbrights
				if (!l) // only do this once for each color
				{
					frac1 = ( (float) (r + g + b) / 3.0f)
					*	(765.0f / (143.0f + 111.0f + 35.0f)) // compensação da intensidade dos valores RGB no menu tinting
				//	*	0.25f // darken
				//	*	0.5f + 0.25f // darken
					/	2.0f // darken
						;
					tintTable[c] = BestColor (
						(143.0f / 255.0f) * frac1
					,	(111.0f / 255.0f) * frac1
					,	( 35.0f / 255.0f) * frac1
					,	0, 254);
				//	map_wbrights[l*256+c] = c;//map[(int)((VID_GRADES-1.0)/range)*256+c];
				//	map_wbrights[l*256+c] = map[ (int) ( (VID_GRADES / range) - 1.0f) * 256 + c];
				//	Q_memcpy (translationTable, colorshadingmap_entity + ( ( (int) (64.0f / r_fullbright_range.value)) - 1) * 256, 256);
				}
			#if 0
				if (c >= 256 - brights) // apply tinted fullbrights to the tinted color shading map
					tintmap_wbrights[i] = tintTable[c];
			}
			else
				tintmap_wbrights[i] = tintmap[i] = tintTable[c] = TRANSPARENT_COLOR;
			#endif
			// tinting - end

			#if 0
			r = (int) (0.5 + frac * host_basepal[c*3  ]); // red
			g = (int) (0.5 + frac * host_basepal[c*3+1]); // green
			b = (int) (0.5 + frac * host_basepal[c*3+2]); // blue
			if (gl_polyblend.value)
				for (s=0 ; s<NUM_CSHIFTS ; s++)
				{
					#if 1
					float
						frac1 = (0.5f + frac * ((float) (
						r
					+	g
					+	b
						))) / 765.0f //3.0
					,	frac2 = (float) (cl.cshifts[s].percent) / 255.0f
						;
					int
						cs_r = (int) ( (float) (cl.cshifts[s].destcolor[0]) * frac2)
					,	cs_g = (int) ( (float) (cl.cshifts[s].destcolor[1]) * frac2)
					,	cs_b = (int) ( (float) (cl.cshifts[s].destcolor[2]) * frac2) // frac * frac não deve ser feito no maior valor
					,	offset = (max (cs_r, max (cs_g, cs_b)) - min (cs_r, min (cs_g, cs_b))) / -2
						;
				//	frac1 = frac1 * frac1;
					// true color tinting algorithm
					r = (int) ( (float)r * (1.0f - frac2)/* * (float)r / 255.0f*/) + cs_r * frac1;// * (1.0f - (float)r / 255.0f);
					g = (int) ( (float)g * (1.0f - frac2)/* * (float)g / 255.0f*/) + cs_g * frac1;// * (1.0f - (float)g / 255.0f);
					b = (int) ( (float)b * (1.0f - frac2)/* * (float)b / 255.0f*/) + cs_b * frac1;// * (1.0f - (float)b / 255.0f);
					#else
					int
						cs_r = cl.cshifts[s].destcolor[0]
					,	cs_g = cl.cshifts[s].destcolor[1]
					,	cs_b = cl.cshifts[s].destcolor[2]
					,	offset = (max (cs_r, max (cs_g, cs_b)) - min (cs_r, min (cs_g, cs_b))) / -2
						;
					float
						frac1 = (0.5 + frac * ((float) (
						r
					+	g
					+	b
						))) / 765.0;//3.0;
					offset = (max (r, max (g, b)) - min (r, min (g, b))) / -1;
					r += (cl.cshifts[s].percent * ( (cs_r)) + (int) ( (float) (r + offset) * (frac1))) >> 8;
					g += (cl.cshifts[s].percent * ( (cs_g)) + (int) ( (float) (g + offset) * (frac1))) >> 8;
					b += (cl.cshifts[s].percent * ( (cs_b)) + (int) ( (float) (b + offset) * (frac1))) >> 8;

					r += (cl.cshifts[s].percent * ( (cs_r)) + (int) ( (float) r * (frac1))) >> 8;
					g += (cl.cshifts[s].percent * ( (cs_g)) + (int) ( (float) g * (frac1))) >> 8;
					b += (cl.cshifts[s].percent * ( (cs_b)) + (int) ( (float) b * (frac1))) >> 8;

					r += (cl.cshifts[s].percent * ( (cs_r + offset)) + (int) ( (float) r * (frac1))) >> 8;
					g += (cl.cshifts[s].percent * ( (cs_g + offset)) + (int) ( (float) g * (frac1))) >> 8;
					b += (cl.cshifts[s].percent * ( (cs_b + offset)) + (int) ( (float) b * (frac1))) >> 8;

					r += (cl.cshifts[s].percent * ((int) ( (float) (cs_r + offset) * (frac1)) - r)) >> 8;
					g += (cl.cshifts[s].percent * ((int) ( (float) (cs_g + offset) * (frac1)) - g)) >> 8;
					b += (cl.cshifts[s].percent * ((int) ( (float) (cs_b + offset) * (frac1)) - b)) >> 8;

					r += (cl.cshifts[s].percent * (cs_r + offset - r/2)) >> 8;
					g += (cl.cshifts[s].percent * (cs_g + offset - g/2)) >> 8;
					b += (cl.cshifts[s].percent * (cs_b + offset - b/2)) >> 8;

					r += (cl.cshifts[s].percent * (cs_r + offset - r)) >> 8;
					g += (cl.cshifts[s].percent * (cs_g + offset - g)) >> 8;
					b += (cl.cshifts[s].percent * (cs_b + offset - b)) >> 8;

					r += (cl.cshifts[s].percent * (cs_r / 2 - r / 2)) >> 8;
					g += (cl.cshifts[s].percent * (cs_g / 2 - g / 2)) >> 8;
					b += (cl.cshifts[s].percent * (cs_b / 2 - b / 2)) >> 8;

					r += (cl.cshifts[s].percent * (cl.cshifts[s].destcolor[0] - r)) >> 8;
					g += (cl.cshifts[s].percent * (cl.cshifts[s].destcolor[1] - g)) >> 8;
					b += (cl.cshifts[s].percent * (cl.cshifts[s].destcolor[2] - b)) >> 8;
					#endif
				}
			#if 0
			else
			{
				float
					frac1 = (0.5f + frac * ((float) (
				//	frac1 = (0.75 + frac * 0.5f * ((float) (
					host_basepal[c*3  ]
				+	host_basepal[c*3+1]
				+	host_basepal[c*3+2]
					))) / 3.0f;
				//	))) / 765.0; //3.0
			//	frac1 = frac1 * frac1;

				#if 0 // lava
				r = 128 + frac1/2.0;
				g = (int) (/*host_basepal[c*3+1] **/ (int) (255.0/1.0 - frac1/1.0));
				b = (int) (/*host_basepal[c*3+1] **/ (int) (255.0/4.0 - frac1/4.0));
				#endif
				#if 0 // yellow/(gold?)
				r = (int) (/*host_basepal[c*3+1] **/ (int) (frac1));
				g = 255;
				b = 0;
				#endif
				#if 0 // lava again
				frac1 *= (765.0f / (255.0f + 80.0f + 0.0f)); // compensação da intensidade dos valores RGB
			//	frac1 = 0.25f + frac1 * 0.5f; // darken
			//	frac1 *= 0.25f; // darken
				r = (255.0f / 255.0f) * frac1;
				g = ( 80.0f / 255.0f) * frac1;
				b = (  0.0f / 255.0f) * frac1;
				#endif
				#if 1 // menu tinting
				frac1 *= (765.0f / (143.0f + 111.0f + 35.0f)); // compensação da intensidade dos valores RGB
			//	frac1 = 0.25f + frac1 * 0.5f; // darken
			//	frac1 *= 0.25f; // darken
				r = (143.0f / 255.0f) * frac1;
				g = (111.0f / 255.0f) * frac1;
				b = ( 35.0f / 255.0f) * frac1;
				#endif
				#if 0 // green
				frac1 *= 3.0f;//(765.0f / (0.0f + 255.0f + 0.0f)); // compensação da intensidade dos valores RGB
			//	frac1 = 0.25f + frac1 * 0.5f; // darken
			//	frac1 *= 0.25f; // darken
				r = 0.0f;//(  0.0f / 255.0f) * frac1;
				b = frac1;
				g = 0.0f;//(  0.0f / 255.0f) * frac1;
				#endif
			//	r = frac1 * 3.0f;
			//	g = 0;
			//	b = 0;
			//	r = (int) (0.5 + frac * host_basepal[c*3  ]); // red
			//	b = (int) (0.5 + frac * host_basepal[c*3+2]); // blue
			//	b = (int) (/*host_basepal[c*3+1] **/ (int) (127.5 - frac1/2.0));
			//	b += (cl.cshifts[s].percent*(cl.cshifts[s].destcolor[2]-b))>>8;
			}
			#endif
			map_wbrights[i] = map[i] = BestColor (r, g, b, 0, 254);
		//	map_wbrights[i] = 1*16 + map_wbrights[i]%16; // poor menu
		//	map_wbrights[i] = 14*16 + 15 - map_wbrights[i]%16; // lava!
			#else
			#define fraq frac//*frac*frac
			// mankrip - end

			// Id Software note: 254 instead of 255 because 255 is the transparent color,
			// and we don't want anything remapping to that
			map_wbrights[i] = map[i] = BestColor (
				(int) (0.5f + fraq * (float) r) // red
			,	(int) (0.5f + fraq * (float) g) // green
			,	(int) (0.5f + fraq * (float) b) // blue
			,	0, 254);

			// Id Software note: fullbrights always stay the same
			if (c >= 256 - brights)
				map_wbrights[i] = c;
			#endif
		}
	}
	/// fill the padding for dithering
	// full brightness
	Q_memcpy (&map[0], &map[256], 256);
	// full darkness
	Q_memcpy (&map[ (1 + VID_GRADES) << 8], &map[ (0 + VID_GRADES) << 8], 256);

	// footer
	map[COLORSHADINGMAP_SIZE] = 0;
	map_wbrights[COLORSHADINGMAP_SIZE] = brights;
	#if 0
	tintmap[COLORSHADINGMAP_SIZE] = 0;
	tintmap_wbrights[COLORSHADINGMAP_SIZE] = brights;
	#endif
}



void MakeColorshadingmap (void)
{
	GrabColormap (
		colorshadingmap[WITHOUT_BRIGHTS]
	,	colorshadingmap[WITH_BRIGHTS]
	#if 0
	,	colorshadingmap[TINTED_WITHOUT_BRIGHTS]
	,	colorshadingmap[TINTED_WITH_BRIGHTS]
	#endif
	,	minmaxclamp (0.0f, r_fullbright_range.value, 255.0f)
	,	0); //2.0f;

	GrabColormap (
		colorshadingmap[CEL_WITHOUT_BRIGHTS]
	,	colorshadingmap[CEL_WITH_BRIGHTS]
	#if 0
	,	colorshadingmap[TINTED_CEL_WITHOUT_BRIGHTS]
	,	colorshadingmap[TINTED_CEL_WITH_BRIGHTS]
	#endif
	,	minmaxclamp (0.0f, r_fullbright_cel_range.value, 255.0f)
	,	(int) r_fullbright_cel_factor.value);

	GrabColormap (
		colorshadingmap[METAL_WITHOUT_BRIGHTS]
	,	colorshadingmap[METAL_WITH_BRIGHTS]
	#if 0
	,	colorshadingmap[TINTED_METAL_WITHOUT_BRIGHTS]
	,	colorshadingmap[TINTED_METAL_WITH_BRIGHTS]
	#endif
	,	minmaxclamp (0.0f, r_fullbright_range.value, 255.0f)
	,	16.0F);

	SetBrights_player ();
	vid.recalc_refdef = true;
	D_FlushCaches ();
}



/*
=============================================================================

COLOR BLENDING

=============================================================================
*/

// percorrendo um percentual por meio de índices
// pegar um valor de intensidade,
// extrair a quantidade de itens para enumeração,
// extrair o valor de cada casa da enumeração,
// (in/de)crementar o índice de casa atual
// e multiplicar este índice pelo valor de cada casa para obter a intensidade resultante
// e depois converter para intensidade de novo

// enumeração 0 a 6 (7 itens) (conteúdo)
// 1/6 * 0.5 // se menor que isso, retorna o índice zero
// 00 a 08
int indexForPercentage (float min, float max, int indexes, float percentage)
{
	#if 1
	if (abs (indexes) < 2)
		return indexes - indexes;
	else
	{
		float
			delta = max - min
		,	slice = (delta / (float) (indexes - 1))
			;
		int i = 0;
		do
		{
			if (percentage < (slice * (0.5f + (float) i)))
				break;
		} while (++i < indexes);
		return i;
	}
	#else
//	return ( (max - min) / (float) (indexes - 1)) * (0.5f + (float) index); // wrong
	#endif
}

// progresso 0 a 1 (fronteiras)
// 1/7 * 1 // percentual do índice 1 (recebo este valor pelo índice 1)
// 00 a 14
float percentageForIndex (float min, float max, int indexes, int index)
{
	#if 0
	float
		delta = max - min
	,	slice = (delta / (float) (indexes))
		;
	return slice * (float) index;
	#else
	return ( (max - min) / (float) (indexes)) * (float) index;
	#endif
}

// progresso 0 a 1 (fronteiras)
// 1/6 * 1 // percentual do índice 1 (recebo este valor pelo índice 1)
// 00 a 14
float valueForIndex (float min, float max, int indexes, int index)
{
	#if 0
	float
		delta = max - min
	,	slice = (delta / (float) (indexes - 1))
		;
	return slice * (float) index;
	#else
	return ( (max - min) / (float) (indexes - 1)) * (float) index;
	#endif
}

#if 0
static void	(*blendfunc[4])(void) = {
	blendfunc_0,
	blendfunc_1,
	blendfunc_2,
	blendfunc_3
};
#endif

#if 0
typedef struct r_colorblendingmode_s
{
	qboolean
		opaque_when_full // use regular non-blended rendering when alpha == 1.0f
//	,	cache_all // caches all maps if set; if not, caches only the last selected one
//	,	invert_cache // use the cache in reverse order for alpha > 0.5
	,	write_depth // should be somewhere else
		;
	int
		/*
		map zero (fully transparent) and the last map (fully opaque), plus any number of intervals.
		< 2 means there will be no intermediate values between 0.0 and 1.0,	only fully transparent and fully opaque,
		so the minimum should be 1.
		*/
		last_level
	,	current_level
		;
	byte
		**alpha // index zero should be fully transparent, so there's no need to allocate a map for it
		;
} r_colorblendingmode_t;

r_colorblendingmode_t colorblendingmodes[COLORBLENDINGMAP_MODES] =
{
	{ true, false,  true, 1, 0, NULL}	// BLEND_SOLID
,	{ true, false,  true, 1, 0, NULL}	// BLEND_STIPPLE
,	{ true, false,  true, 1, 0, NULL}	// BLEND_BLEND

,	{ true,  true,  true, 1, 0, NULL}	// BLEND_WEIGHTEDMEAN
,	{ true, false,  true, 1, 0, NULL}	// BLEND_ADDITIVE
#if 0
,	{ true, false,  true, 1, 0, NULL}	// BLEND_LIQUID
,	{ true, false,  true, 1, 0, NULL}	// BLEND_WATER
,	{ true, false,  true, 1, 0, NULL}	// BLEND_SLIME
,	{ true, false,  true, 1, 0, NULL}	// BLEND_LAVA
,	{ true, false,  true, 1, 0, NULL}	// BLEND_PORTAL
,	{ true, false,  true, 1, 0, NULL}	// BLEND_GLASS
,	{ true, false,  true, 1, 0, NULL}	// BLEND_SKY
,	{ true, false,  true, 1, 0, NULL}	// BLEND_SMOKE
#endif
}
		;

typedef struct r_colorblending_s
{
	int
		current_mode
		;
	byte
		*currentmap
		;
	r_colorblendingmode_t
		mode[COLORBLENDINGMAP_MODES]
		;
} r_colorblending_t;

r_colorblending_t colorblending;

byte *mapForAlpha (int blendmode, float alpha)
{
	int i = indexForPercentage (0.0f, 1.0f, colorblending.mode[blendmode].num_maps, minmaxclamp (0.0f, alpha, 1.0f));
	if (i >= 0 && i < colorblending.mode[blendmode].num_maps)
		return colorblending.mode[blendmode].alpha[i];
	else
		return NULL;
}

// progresso	0 a 1 (fronteiras):	1/7 * 1		= 14
// enumeração	0 a 6 (7 itens):	1/6 * 0.5	= 08
byte D_DrawPixel_00				(byte under, byte over)	{	return under;	}
// progresso	0 a 1 (fronteiras):	1/7 * 2		= 29
// enumeração	0 a 6 (7 itens):	1/6 * 1.5	= 25
byte D_DrawPixel_17				(byte under, byte over)	{	return (over == 255) ? under : colorblendingmap[BLEND_WEIGHTEDMEAN17][over + under*256];	}
// progresso	0 a 1 (fronteiras):	1/7 * 3		= 43
// enumeração	0 a 6 (7 itens):	1/6 * 2.5	= 42
byte D_DrawPixel_33				(byte under, byte over)	{	return (over == 255) ? under : colorblendingmap[BLEND_WEIGHTEDMEAN33][over + under*256];	}
// progresso	0 a 1 (fronteiras):	1/7 * 4		= 57
// enumeração	0 a 6 (7 itens):	1/6 * 3.5	= 58
byte D_DrawPixel_50				(byte under, byte over)	{	return (over == 255) ? under : colorblendingmap[BLEND_WEIGHTEDMEAN50][over + under*256];	}
// progresso	0 a 1 (fronteiras):	1/7 * 5		= 71
// enumeração	0 a 6 (7 itens):	1/6 * 4.5	= 75
byte D_DrawPixel_67				(byte under, byte over)	{	return (over == 255) ? under : colorblendingmap[BLEND_WEIGHTEDMEAN33][under + over*256];	}
// progresso	0 a 1 (fronteiras):	1/7 * 6		= 86
// enumeração	0 a 6 (7 itens):	1/6 * 5.5	= 92
byte D_DrawPixel_83				(byte under, byte over)	{	return (over == 255) ? under : colorblendingmap[BLEND_WEIGHTEDMEAN17][under + over*256];	}
// progresso	0 a 1 (fronteiras):	1/7 * 7		= 100
// enumeração	0 a 6 (7 itens):	1/6 * 6.5	= 100
byte D_DrawPixel_100			(byte under, byte over)	{	return (over == 255) ? under : over;	}

byte D_DrawPixel_Add			(byte under, byte over)	{	return (over == 255) ? under : colorblendingmap[BLEND_ADDITIVE		][over + under*256];	}
byte D_DrawPixel_Max			(byte under, byte over)	{	return (over == 255) ? under : colorblendingmap[BLEND_MAX			][over + under*256];	}
byte D_DrawPixel_ReverseAlpha	(byte under, byte over)	{	return (over == 255) ? under : colorblendingmap[BLEND_MAX			][under + over*256];	}
byte D_DrawPixel_Sky			(byte under, byte over)	{	return (over == 255) ? under : colorblendingmap[BLEND_SMOKE			][over + under*256];	}

byte D_DrawPixel_Opaque			(byte under, byte over)	{	return (over == 255) ? under : over;	}
byte D_DrawPixel_Transparent	(byte under, byte over)	{	return under;	}
#endif

byte	*colorblendingmap;
byte	**colorblendingmode;

void MakeColorblendingmaps (void)
{
	int
		l = 0
	,	c
//	,	brights = minmaxclamp (0, 256 - (int) r_fullbright_colors.value, 256);
		;

	#if 0
	if (!palette_initialized)
	{
		colorblending.mode[BLEND_SOLID].opaque_when_full = true;
		colorblending.mode[BLEND_SOLID].cache_all = false;
		colorblending.mode[BLEND_SOLID].write_depth = true;
		colorblending.mode[BLEND_SOLID].num_maps = 0;
	}
	#endif

//	for ( ; m < COLORBLENDINGMAP_MODES ; m++)
	for (l=0 ; l<256 ; l++)
	{
		for (c=0 ; c<256 ; c++)
		{
			// note: 254 instead of 255 because 255 is the transparent color,
			// and we don't want anything remapping to that

			float
				frac1 = (float) (
				host_basepal[c*3  ]
			+	host_basepal[c*3+1]
			+	host_basepal[c*3+2]
				)
			,	frac2 = (float) (
				host_basepal[l*3  ]
			+	host_basepal[l*3+1]
			+	host_basepal[l*3+2]
				);
			#if 0
			frac2 = frac2 / (frac1+frac2);
		//	frac2 = frac2 * 0.3;
			frac1 = 1.0 - frac2;
			#else
			#if 1
		//	frac1 = frac1 / 765.0;
			frac1 = frac1 / (frac1+frac2);
			frac1 = frac1 * 0.7;
			#else
			frac1 = frac1 / (frac1+frac2);
		//	frac1 = frac1 * 0.5;
			#endif
		//	frac1 = frac1*frac1;

			frac2 = 1.0 - frac1;
			#endif

			if (c == 255)
			{
				colorblendingmode[BLEND_WEIGHTEDMEAN	][l*256+c] = l;
				colorblendingmode[BLEND_ADDITIVE		][l*256+c] = l;
				colorblendingmode[BLEND_SMOKE			][l*256+c] = l;
				continue;
			}
			else if (l == 255)
			{
				colorblendingmode[BLEND_WEIGHTEDMEAN	][l*256+c] = c;
				colorblendingmode[BLEND_ADDITIVE		][l*256+c] = c;
				colorblendingmode[BLEND_SMOKE			][l*256+c] = c;
				continue;
			}

			colorblendingmode[BLEND_WEIGHTEDMEAN	][l*256+c] = BestColor (
				(int) (0.5f + host_basepal[c*3  ] / 3.0f + host_basepal[l*3  ] / 1.5f) // red
			,	(int) (0.5f + host_basepal[c*3+1] / 3.0f + host_basepal[l*3+1] / 1.5f) // green
			,	(int) (0.5f + host_basepal[c*3+2] / 3.0f + host_basepal[l*3+2] / 1.5f) // blue
			,	0, 254);

			colorblendingmode[BLEND_ADDITIVE			][l*256+c] = BestColor (
				minmaxclamp (0, (host_basepal[c*3  ] + host_basepal[l*3  ]), 255) // red
			,	minmaxclamp (0, (host_basepal[c*3+1] + host_basepal[l*3+1]), 255) // green
			,	minmaxclamp (0, (host_basepal[c*3+2] + host_basepal[l*3+2]), 255) // blue
			,	0, 254);

			colorblendingmode[BLEND_SMOKE			][l*256+c] = BestColor (
				(int) ( (float) host_basepal[c*3  ]*frac1 + (float) host_basepal[l*3  ]*frac2) // red
			,	(int) ( (float) host_basepal[c*3+1]*frac1 + (float) host_basepal[l*3+1]*frac2) // green
			,	(int) ( (float) host_basepal[c*3+2]*frac1 + (float) host_basepal[l*3+2]*frac2) // blue
			,	0, 254);

			#if 0
			colorblendingmode[BLEND_SMOKE			][l*256+c] = (c < brights)
			?	BestColor (
				(int) ( (float) host_basepal[c*3  ]*frac1 + (float) host_basepal[l*3  ]*frac2) // red
			,	(int) ( (float) host_basepal[c*3+1]*frac1 + (float) host_basepal[l*3+1]*frac2) // green
			,	(int) ( (float) host_basepal[c*3+2]*frac1 + (float) host_basepal[l*3+2]*frac2) // blue
			,	0, 254)
			:	BestColor (
				minmaxclamp (0, (host_basepal[c*3  ] + host_basepal[l*3  ]), 255) // red
			,	minmaxclamp (0, (host_basepal[c*3+1] + host_basepal[l*3+1]), 255) // green
			,	minmaxclamp (0, (host_basepal[c*3+2] + host_basepal[l*3+2]), 255) // blue
			,	0, 254);

			colorblendingmap[BLEND_LIQUID			][l*256+c] = BestColor (
				(int) ( (float) host_basepal[c*3  ]*(frac1 / (frac1+frac2))*0.7f + (float) host_basepal[l*3  ]*(1.0f - (frac1 / (frac1+frac2))*0.7f)) // red
			,	(int) ( (float) host_basepal[c*3+1]*(frac1 / (frac1+frac2))*0.7f + (float) host_basepal[l*3+1]*(1.0f - (frac1 / (frac1+frac2))*0.7f)) // green
			,	(int) ( (float) host_basepal[c*3+2]*(frac1 / (frac1+frac2))*0.7f + (float) host_basepal[l*3+2]*(1.0f - (frac1 / (frac1+frac2))*0.7f)) // blue
			,	0, 254);
			colorblendingmap[BLEND_SLIME			][l*256+c] = BestColor (
				(int) ( (float) host_basepal[c*3  ]*(frac1 / (frac1+frac2)) + (float) host_basepal[l*3  ]*(frac2 / (frac1+frac2))*0.3f) // red
			,	(int) ( (float) host_basepal[c*3+1]*(frac1 / (frac1+frac2)) + (float) host_basepal[l*3+1]*(frac2 / (frac1+frac2))*0.3f) // green
			,	(int) ( (float) host_basepal[c*3+2]*(frac1 / (frac1+frac2)) + (float) host_basepal[l*3+2]*(frac2 / (frac1+frac2))*0.3f) // blue
			,	0, 254);
			colorblendingmap[BLEND_GLASS			][l*256+c] = BestColor (
				(int) ( (float) host_basepal[c*3  ]*(frac2 / (frac1+frac2)) + (float) host_basepal[l*3  ]*(frac1 / (frac1+frac2))) // red
			,	(int) ( (float) host_basepal[c*3+1]*(frac2 / (frac1+frac2)) + (float) host_basepal[l*3+1]*(frac1 / (frac1+frac2))) // green
			,	(int) ( (float) host_basepal[c*3+2]*(frac2 / (frac1+frac2)) + (float) host_basepal[l*3+2]*(frac1 / (frac1+frac2))) // blue
			,	0, 254);
			colorblendingmap[BLEND_WEIGHTEDMEAN17	][l*256+c] = BestColor (
				(int) (0.5f + host_basepal[c*3  ] / 6.0f + host_basepal[l*3  ] / 1.2f) // red
			,	(int) (0.5f + host_basepal[c*3+1] / 6.0f + host_basepal[l*3+1] / 1.2f) // green
			,	(int) (0.5f + host_basepal[c*3+2] / 6.0f + host_basepal[l*3+2] / 1.2f) // blue
			,	0, 254);
			colorblendingmap[BLEND_WEIGHTEDMEAN33	][l*256+c] = BestColor (
				(int) (0.5f + host_basepal[c*3  ] / 3.0f + host_basepal[l*3  ] / 1.5f) // red
			,	(int) (0.5f + host_basepal[c*3+1] / 3.0f + host_basepal[l*3+1] / 1.5f) // green
			,	(int) (0.5f + host_basepal[c*3+2] / 3.0f + host_basepal[l*3+2] / 1.5f) // blue
			,	0, 254);
			colorblendingmap[BLEND_WEIGHTEDMEAN50	][l*256+c] = BestColor (
				(int) (0.5f + (float) (host_basepal[c*3  ] + host_basepal[l*3  ]) * 0.5f) // red
			,	(int) (0.5f + (float) (host_basepal[c*3+1] + host_basepal[l*3+1]) * 0.5f) // green
			,	(int) (0.5f + (float) (host_basepal[c*3+2] + host_basepal[l*3+2]) * 0.5f) // blue
			,	0, 254);
			colorblendingmap[BLEND_ADDITIVE			][l*256+c] = BestColor (
				minmaxclamp (0, (host_basepal[c*3  ] + host_basepal[l*3  ]), 255) // red
			,	minmaxclamp (0, (host_basepal[c*3+1] + host_basepal[l*3+1]), 255) // green
			,	minmaxclamp (0, (host_basepal[c*3+2] + host_basepal[l*3+2]), 255) // blue
			,	0, 254);
			colorblendingmap[BLEND_MAX				][l*256+c] = BestColor (
				(int) (max(host_basepal[c*3  ], host_basepal[l*3  ])) // red
			,	(int) (max(host_basepal[c*3+1], host_basepal[l*3+1])) // green
			,	(int) (max(host_basepal[c*3+2], host_basepal[l*3+2])) // blue
			,	0, 254);
			colorblendingmap[BLEND_BINARY_OR		][l*256+c] = BestColor (
				(int) (host_basepal[c*3  ] | host_basepal[l*3  ]) // red
			,	(int) (host_basepal[c*3+1] | host_basepal[l*3+1]) // green
			,	(int) (host_basepal[c*3+2] | host_basepal[l*3+2]) // blue
			,	0, 254);
			#endif
		}
	}
}



/*
=============================================================================

SCREEN COLOR SHIFTING

=============================================================================
*/

cvar_t		gl_cshiftpercent	= {"gl_cshiftpercent"	, "100", false};
cvar_t		gl_polyblend		= {"gl_polyblend"		,	"1", false};
int			gl_polyblend_old;

// bonus
cshift_t	cshift_bonus	= { {215, 186,  69},  50 };
// contents
cshift_t	cshift_empty	= { {130,  80,  50},   0 };
cshift_t	cshift_water	= { {130,  80,  50}, 128 };
cshift_t	cshift_slime	= { {  0,  25,   5}, 150 };
cshift_t	cshift_lava		= { {255,  80,   0}, 150 };
// powerup
cshift_t	cshift_none		= { {  0,   0,   0},   0 };
cshift_t	cshift_quad		= { {  0,   0, 255},  30 };
cshift_t	cshift_suit		= { {  0, 255,   0},  20 };
cshift_t	cshift_invis	= { {100, 100, 100}, 100 };
cshift_t	cshift_invul	= { {255, 255,   0},  30 };
// damage
cshift_t	cshift_armor	= { {200, 100, 100},   0 };
cshift_t	cshift_lowarmor	= { {220,  50,  50},   0 };
cshift_t	cshift_noarmor	= { {255,   0,   0},   0 };

#define cshift_set(c,r,g,b,a) (c).destcolor[0]=(r);(c).destcolor[1]=(g);(c).destcolor[2]=(b);(c).percent=(a);

void V_cshift_f (void)
{
	cshift_set (
		cshift_empty
	,	atoi(Cmd_Argv(1))
	,	atoi(Cmd_Argv(2))
	,	atoi(Cmd_Argv(3))
	,	atoi(Cmd_Argv(4))
		);
}



//================================================================
//================== Id Software GPL code begin ==================
//================================================================
void V_BonusFlash_f (void) // When you run over an item, the server sends this command
{
	#ifndef _arch_dreamcast
	cshift_bonus.percent = 50;
	cl.cshifts[CSHIFT_BONUS] = cshift_bonus; // TO DO: should be set at initialization
	#endif

#if 0
	{
		int             i;
		extern char	com_cmdline[256];
		#ifdef _arch_dreamcast
		extern char	*modmenu_argv[MAX_NUM_ARGVS]; // mankrip - edited
		for (i=0 ; i<com_argc ; i++)
			Con_Printf ("modmenu_argv %i \"%s\"\n", i, modmenu_argv[i]);
		Con_Printf ("\n");
		#endif

		Con_Printf ("com_cmdline %s\n", com_cmdline);
		Con_Printf ("com_argc %i\n", com_argc);
		for (i=0 ; i<com_argc ; i++)
			Con_Printf ("com_argv %i \"%s\"\n", i, com_argv[i]);
	}
#endif
#if 0
	{
	extern int
		scr_2d_align_h // left, center, right
	,	scr_2d_align_v // top, center, bottom
	,	scr_2d_scale_h
	,	scr_2d_scale_v
		// non-scaled:
	,	scr_2d_left
	,	scr_2d_right
	,	scr_2d_top
	,	scr_2d_bottom
	,	scr_2d_width
	,	scr_2d_height
	,	scr_2d_offset_x
	,	scr_2d_offset_y
	,	scr_2d_padding_x
	,	scr_2d_padding_y
		;
	Con_Printf ("\
scr_2d_scale_h %i\n\
scr_2d_scale_v %i\n\
scr_2d_left   %i\n\
scr_2d_right  %i\n\
scr_2d_top    %i\n\
scr_2d_bottom %i\n\
scr_2d_width  %i\n\
scr_2d_height %i\n\
scr_2d_padding_x %i\n\
scr_2d_padding_y %i\n\
vid.width  %i\n\
vid.height %i\n\
"
	,	scr_2d_scale_h
	,	scr_2d_scale_v
	,	scr_2d_left
	,	scr_2d_right
	,	scr_2d_top
	,	scr_2d_bottom
	,	scr_2d_width
	,	scr_2d_height
	,	scr_2d_padding_x
	,	scr_2d_padding_y
	,	vid.width
	,	vid.height
	);
	}
#endif
}

void V_SetContentsColor (int contents)
{
	switch (contents)
	{
	case CONTENTS_EMPTY:
	case CONTENTS_SOLID:
		cl.cshifts[CSHIFT_CONTENTS] = cshift_empty;
		break;
	case CONTENTS_LAVA:
		cl.cshifts[CSHIFT_CONTENTS] = cshift_lava;
		break;
	case CONTENTS_SLIME:
		cl.cshifts[CSHIFT_CONTENTS] = cshift_slime;
		break;
	default:
		cl.cshifts[CSHIFT_CONTENTS] = cshift_water;
	}
}

cshift_t V_CalcPowerupCshift (void)
{
	if (cl.items & IT_QUAD)				return cshift_quad;
	if (cl.items & IT_SUIT)				return cshift_suit;
	if (cl.items & IT_INVISIBILITY)		return cshift_invis;
	if (cl.items & IT_INVULNERABILITY)	return cshift_invul;
	return cshift_none;
}

void V_CalcDamageCshift (int armor, int blood, float count)
{
	#ifndef _arch_dreamcast
	if (armor > blood)
		cl.cshifts[CSHIFT_DAMAGE] = cshift_armor;
	else if (armor)
		cl.cshifts[CSHIFT_DAMAGE] = cshift_lowarmor;
	else
		cl.cshifts[CSHIFT_DAMAGE] = cshift_noarmor;
	cl.cshifts[CSHIFT_DAMAGE].percent = minmaxclamp (0, cl.cshifts[CSHIFT_DAMAGE].percent + 3*count, 150);
	#endif
}

#if 0
void WarpPalette (void)
{
	int		i,j;
	byte	newpalette[768];
	int		basecolor[3];

	basecolor[0] = 130;
	basecolor[1] = 80;
	basecolor[2] = 50;

// pull the colors halfway to bright brown
	for (i=0 ; i<256 ; i++)
	{
		for (j=0 ; j<3 ; j++)
		{
			newpalette[i*3+j] = (host_basepal[i*3+j] + basecolor[j])/2;
		}
	}

	VID_ShiftPalette (newpalette);
}
#endif

#ifdef	GLQUAKE
byte		ramps[3][256];
float		v_blend[4];		// rgba 0.0 - 1.0

void V_CalcBlend (void)
{
	float	r, g, b, a, a2;
	int		j;

	r = 0;
	g = 0;
	b = 0;
	a = 0;

	for (j=0 ; j<NUM_CSHIFTS ; j++)
	{
		if (!gl_cshiftpercent.value)
			continue;

		a2 = ((cl.cshifts[j].percent * gl_cshiftpercent.value) / 100.0) / 255.0;

//		a2 = cl.cshifts[j].percent/255.0;
		if (!a2)
			continue;
		a = a + a2*(1-a);
//Con_Printf ("j:%i a:%f\n", j, a);
		a2 = a2/a;
		r = r*(1-a2) + cl.cshifts[j].destcolor[0]*a2;
		g = g*(1-a2) + cl.cshifts[j].destcolor[1]*a2;
		b = b*(1-a2) + cl.cshifts[j].destcolor[2]*a2;
	}

	v_blend[0] = r/255.0;
	v_blend[1] = g/255.0;
	v_blend[2] = b/255.0;
	v_blend[3] = a;
	if (v_blend[3] > 1)
		v_blend[3] = 1;
	if (v_blend[3] < 0)
		v_blend[3] = 0;
}
#endif

void V_UpdatePalette (void)
{
	int		i, j;
	qboolean	new;
	byte	*basepal, *newpal;
	#ifdef	GLQUAKE
	float	r,g,b,a;
	int		ir, ig, ib;
	#else
	int		r,g,b;
	#endif
	qboolean force;

	cl.cshifts[CSHIFT_POWERUP] = V_CalcPowerupCshift ();

	new = false;

	for (i=0 ; i<NUM_CSHIFTS ; i++)
	{
		if (cl.cshifts[i].percent != cl.prev_cshifts[i].percent)
		{
			new = true;
			cl.prev_cshifts[i].percent = cl.cshifts[i].percent;
		}
		for (j=0 ; j<3 ; j++)
			if (cl.cshifts[i].destcolor[j] != cl.prev_cshifts[i].destcolor[j])
			{
				new = true;
				cl.prev_cshifts[i].destcolor[j] = cl.cshifts[i].destcolor[j];
			}
	}

	// drop the damage value
	cl.cshifts[CSHIFT_DAMAGE].percent -= host_frametime*150;
	if (cl.cshifts[CSHIFT_DAMAGE].percent <= 0)
		cl.cshifts[CSHIFT_DAMAGE].percent = 0;

	// drop the bonus value
	cl.cshifts[CSHIFT_BONUS].percent -= host_frametime*100;
	if (cl.cshifts[CSHIFT_BONUS].percent <= 0)
		cl.cshifts[CSHIFT_BONUS].percent = 0;

	force = V_CheckGamma ();
	// mankrip - gl_polyblend begin
	if (!force) // no gamma changes
	{
		if (gl_polyblend_old == gl_polyblend.value) // gl_polyblend hasn't changed
		{
		//	if (!gl_polyblend_old || !new) // no pallete changes allowed, or no pallette changes to do
			if (!gl_polyblend_old) // no pallete changes allowed
				return;
			else if (!new) // no pallette changes
				return;
		}
	}
	gl_polyblend_old = gl_polyblend.value;
	// mankrip - gl_polyblend end
#if 0
	MakeColorshadingmap ();
#else

	#ifdef	GLQUAKE
	V_CalcBlend ();

	a = v_blend[3];
	r = 255*v_blend[0]*a;
	g = 255*v_blend[1]*a;
	b = 255*v_blend[2]*a;

	a = 1-a;
	for (i=0 ; i<256 ; i++)
	{
		ir = i*a + r;
		ig = i*a + g;
		ib = i*a + b;
		if (ir > 255)
			ir = 255;
		if (ig > 255)
			ig = 255;
		if (ib > 255)
			ib = 255;

		ramps[0][i] = gammatable[ir];
		ramps[1][i] = gammatable[ig];
		ramps[2][i] = gammatable[ib];
	}
	#endif

	basepal = host_basepal;
	newpal = shifted_basepal;

	#ifdef	GLQUAKE
	for (i=0 ; i<256 ; i++)
	{
		ir = basepal[0];
		ig = basepal[1];
		ib = basepal[2];
		basepal += 3;

		newpal[0] = ramps[0][ir];
		newpal[1] = ramps[1][ig];
		newpal[2] = ramps[2][ib];
		newpal += 3;
	}
	#else
	for (i=0 ; i<256 ; i++)
	{
		r = basepal[0];
		g = basepal[1];
		b = basepal[2];
		basepal += 3;

		if (gl_polyblend.value != 0) // mankrip - gl_polyblend
		for (j=0 ; j<NUM_CSHIFTS ; j++)
		{
			r += (cl.cshifts[j].percent*(cl.cshifts[j].destcolor[0]-r))>>8;
			g += (cl.cshifts[j].percent*(cl.cshifts[j].destcolor[1]-g))>>8;
			b += (cl.cshifts[j].percent*(cl.cshifts[j].destcolor[2]-b))>>8;
		}

		newpal[0] = gammatable[r];
		newpal[1] = gammatable[g];
		newpal[2] = gammatable[b];
		newpal += 3;
	}
	#endif
	VID_ShiftPalette (shifted_basepal);
#endif
}
//================================================================
//=================== Id Software GPL code end ===================
//================================================================



/*
=============================================================================

INITIALIZATION

=============================================================================
*/



void Palette_Init (void)
{
	loadedfile_t	*fileinfo;	// 2001-09-12 Returning information about loaded file by Maddes
	int
		i
		;

	currentTable = identityTable = (byte *) malloc (sizeof (byte) * 256);
	tintTable					 = (byte *) malloc (sizeof (byte) * 256);
	translationTable			 = (byte *) malloc (sizeof (byte) * 256);

	for (i = 0 ; i < 256 ; i++)
		identityTable[i] = (byte) i;

	Cmd_AddCommand ("v_cshift"	, V_cshift_f);
	Cmd_AddCommand ("bf"		, V_BonusFlash_f);
	Cvar_RegisterVariableWithCallback (&r_fullbright_colors		, &MakeColorshadingmap);
	Cvar_RegisterVariableWithCallback (&r_fullbright_range		, &MakeColorshadingmap);
	Cvar_RegisterVariableWithCallback (&r_fullbright_cel_range	, &MakeColorshadingmap);
	Cvar_RegisterVariableWithCallback (&r_fullbright_cel_factor	, &MakeColorshadingmap);

	Cvar_RegisterVariableWithCallback (&r_brights_world		, &SetBrights_world		);
	Cvar_RegisterVariableWithCallback (&r_brights_staticent	, &SetBrights_staticent	);
	Cvar_RegisterVariableWithCallback (&r_brights_beam		, &SetBrights_beam		);
	Cvar_RegisterVariableWithCallback (&r_brights_entity	, &SetBrights_entity	);
	Cvar_RegisterVariableWithCallback (&r_brights_player	, &SetBrights_player	);
	Cvar_RegisterVariableWithCallback (&r_brights_viewmodel	, &SetBrights_viewmodel	);
	Cvar_RegisterVariableWithCallback (&r_brights_particle	, &SetBrights_particle	);

	Cvar_RegisterVariable (&gl_cshiftpercent);
	Cvar_RegisterVariable (&gl_polyblend);
	Cvar_RegisterVariable (&v_gamma);

	// 2001-09-12 Returning information about loaded file by Maddes  start
	fileinfo = COM_LoadHunkFile ("gfx/palette.lmp");
	if (!fileinfo)
	// 2001-09-12 Returning information about loaded file by Maddes  end
		Sys_Error ("Couldn't load gfx/palette.lmp");
	host_basepal = fileinfo->data;	// 2001-09-12 Returning information about loaded file by Maddes
	shifted_basepal = (byte *) malloc (sizeof (byte) * fileinfo->filelen);
	RemapBackwards_Palette (host_basepal);

	#if 0
	colorshadingmap = (byte **) malloc (sizeof (byte *) * ALL_COLORSHADINGMAP_STYLES);
	for (i = 0 ; i < ALL_COLORSHADINGMAP_STYLES ; i++)
	#else
	colorshadingmap = (byte **) malloc (sizeof (byte *) * COLORSHADINGMAP_STYLES);
	for (i = 0 ; i < COLORSHADINGMAP_STYLES ; i++)
	#endif
		colorshadingmap[i] = (byte *) malloc (sizeof (byte) * COLORSHADINGMAP_SIZE);
	MakeColorshadingmap		();
	SetBrights_world		();
	SetBrights_staticent	();
	SetBrights_beam			();
	SetBrights_entity		();
	SetBrights_player		();
	SetBrights_viewmodel	();
	SetBrights_particle		();

	colorblendingmode = (byte **) malloc (sizeof (byte *) * COLORBLENDING_MAPS);
	for (i = 0 ; i < COLORBLENDING_MAPS ; i++)
		colorblendingmode[i] = (byte *) malloc (sizeof (byte) * COLORBLENDINGMAP_SIZE);
	MakeColorblendingmaps	();
	colorblendingmap = colorblendingmode[BLEND_WEIGHTEDMEAN]; // default

	BuildGammaTable (1.0);	// no gamma yet
	palette_initialized = true;
}
