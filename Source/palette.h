// !!! must be kept the same as in quakeasm.h !!!
#define TRANSPARENT_COLOR	0xFF

extern void
	* acolormap	// FIXME: should go away
	;
extern byte		*mdlcolormap;	// mankrip

typedef	byte	color8;	// color value index
typedef	byte	color16[2];	// 16-bit RGB color value
typedef	byte	color24[3];	// 24-bit RGB color value
typedef	byte	color32[4];	// 32-bit RGBA color value

extern byte
	* host_basepal
,	* shifted_basepal
,	* currentTable
,	* identityTable
,	* tintTable
,	* translationTable
	;
extern void
	RemapBackwards_Palette	(byte * p)
,	RemapBackwards_Pixels	(byte * i, int size)
,	RemapColorKey_Pixels	(byte * i, int size)
,	BuildColorTranslationTable (int top, int bottom, byte *source, byte *dest)
	;

#define		COLORSHADINGMAP_SIZE ( (1 + VID_GRADES + 1) * 256 + 1)
extern void
	* colormap
	;

#define		WITHOUT_BRIGHTS			0
#define		WITH_BRIGHTS			1
#define		CEL_WITHOUT_BRIGHTS		2 // EF_CELSHADING
#define		CEL_WITH_BRIGHTS		3 // EF_CELSHADING
#define		METAL_WITHOUT_BRIGHTS	4 // EF_REFLECTIVE
#define		METAL_WITH_BRIGHTS		5 // EF_REFLECTIVE
#define		COLORSHADINGMAP_STYLES	6 // should not include menu tinting maps
// COLORSHADINGMAP_STYLES doesn't count as a map, so restart counting from the index before it
#define		TINTED_WITHOUT_BRIGHTS			6
#define		TINTED_WITH_BRIGHTS				7
#define		TINTED_CEL_WITHOUT_BRIGHTS		8 // EF_CELSHADING
#define		TINTED_CEL_WITH_BRIGHTS			9 // EF_CELSHADING
#define		TINTED_METAL_WITHOUT_BRIGHTS	10 // EF_REFLECTIVE
#define		TINTED_METAL_WITH_BRIGHTS		11 // EF_REFLECTIVE
#define		ALL_COLORSHADINGMAP_STYLES		12 // includes menu mapping
extern	byte
	** colorshadingmap

,	* colorshadingmap_world		// for the world model, including inline BSP models
,	* colorshadingmap_staticent	// for non-inline static entities (efrags)
,	* colorshadingmap_beam		// for cl_tents
,	* colorshadingmap_entity	// for everything else
,	* colorshadingmap_player
,	* colorshadingmap_viewmodel
,	* colorshadingmap_particle
	;
extern	cvar_t
	r_brights_world
,	r_brights_staticent
,	r_brights_beam
,	r_brights_entity
,	r_brights_player
,	r_brights_viewmodel
,	r_brights_particle

,	r_fullbright_colors
,	r_fullbright_range
,	r_fullbright_cel_range
,	r_fullbright_cel_factor

,	gl_cshiftpercent
,	gl_polyblend // mankrip - gl_polyblend
	;
extern int
	gl_polyblend_old // mankrip - gl_polyblend
	;
extern unsigned short
	d_8to16table[256]
	;
extern unsigned
	d_8to24table[256]
	;
extern void
	Palette_Init (void)
,	VID_SetPalette (unsigned char *palette) // called at startup and after any gamma correction
,	VID_ShiftPalette (unsigned char *palette) // called for bonus and pain flashes, and for underwater color changes
,	V_CalcDamageCshift (int armor, int blood, float count)
	;

#define	CSHIFT_CONTENTS	0
#define	CSHIFT_DAMAGE	1
#define	CSHIFT_BONUS	2
#define	CSHIFT_POWERUP	3
#define	NUM_CSHIFTS		4

typedef struct
{
	int
		destcolor[3]
	,	percent // 0-256
		;
} cshift_t;

#define		COLORBLENDINGMAP_SIZE (256*256)

// drawing mode
//#define		BLEND_NONE				0 // definir NONE e SOLID juntos, variando com alpha
#define		BLEND_SOLID				0 // copy mode
#define		BLEND_STIPPLE			1
#define		BLEND_BLEND				2
#define		COLORBLENDING_MODES		3

// actual blend modes
#define		BLEND_WEIGHTEDMEAN		0
#define		BLEND_ADDITIVE			1
#define		BLEND_SMOKE				2
//#define		BLEND_MAX				3
//#define		BLEND_BINARY_OR			4
#define		COLORBLENDING_MAPS	3

extern byte
	** colorblendingmode
,	* colorblendingmap // current color blending map

,	D_DrawPixel_17			(byte over, byte under)
,	D_DrawPixel_33			(byte over, byte under)
,	D_DrawPixel_50			(byte over, byte under)
,	D_DrawPixel_67			(byte over, byte under)
,	D_DrawPixel_83			(byte over, byte under)

,	D_DrawPixel_Sky			(byte over, byte under)
,	D_DrawPixel_Opaque		(byte over, byte under)
,	D_DrawPixel_Transparent	(byte over, byte under)
	;
