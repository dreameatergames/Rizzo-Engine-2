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
// models.c -- model loading and caching

// models are the only shared resource between a client and server running
// on the same machine.

#include "quakedef.h"
#include "r_local.h"
#include "spr_file.h"
#include "mdl_file.h"
#include "bsp_file.h"
#include "ptc_file.h"

model_t	*loadmodel;
char	loadname[32];	// for hunk tags

void Mod_LoadBrushModel (model_t *mod, void *buffer, loadedfile_t *fileinfo);	// 2001-09-12 .ENT support by Maddes
//void Mod_LoadBrushModel (model_t *mod, void *buffer);
void Mod_LoadAliasModel (model_t *mod, void *buffer);
model_t *Mod_LoadModel (model_t *mod, qboolean crash);

byte	mod_novis[MAX_MAP_LEAFS/8];

#define	MAX_MOD_KNOWN	256
model_t	mod_known[MAX_MOD_KNOWN];
int		mod_numknown;

// values for model_t's needload
#define NL_PRESENT		0
#define NL_NEEDS_LOADED	1
#define NL_UNREFERENCED	2

cvar_t	external_ent = {"external_ent","1"};	// 2001-09-12 .ENT support by Maddes
cvar_t	external_vis = {"external_vis","1"};	// 2001-12-28 .VIS support by Maddes
/*
===============
Mod_Init
===============
*/
void Mod_Init (void)
{
	memset (mod_novis, 0xff, sizeof(mod_novis));
	Cvar_RegisterVariable (&external_ent);	// 2001-09-12 .ENT support by Maddes
	Cvar_RegisterVariable (&external_vis);	// 2001-12-28 .VIS support by Maddes
}

/*
===============
Mod_Extradata

Caches the data if needed
===============
*/
void *Mod_Extradata (model_t *mod)
{
	void	*r;

	r = Cache_Check (&mod->cache);
	if (r)
		return r;

	Mod_LoadModel (mod, true);

	if (!mod->cache.data)
		Sys_Error ("Mod_Extradata: caching failed");
	return mod->cache.data;
}

/*
===============
Mod_PointInLeaf
===============
*/
mleaf_t *Mod_PointInLeaf (vec3_t p, model_t *model)
{
	mnode_t		*node;
	float		d;
	mplane_t	*plane;

	if (!model || !model->nodes)
		Sys_Error ("Mod_PointInLeaf: bad model");

	node = model->nodes;
	while (1)
	{
		if (node->contents < 0)
			return (mleaf_t *)node;
		plane = node->plane;
		d = DotProduct (p,plane->normal) - plane->dist;
		if (d > 0)
			node = node->children[0];
		else
			node = node->children[1];
	}

	return NULL;	// never reached
}


/*
===================
Mod_DecompressVis
===================
*/
byte *Mod_DecompressVis (byte *in, model_t *model)
{
	static byte	decompressed[MAX_MAP_LEAFS/8];
	int		c;
	byte	*out;
	int		row;

	row = (model->numleafs+7)>>3;
	out = decompressed;

	if (!in)
	{	// no vis info, so make all visible
		while (row)
		{
			*out++ = 0xff;
			row--;
		}
		return decompressed;
	}

	do
	{
		if (*in)
		{
			*out++ = *in++;
			continue;
		}

		c = in[1];
		in += 2;
		while (c)
		{
			*out++ = 0;
			c--;
		}
	} while (out - decompressed < row);

	return decompressed;
}

byte *Mod_LeafPVS (mleaf_t *leaf, model_t *model)
{
	if (leaf == model->leafs)
		return mod_novis;
	return Mod_DecompressVis (leaf->compressed_vis, model);
}

/*
===================
Mod_ClearAll
===================
*/
void Mod_ClearAll (void)
{
	int		i;
	model_t	*mod;


	for (i=0 , mod=mod_known ; i<mod_numknown ; i++, mod++) {
		mod->needload = NL_UNREFERENCED;
//FIX FOR CACHE_ALLOC ERRORS:
		if (mod->type == mod_sprite) mod->cache.data = NULL;
	}
}

/*
==================
Mod_FindName

==================
*/
model_t *Mod_FindName (char *name)
{
	int		i;
	model_t	*mod;
	model_t	*avail = NULL;

	if (!name[0])
		Sys_Error ("Mod_ForName: NULL name");

//
// search the currently loaded models
//
	for (i=0 , mod=mod_known ; i<mod_numknown ; i++, mod++)
	{
		if (!strcmp (mod->name, name) )
			break;
		if (mod->needload == NL_UNREFERENCED)
			if (!avail || mod->type != mod_alias)
				avail = mod;
	}

	if (i == mod_numknown)
	{
		if (mod_numknown == MAX_MOD_KNOWN)
		{
			if (avail)
			{
				mod = avail;
				if (mod->type == mod_alias)
					if (Cache_Check (&mod->cache))
						Cache_Free (&mod->cache);
			}
			else
				Sys_Error ("mod_numknown == MAX_MOD_KNOWN");
		}
		else
			mod_numknown++;
		strcpy (mod->name, name);
		mod->needload = NL_NEEDS_LOADED;
	}

	return mod;
}

/*
==================
Mod_TouchModel

==================
*/
void Mod_TouchModel (char *name)
{
	model_t	*mod;

	mod = Mod_FindName (name);

	if (mod->needload == NL_PRESENT)
	{
		if (mod->type == mod_alias)
			Cache_Check (&mod->cache);
	}
}

/*
==================
Mod_LoadModel

Loads a model into the cache
==================
*/
model_t *Mod_LoadModel (model_t *mod, qboolean crash)
{
	unsigned *buf;
	byte	stackbuf[1024];		// avoid dirtying the cache heap
	loadedfile_t	*fileinfo;	// 2001-09-12 Returning information about loaded file by Maddes

	if (mod->type == mod_alias)
	{
		if (Cache_Check (&mod->cache))
		{
			mod->needload = NL_PRESENT;
			return mod;
		}
	}
	else
	{
		if (mod->needload == NL_PRESENT)
			return mod;
	}
	if (developer.value == 1) Con_DPrintf ("Loading %s\n",mod->name); // mankrip

//
// because the world is so huge, load it one piece at a time
//

//
// load the file
//
// 2001-09-12 Returning information about loaded file by Maddes  start
/*
	buf = (unsigned *)COM_LoadStackFile (mod->name, stackbuf, sizeof(stackbuf));
	if (!buf)
*/
	fileinfo = COM_LoadStackFile (mod->name, stackbuf, sizeof(stackbuf));
	if (!fileinfo)
// 2001-09-12 Returning information about loaded file by Maddes  end
	{
		if (crash)
			Sys_Error ("Mod_NumForName: %s not found", mod->name);
		return NULL;
	}
	buf = (unsigned *)fileinfo->data;	// 2001-09-12 Returning information about loaded file by Maddes

//
// allocate a new model
//
	COM_FileBase (mod->name, loadname);

	loadmodel = mod;

//
// fill it in
//

// call the apropriate loader
	mod->needload = NL_PRESENT;

	switch (LittleLong(*(unsigned *)buf))
	{
	case IDPOLYHEADER:
		Mod_LoadAliasModel (mod, buf);
		break;

	case IDSPRITEHEADER:
		Mod_LoadSpriteModel (mod, buf);
		break;

	default:
		Mod_LoadBrushModel (mod, buf, fileinfo);	// 2001-09-12 .ENT support by Maddes
		break;
	}

	return mod;
}

/*
==================
Mod_ForName

Loads in a model for the given name
==================
*/
model_t *Mod_ForName (char *name, qboolean crash)
{
	model_t	*mod;

	mod = Mod_FindName (name);

	return Mod_LoadModel (mod, crash);
}


/*
===============================================================================

					BRUSHMODEL LOADING

===============================================================================
*/

byte	*mod_base;


/*
=================
Mod_LoadTextures
=================
*/
void Mod_LoadTextures (lump_t *l)
{
	int		i, j, pixels, num, max, altmax;
	miptex_t	*mt;
	texture_t	*tx, *tx2;
	texture_t	*anims[10];
	texture_t	*altanims[10];
	dmiptexlump_t *m;

	if (!l->filelen)
	{
		loadmodel->textures = NULL;
		return;
	}
	m = (dmiptexlump_t *)(mod_base + l->fileofs);

	m->nummiptex = LittleLong (m->nummiptex);

	loadmodel->numtextures = m->nummiptex;
	loadmodel->textures = Hunk_AllocName (m->nummiptex * sizeof(*loadmodel->textures) , loadname);

	for (i=0 ; i<m->nummiptex ; i++)
	{
		m->dataofs[i] = LittleLong(m->dataofs[i]);
		if (m->dataofs[i] == -1)
			continue;
		mt = (miptex_t *)((byte *)m + m->dataofs[i]);
		mt->width = LittleLong (mt->width);
		mt->height = LittleLong (mt->height);
		for (j=0 ; j<MIPLEVELS ; j++)
			mt->offsets[j] = LittleLong (mt->offsets[j]);

		if ( (mt->width & 15) || (mt->height & 15) )
			Sys_Error ("Texture %s is not 16 aligned", mt->name);
		pixels = mt->width*mt->height/64*85;
		tx = Hunk_AllocName (sizeof(texture_t) +pixels, loadname );
		loadmodel->textures[i] = tx;

		memcpy (tx->name, mt->name, sizeof(tx->name));
		tx->width = mt->width;
		tx->height = mt->height;
		for (j=0 ; j<MIPLEVELS ; j++)
			tx->offsets[j] = mt->offsets[j] + sizeof(texture_t) - sizeof(miptex_t);
		// the pixels immediately follow the structures
		memcpy ( tx+1, mt+1, pixels);
		RemapBackwards_Pixels ( (byte *) tx + sizeof (texture_t), pixels); // mankrip

		if (!Q_strncmp(mt->name,"sky",3))
		// mankrip - begin
		{
			if (pixels >= 256*128)
				for (j=0 ; j<128 ; j++)
					RemapColorKey_Pixels ( (byte *) tx + sizeof (texture_t) + j*256, 128);
		// mankrip - end
			R_InitSky (tx);
		} // mankrip
	}

//
// sequence the animations
//
	for (i=0 ; i<m->nummiptex ; i++)
	{
		tx = loadmodel->textures[i];
		if (!tx || tx->name[0] != '+')
			continue;
		if (tx->anim_next)
			continue;	// allready sequenced

	// find the number of frames in the animation
		memset (anims, 0, sizeof(anims));
		memset (altanims, 0, sizeof(altanims));

		max = tx->name[1];
		altmax = 0;
		if (max >= 'a' && max <= 'z')
			max -= 'a' - 'A';
		if (max >= '0' && max <= '9')
		{
			max -= '0';
			altmax = 0;
			anims[max] = tx;
			max++;
		}
		else if (max >= 'A' && max <= 'J')
		{
			altmax = max - 'A';
			max = 0;
			altanims[altmax] = tx;
			altmax++;
		}
		else
			Sys_Error ("Bad animating texture %s", tx->name);

		for (j=i+1 ; j<m->nummiptex ; j++)
		{
			tx2 = loadmodel->textures[j];
			if (!tx2 || tx2->name[0] != '+')
				continue;
			if (strcmp (tx2->name+2, tx->name+2))
				continue;

			num = tx2->name[1];
			if (num >= 'a' && num <= 'z')
				num -= 'a' - 'A';
			if (num >= '0' && num <= '9')
			{
				num -= '0';
				anims[num] = tx2;
				if (num+1 > max)
					max = num + 1;
			}
			else if (num >= 'A' && num <= 'J')
			{
				num = num - 'A';
				altanims[num] = tx2;
				if (num+1 > altmax)
					altmax = num+1;
			}
			else
				Sys_Error ("Bad animating texture %s", tx->name);
		}

#define	ANIM_CYCLE	2
	// link them all together
		for (j=0 ; j<max ; j++)
		{
			tx2 = anims[j];
			if (!tx2)
				Sys_Error ("Missing frame %i of %s",j, tx->name);
			tx2->anim_total = max * ANIM_CYCLE;
			tx2->anim_min = j * ANIM_CYCLE;
			tx2->anim_max = (j+1) * ANIM_CYCLE;
			tx2->anim_next = anims[ (j+1)%max ];
			if (altmax)
				tx2->alternate_anims = altanims[0];
		}
		for (j=0 ; j<altmax ; j++)
		{
			tx2 = altanims[j];
			if (!tx2)
				Sys_Error ("Missing frame %i of %s",j, tx->name);
			tx2->anim_total = altmax * ANIM_CYCLE;
			tx2->anim_min = j * ANIM_CYCLE;
			tx2->anim_max = (j+1) * ANIM_CYCLE;
			tx2->anim_next = altanims[ (j+1)%altmax ];
			if (max)
				tx2->alternate_anims = anims[0];
		}
	}
}

/*
=================
Mod_LoadLighting
=================
*/
void Mod_LoadLighting (lump_t *l)
{
	if (!l->filelen)
	{
		loadmodel->lightdata = NULL;
		return;
	}
	loadmodel->lightdata = Hunk_AllocName ( l->filelen, loadname);
	memcpy (loadmodel->lightdata, mod_base + l->fileofs, l->filelen);
}


/*
=================
Mod_LoadVisibility
=================
*/
void Mod_LoadVisibility (lump_t *l)
{
	if (!l->filelen)
	{
		loadmodel->visdata = NULL;
		return;
	}
// 2001-12-28 .VIS support by Maddes  start
//	loadmodel->visdata = Hunk_AllocName ( l->filelen, loadname);
	loadmodel->visdata = Hunk_AllocName ( l->filelen, "INT_VIS");
// 2001-12-28 .VIS support by Maddes  end
	memcpy (loadmodel->visdata, mod_base + l->fileofs, l->filelen);
}


/*
=================
Mod_LoadEntities
=================
*/
void Mod_LoadEntities (lump_t *l, loadedfile_t *brush_fileinfo)	// 2001-09-12 .ENT support by Maddes
{
// 2001-09-12 .ENT support by Maddes  start
	char	entfilename[1024];
	loadedfile_t	*fileinfo;
	searchpath_t	*s_check;

	loadmodel->entities = NULL;

	if (external_ent.value)
	{
		// check for a .ENT file
		strcpy(entfilename, loadmodel->name);
		COM_StripExtension(entfilename, entfilename);
		strcat(entfilename, ".ent");

		fileinfo = COM_LoadHunkFile (entfilename);
		if (fileinfo && fileinfo->filelen)
		{
			// .ENT file only allowed from same directory of map file or another directory before in the searchpath
			s_check = COM_GetDirSearchPath(brush_fileinfo->path);	// get last searchpath of map directory
			// mankrip - begin
			if (s_check->pack) // it's a damn pack file, so let's find the searchpath of the directory where it is
			{
				char	brushfilename[1024];
				int i;
				searchpath_t	*s_temp;

				strcpy(brushfilename, s_check->pack->filename);
				// remove the packfile from the path
				for (i=Q_strlen(brushfilename); i > 0 ; i--)
					if (brushfilename[i] == '/' || brushfilename[i] == '\\')
					{
						brushfilename[i] = 0;
						break;
					}
				// find the searchpath of the directory
				s_temp = s_check;
				while ((s_temp = s_temp->next))
					if (!s_temp->pack)
						if (!Q_strcmp(brushfilename, s_temp->filename))
						{
							s_check = s_temp;
							break;
						}
			}
			// mankrip - end
			while ( (s_check = s_check->next) )		// next searchpath
			{
				if (s_check == fileinfo->path)	// found .ENT searchpath = after map directory
				{
					Con_DPrintf("%s not allowed from %s as %s is from %s\n",
						entfilename,
						fileinfo->path->pack ? fileinfo->path->pack->filename : fileinfo->path->filename,
						loadmodel->name,
						brush_fileinfo->path->pack ? brush_fileinfo->path->pack->filename : brush_fileinfo->path->filename);
					break;
				}
			}

			if (!s_check)	// .ENT searchpath not found = before map directory
			{
				Con_DPrintf("Loaded %s\n", entfilename);//, fileinfo->path->pack ? fileinfo->path->pack->filename : fileinfo->path->filename);
				loadmodel->entities = fileinfo->data;
				return;
			}
		}

		// no .ENT found, use the original entity list
	}
// 2001-09-12 .ENT support by Maddes  end
	if (!l->filelen)
	{
		loadmodel->entities = NULL;
		return;
	}
// 2001-09-12 .ENT support by Maddes  start
//	loadmodel->entities = Hunk_AllocName ( l->filelen, loadname);
	loadmodel->entities = Hunk_AllocName ( l->filelen, "INT_ENT");
// 2001-09-12 .ENT support by Maddes  end
	memcpy (loadmodel->entities, mod_base + l->fileofs, l->filelen);
}


/*
=================
Mod_LoadVertexes
=================
*/
void Mod_LoadVertexes (lump_t *l)
{
	dvertex_t	*in;
	mvertex_t	*out;
	int			i, count;

	in = (void *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Sys_Error ("MOD_LoadBmodel: funny lump size in %s",loadmodel->name);
	count = l->filelen / sizeof(*in);
//	out = Hunk_AllocName ( count*sizeof(*out), loadname);
	out = Hunk_AllocName ( (count+8)*sizeof(*out), loadname); // mankrip - skyboxes - extra for skybox // Code taken from the ToChriS engine - Author: Vic (vic@quakesrc.org) (http://hkitchen.quakesrc.org/)

	loadmodel->vertexes = out;
	loadmodel->numvertexes = count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		out->position[0] = LittleFloat (in->point[0]);
		out->position[1] = LittleFloat (in->point[1]);
		out->position[2] = LittleFloat (in->point[2]);
	}
}

/*
=================
Mod_LoadSubmodels
=================
*/
void Mod_LoadSubmodels (lump_t *l)
{
	dmodel_t	*in;
	dmodel_t	*out;
	int			i, j, count;

	in = (void *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Sys_Error ("MOD_LoadBmodel: funny lump size in %s",loadmodel->name);
	count = l->filelen / sizeof(*in);
	out = Hunk_AllocName ( count*sizeof(*out), loadname);

	loadmodel->submodels = out;
	loadmodel->numsubmodels = count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		for (j=0 ; j<3 ; j++)
		{	// spread the mins / maxs by a pixel
			out->mins[j] = LittleFloat (in->mins[j]) - 1;
			out->maxs[j] = LittleFloat (in->maxs[j]) + 1;
			out->origin[j] = LittleFloat (in->origin[j]);
		}
		for (j=0 ; j<MAX_MAP_HULLS ; j++)
			out->headnode[j] = LittleLong (in->headnode[j]);
		out->visleafs = LittleLong (in->visleafs);
		out->firstface = LittleLong (in->firstface);
		out->numfaces = LittleLong (in->numfaces);
	}
}

/*
=================
Mod_LoadEdges
=================
*/
void Mod_LoadEdges (lump_t *l)
{
	dedge_t *in;
	medge_t *out;
	int 	i, count;

	in = (void *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Sys_Error ("MOD_LoadBmodel: funny lump size in %s",loadmodel->name);
	count = l->filelen / sizeof(*in);
//	out = Hunk_AllocName ( (count + 1) * sizeof(*out), loadname);
	out = Hunk_AllocName ( (count + 13) * sizeof(*out), loadname); // mankrip - skyboxes - extra for skybox // Code taken from the ToChriS engine - Author: Vic (vic@quakesrc.org) (http://hkitchen.quakesrc.org/)

	loadmodel->edges = out;
	loadmodel->numedges = count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		out->v[0] = (unsigned short)LittleShort(in->v[0]);
		out->v[1] = (unsigned short)LittleShort(in->v[1]);
	}
}

/*
=================
Mod_LoadTexinfo
=================
*/
void Mod_LoadTexinfo (lump_t *l)
{
	texinfo_t *in;
	mtexinfo_t *out;
	int 	i, j, count;
	int		miptex;
	float	len1, len2;

	in = (void *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Sys_Error ("MOD_LoadBmodel: funny lump size in %s",loadmodel->name);
	count = l->filelen / sizeof(*in);
//	out = Hunk_AllocName ( count*sizeof(*out), loadname);
	out = Hunk_AllocName ( (count+6)*sizeof(*out), loadname); // mankrip - skyboxes - extra for skybox // Code taken from the ToChriS engine - Author: Vic (vic@quakesrc.org) (http://hkitchen.quakesrc.org/)
	loadmodel->texinfo = out;
	loadmodel->numtexinfo = count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		for (j=0 ; j<8 ; j++)
			out->vecs[0][j] = LittleFloat (in->vecs[0][j]);
		len1 = Length (out->vecs[0]);
		len2 = Length (out->vecs[1]);
		len1 = (len1 + len2)/2;
		if (len1 < 0.32)
			out->mipadjust = 4;
		else if (len1 < 0.49)
			out->mipadjust = 3;
		else if (len1 < 0.99)
			out->mipadjust = 2;
		else
			out->mipadjust = 1;
#if 0
		if (len1 + len2 < 0.001)
			out->mipadjust = 1;		// don't crash
		else
			out->mipadjust = 1 / floor( (len1+len2)/2 + 0.1 );
#endif

		miptex = LittleLong (in->miptex);
		out->flags = LittleLong (in->flags);

		if (!loadmodel->textures)
		{
			out->texture = r_notexture_mip;	// checkerboard texture
			out->flags = 0;
		}
		else
		{
			if (miptex >= loadmodel->numtextures)
				Sys_Error ("miptex >= loadmodel->numtextures");
			out->texture = loadmodel->textures[miptex];
			if (!out->texture)
			{
				out->texture = r_notexture_mip; // texture not found
				out->flags = 0;
			}
		}
	}
}

/*
================
CalcSurfaceExtents

Fills in s->texturemins[] and s->extents[]
================
*/
void CalcSurfaceExtents (msurface_t *s)
{
	float	mins[2], maxs[2], val;
	int		i,j, e;
	mvertex_t	*v;
	mtexinfo_t	*tex;
	int		bmins[2], bmaxs[2];

	mins[0] = mins[1] = 999999;
	maxs[0] = maxs[1] = -99999;

	tex = s->texinfo;

	for (i=0 ; i<s->numedges ; i++)
	{
		e = loadmodel->surfedges[s->firstedge+i];
		if (e >= 0)
			v = &loadmodel->vertexes[loadmodel->edges[e].v[0]];
		else
			v = &loadmodel->vertexes[loadmodel->edges[-e].v[1]];

		for (j=0 ; j<2 ; j++)
		{
			val = v->position[0] * tex->vecs[j][0] +
				v->position[1] * tex->vecs[j][1] +
				v->position[2] * tex->vecs[j][2] +
				tex->vecs[j][3];
			if (val < mins[j])
				mins[j] = val;
			if (val > maxs[j])
				maxs[j] = val;
		}
	}

	for (i=0 ; i<2 ; i++)
	{
		bmins[i] = floor(mins[i]/16);
		bmaxs[i] = ceil(maxs[i]/16);

		s->texturemins[i] = bmins[i] * 16;
		s->extents[i] = (bmaxs[i] - bmins[i]) * 16;
		if ( !(tex->flags & TEX_SPECIAL) && s->extents[i] > 256)
			Sys_Error ("Bad surface extents");
	}
}


/*
=================
Mod_LoadFaces
=================
*/
void Mod_LoadFaces (lump_t *l)
{
	dface_t		*in;
	msurface_t 	*out;
	int			i, count, surfnum;
	int			planenum, side;

	in = (void *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Sys_Error ("MOD_LoadBmodel: funny lump size in %s",loadmodel->name);
	count = l->filelen / sizeof(*in);
//	out = Hunk_AllocName ( count*sizeof(*out), loadname);
	out = Hunk_AllocName ( (count+6)*sizeof(*out), loadname); // mankrip - skyboxes - extra for skybox // Code taken from the ToChriS engine - Author: Vic (vic@quakesrc.org) (http://hkitchen.quakesrc.org/)
	loadmodel->surfaces = out;
	loadmodel->numsurfaces = count;

	for ( surfnum=0 ; surfnum<count ; surfnum++, in++, out++)
	{
		out->firstedge = LittleLong(in->firstedge);
		out->numedges = LittleShort(in->numedges);
		out->flags = 0;

		planenum = LittleShort(in->planenum);
		side = LittleShort(in->side);
		if (side)
			out->flags |= SURF_PLANEBACK;

		out->plane = loadmodel->planes + planenum;

		out->texinfo = loadmodel->texinfo + LittleShort (in->texinfo);

		CalcSurfaceExtents (out);

	// lighting info

		for (i=0 ; i<MAXLIGHTMAPS ; i++)
			out->styles[i] = in->styles[i];
		i = LittleLong(in->lightofs);
		if (i == -1)
			out->samples = NULL;
		else
			out->samples = loadmodel->lightdata + i;

	// set the drawing flags flag

		if (!Q_strncmp(out->texinfo->texture->name,"*"			,  1))		// turbulent
		{
			// mankrip - translucent water - begin
					if (!Q_strncmp (out->texinfo->texture->name, "*water"	, 6))	out->flags |= SURF_BLEND_WATER	;
			else	if (!Q_strncmp (out->texinfo->texture->name, "*slime"	, 6))	out->flags |= SURF_BLEND_SLIME	;
			else	if (!Q_strncmp (out->texinfo->texture->name, "*lava"	, 5))	out->flags |= SURF_BLEND_LAVA	;
			else	if (!Q_strncmp (out->texinfo->texture->name, "*teleport", 9))	out->flags |= SURF_BLEND_PORTAL	;
			// mankrip - translucent water - end
			out->flags |= (SURF_DRAWTURB | SURF_DRAWTILED);
			for (i=0 ; i<2 ; i++)
			{
				out->extents[i] = 16384;
				out->texturemins[i] = -8192;
			}
			continue;
		}
		else if (!Q_strncmp(out->texinfo->texture->name,"sky",3))	// sky
		{
			out->flags |= (SURF_DRAWSKY | SURF_DRAWTILED);
			continue;
		}
		else if (!Q_strncmp(out->texinfo->texture->name,"window02_1"	, 10))		// mirror
			out->flags |= SURF_BLEND_GLASS;
	}
}


/*
=================
Mod_SetParent
=================
*/
void Mod_SetParent (mnode_t *node, mnode_t *parent)
{
	node->parent = parent;
	if (node->contents < 0)
		return;
	Mod_SetParent (node->children[0], node);
	Mod_SetParent (node->children[1], node);
}

/*
=================
Mod_LoadNodes
=================
*/
void Mod_LoadNodes (lump_t *l)
{
	int			i, j, count, p;
	dnode_t		*in;
	mnode_t 	*out;

	in = (void *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Sys_Error ("MOD_LoadBmodel: funny lump size in %s",loadmodel->name);
	count = l->filelen / sizeof(*in);
	out = Hunk_AllocName ( count*sizeof(*out), loadname);

	loadmodel->nodes = out;
	loadmodel->numnodes = count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		for (j=0 ; j<3 ; j++)
		{
			out->minmaxs[j] = LittleShort (in->mins[j]);
			out->minmaxs[3+j] = LittleShort (in->maxs[j]);
		}

		p = LittleLong(in->planenum);
		out->plane = loadmodel->planes + p;

		out->firstsurface = LittleShort (in->firstface);
		out->numsurfaces = LittleShort (in->numfaces);

		for (j=0 ; j<2 ; j++)
		{
			p = LittleShort (in->children[j]);
			if (p >= 0)
				out->children[j] = loadmodel->nodes + p;
			else
				out->children[j] = (mnode_t *)(loadmodel->leafs + (-1 - p));
		}
	}

	Mod_SetParent (loadmodel->nodes, NULL);	// sets nodes and leafs
}

void Mod_ProcessLeafs (dleaf_t *in, int filelen);	// 2001-12-28 .VIS support by Maddes
/*
=================
Mod_LoadLeafs
=================
*/
void Mod_LoadLeafs (lump_t *l)
{
	dleaf_t 	*in;
// 2001-12-28 .VIS support by Maddes  start
/*
	mleaf_t 	*out;
	int			i, j, count, p;
*/
// 2001-12-28 .VIS support by Maddes  end

	in = (void *)(mod_base + l->fileofs);
// 2001-12-28 .VIS support by Maddes  start
/*
	if (l->filelen % sizeof(*in))
		Sys_Error ("MOD_LoadBmodel: funny lump size in %s",loadmodel->name);
	count = l->filelen / sizeof(*in);
	out = Hunk_AllocName ( count*sizeof(*out), loadname);
*/

	Mod_ProcessLeafs (in, l->filelen);
}

void Mod_ProcessLeafs (dleaf_t *in, int filelen)
{
	mleaf_t 	*out;
	int			i, j, count, p;

	if (filelen % sizeof(*in))
		Sys_Error ("Mod_ProcessLeafs: funny lump size in %s", loadmodel->name);
	count = filelen / sizeof(*in);
	out = Hunk_AllocName ( count*sizeof(*out), "USE_LEAF");
// 2001-12-28 .VIS support by Maddes  end

	loadmodel->leafs = out;
	loadmodel->numleafs = count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		for (j=0 ; j<3 ; j++)
		{
			out->minmaxs[j] = LittleShort (in->mins[j]);
			out->minmaxs[3+j] = LittleShort (in->maxs[j]);
		}

		p = LittleLong(in->contents);
		out->contents = p;

		out->firstmarksurface = loadmodel->marksurfaces +
			LittleShort(in->firstmarksurface);
		out->nummarksurfaces = LittleShort(in->nummarksurfaces);

		p = LittleLong(in->visofs);
		if (p == -1)
			out->compressed_vis = NULL;
		else
			out->compressed_vis = loadmodel->visdata + p;
		out->efrags = NULL;

		for (j=0 ; j<4 ; j++)
			out->ambient_sound_level[j] = in->ambient_level[j];
	}
}

/*
=================
Mod_LoadClipnodes
=================
*/
void Mod_LoadClipnodes (lump_t *l)
{
	dclipnode_t *in, *out;
	int			i, count;
	hull_t		*hull;

	in = (void *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Sys_Error ("MOD_LoadBmodel: funny lump size in %s",loadmodel->name);
	count = l->filelen / sizeof(*in);
	out = Hunk_AllocName ( count*sizeof(*out), loadname);

	loadmodel->clipnodes = out;
	loadmodel->numclipnodes = count;

	hull = &loadmodel->hulls[1];
	hull->clipnodes = out;
	hull->firstclipnode = 0;
	hull->lastclipnode = count-1;
	hull->planes = loadmodel->planes;
	hull->clip_mins[0] = -16;
	hull->clip_mins[1] = -16;
	hull->clip_mins[2] = -24;
	hull->clip_maxs[0] = 16;
	hull->clip_maxs[1] = 16;
	hull->clip_maxs[2] = 32;

	hull = &loadmodel->hulls[2];
	hull->clipnodes = out;
	hull->firstclipnode = 0;
	hull->lastclipnode = count-1;
	hull->planes = loadmodel->planes;
	hull->clip_mins[0] = -32;
	hull->clip_mins[1] = -32;
	hull->clip_mins[2] = -24;
	hull->clip_maxs[0] = 32;
	hull->clip_maxs[1] = 32;
	hull->clip_maxs[2] = 64;

	for (i=0 ; i<count ; i++, out++, in++)
	{
		out->planenum = LittleLong(in->planenum);
		out->children[0] = LittleShort(in->children[0]);
		out->children[1] = LittleShort(in->children[1]);
	}
}

/*
=================
Mod_MakeHull0

Deplicate the drawing hull structure as a clipping hull
=================
*/
void Mod_MakeHull0 (void)
{
	mnode_t		*in, *child;
	dclipnode_t *out;
	int			i, j, count;
	hull_t		*hull;

	hull = &loadmodel->hulls[0];

	in = loadmodel->nodes;
	count = loadmodel->numnodes;
	out = Hunk_AllocName ( count*sizeof(*out), loadname);

	hull->clipnodes = out;
	hull->firstclipnode = 0;
	hull->lastclipnode = count-1;
	hull->planes = loadmodel->planes;

	for (i=0 ; i<count ; i++, out++, in++)
	{
		out->planenum = in->plane - loadmodel->planes;
		for (j=0 ; j<2 ; j++)
		{
			child = in->children[j];
			if (child->contents < 0)
				out->children[j] = child->contents;
			else
				out->children[j] = child - loadmodel->nodes;
		}
	}
}

/*
=================
Mod_LoadMarksurfaces
=================
*/
void Mod_LoadMarksurfaces (lump_t *l)
{
	int		i, j, count;
	short		*in;
	msurface_t **out;

	in = (void *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Sys_Error ("MOD_LoadBmodel: funny lump size in %s",loadmodel->name);
	count = l->filelen / sizeof(*in);
	out = Hunk_AllocName ( count*sizeof(*out), loadname);

	loadmodel->marksurfaces = out;
	loadmodel->nummarksurfaces = count;

	for ( i=0 ; i<count ; i++)
	{
		j = LittleShort(in[i]);
		if (j >= loadmodel->numsurfaces)
			Sys_Error ("Mod_ParseMarksurfaces: bad surface number");
		out[i] = loadmodel->surfaces + j;
	}
}

/*
=================
Mod_LoadSurfedges
=================
*/
void Mod_LoadSurfedges (lump_t *l)
{
	int		i, count;
	int		*in, *out;

	in = (void *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Sys_Error ("MOD_LoadBmodel: funny lump size in %s",loadmodel->name);
	count = l->filelen / sizeof(*in);
//	out = Hunk_AllocName ( count*sizeof(*out), loadname);
	out = Hunk_AllocName ( (count+24)*sizeof(*out), loadname); // mankrip - skyboxes - extra for skybox // Code taken from the ToChriS engine - Author: Vic (vic@quakesrc.org) (http://hkitchen.quakesrc.org/)

	loadmodel->surfedges = out;
	loadmodel->numsurfedges = count;

	for ( i=0 ; i<count ; i++)
		out[i] = LittleLong (in[i]);
}

/*
=================
Mod_LoadPlanes
=================
*/
void Mod_LoadPlanes (lump_t *l)
{
	int			i, j;
	mplane_t	*out;
	dplane_t 	*in;
	int			count;
	int			bits;

	in = (void *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Sys_Error ("MOD_LoadBmodel: funny lump size in %s",loadmodel->name);
	count = l->filelen / sizeof(*in);
//	out = Hunk_AllocName ( count*2*sizeof(*out), loadname);
	out = Hunk_AllocName ( (count+6)*2*sizeof(*out), loadname); // mankrip - skyboxes - extra for skybox // Code taken from the ToChriS engine - Author: Vic (vic@quakesrc.org) (http://hkitchen.quakesrc.org/)

	loadmodel->planes = out;
	loadmodel->numplanes = count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		bits = 0;
		for (j=0 ; j<3 ; j++)
		{
			out->normal[j] = LittleFloat (in->normal[j]);
			if (out->normal[j] < 0)
				bits |= 1<<j;
		}

		out->dist = LittleFloat (in->dist);
		out->type = LittleLong (in->type);
		out->signbits = bits;
	}
}

/*
=================
RadiusFromBounds
=================
*/
float RadiusFromBounds (vec3_t mins, vec3_t maxs)
{
	int		i;
	vec3_t	corner;

	for (i=0 ; i<3 ; i++)
	{
		corner[i] = fabs(mins[i]) > fabs(maxs[i]) ? fabs(mins[i]) : fabs(maxs[i]);
	}

	return Length (corner);
}

// 2001-12-28 .VIS support by Maddes  start
/*
=================
Mod_LoadExternalVisibility
=================
*/
void Mod_LoadExternalVisibility (int fhandle)
{
	long	filelen;

	// get visibility data length
	filelen = 0;
	Sys_FileRead (fhandle, &filelen, 4);
	filelen = LittleLong(filelen);

	Con_DPrintf("...%i bytes visibility data\n", filelen);

	// load visibility data
	if (!filelen)
	{
		loadmodel->visdata = NULL;
		return;
	}
	loadmodel->visdata = Hunk_AllocName ( filelen, "EXT_VIS");
	Sys_FileRead (fhandle, loadmodel->visdata, filelen);
}

/*
=================
Mod_LoadExternalLeafs
=================
*/
void Mod_LoadExternalLeafs (int fhandle)
{
	dleaf_t 	*in;
	long	filelen;

	// get leaf data length
	filelen = 0;
	Sys_FileRead (fhandle, &filelen, 4);
	filelen = LittleLong(filelen);

	Con_DPrintf("...%i bytes leaf data\n", filelen);

	// load leaf data
	if (!filelen)
	{
		loadmodel->leafs = NULL;
		loadmodel->numleafs = 0;
		return;
	}
	in = Hunk_AllocName (filelen, "EXT_LEAF");
	Sys_FileRead (fhandle, in, filelen);

	Mod_ProcessLeafs (in, filelen);
}

int Mod_FindExternalVIS (loadedfile_t *brush_fileinfo)
{
	char	visfilename[1024];
	char	vispathname[MAX_OSPATH]; // mankrip
	int		fhandle;
	int		len, i, pos;
	searchpath_t	*s_vis;
	vispatch_t	header;
	char	mapname[VISPATCH_MAPNAME_LENGTH+5];	// + ".vis" + EoS

	fhandle = -1;

	if (external_vis.value)
	{
		// check for a .VIS file
		strcpy(visfilename, loadmodel->name);
		COM_StripExtension(visfilename, visfilename);
		strcat(visfilename, ".vis");

		len = COM_OpenFile (visfilename, &fhandle, &s_vis);
		if (fhandle == -1)	// check for a .VIS file with map's directory name (e.g. ID1.VIS)
		{
		//	strcpy(visfilename, "maps/"); // mankrip - removed
		//	strcat(visfilename, COM_SkipPath(brush_fileinfo->path->filename)); // mankrip - removed

			// mankrip - begin
			// wrong: quake\gamedir\maps\gamedir.vis - don't load from the "maps" directory, to prevent mistakes if there's a map named "gamedir.bsp"
			// right: quake\gamedir\gamedir.vis - we'll look for the VIS packs directly into the game directory
			if (brush_fileinfo->path->pack)
			{
				strcpy(visfilename, brush_fileinfo->path->pack->filename);
				// remove the packfile from the path
				for (i=Q_strlen(visfilename); i > 0 ; i--)
					if (visfilename[i] == '/' || visfilename[i] == '\\')
					{
						visfilename[i] = 0;
						break;
					}
			//	strcpy(vispathname, visfilename);
				strcpy(visfilename, COM_SkipPath(visfilename));
			}
			else
			{
			//	strcpy(vispathname, brush_fileinfo->path->filename);
				strcpy(visfilename, COM_SkipPath(brush_fileinfo->path->filename));
			}
		/*	// make sure it is in lowercase
			for (i=Q_strlen(visfilename); i > 0 ; i--)
				if (visfilename[i] >= 'A' && visfilename[i] <= 'Z')
					visfilename[i] += ('a' - 'A');
		*/	// mankrip - end

			strcat(visfilename, ".vis");
			len = COM_OpenFile (visfilename, &fhandle, &s_vis);
		}
		// mankrip - begin
		else if (fhandle >= 0)
		{
			Q_strcpy(vispathname, brush_fileinfo->path->pack ? brush_fileinfo->path->pack->filename : brush_fileinfo->path->filename);
			// get the full path to the game dir where the .bsp file is
			for (i=Q_strlen(vispathname); i > 0 ; i--)
				if (vispathname[i] == '/' || vispathname[i] == '\\')
				{
					vispathname[i] = 0;
					break;
				}
			Q_strcpy(visfilename, s_vis->pack ? s_vis->pack->filename : s_vis->filename);
			// get the full path to the game dir where the .vis file is
			for (i=Q_strlen(visfilename); i > 0 ; i--)
				if (visfilename[i] == '/' || visfilename[i] == '\\')
				{
					visfilename[i] = 0;
					break;
				}
			if (Q_strcasecmp(vispathname, visfilename)) // different game directories
			{
				COM_CloseFile(fhandle);
				return -1;
			}
		}
		// mankrip - end

		if (fhandle >= 0)
		{
			// check file for size
			if (len <= 0)
			{
				COM_CloseFile(fhandle);
				fhandle = -1;
			}
		}

		if (fhandle >= 0)
		{
			// search map in visfile
			strncpy(mapname, loadname, VISPATCH_MAPNAME_LENGTH);
			mapname[VISPATCH_MAPNAME_LENGTH] = 0;
			strcat(mapname, ".bsp");

			pos = 0;
			while ((i = Sys_FileRead (fhandle, &header, sizeof(struct vispatch_s))) == sizeof(struct vispatch_s))
			{
				header.filelen = LittleLong(header.filelen);

				pos += i;

				if (!Q_strncasecmp (header.mapname, mapname, VISPATCH_MAPNAME_LENGTH))	// found
				{
					break;
				}

				pos += header.filelen;

				Sys_FileSeek(fhandle, pos);
			}

			if (i != sizeof(struct vispatch_s))
			{
				COM_CloseFile(fhandle);
				fhandle = -1;
			}
		}

		if (fhandle >= 0)
		{
			Con_DPrintf("Loaded %s for %s from %s\n", visfilename, mapname, s_vis->pack ? s_vis->pack->filename : s_vis->filename);
		}
	}

	return fhandle;
}
// 2001-12-28 .VIS support by Maddes  end

/*
=================
Mod_LoadBrushModel
=================
*/
void Mod_LoadBrushModel (model_t *mod, void *buffer, loadedfile_t *brush_fileinfo)	// 2001-09-12 .ENT support by Maddes
{
	int			i, j;
	dheader_t	*header;
	dmodel_t 	*bm;
	int			fhandle;	// 2001-12-28 .VIS support by Maddes

	loadmodel->type = mod_brush;

	header = (dheader_t *)buffer;

	i = LittleLong (header->version);
	if (i != BSPVERSION)
//		Sys_Error ("Mod_LoadBrushModel: %s has wrong version number (%i should be %i)", mod->name, i, BSPVERSION);
	// MrG - incorrect BSP version is no longer fatal - begin
	{
		Con_Printf("Mod_LoadBrushModel: %s has wrong version number (%i should be %i)", mod->name, i, BSPVERSION);
		mod->numvertexes=-1;	// HACK
		return;
	}
	// MrG - incorrect BSP version is no longer fatal - end

// swap all the lumps
	mod_base = (byte *)header;

	for (i=0 ; i<sizeof(dheader_t)/4 ; i++)
		((int *)header)[i] = LittleLong ( ((int *)header)[i]);

// load into heap

	Mod_LoadVertexes (&header->lumps[LUMP_VERTEXES]);
	Mod_LoadEdges (&header->lumps[LUMP_EDGES]);
	Mod_LoadSurfedges (&header->lumps[LUMP_SURFEDGES]);
	Mod_LoadTextures (&header->lumps[LUMP_TEXTURES]);
	Mod_LoadLighting (&header->lumps[LUMP_LIGHTING]);
	Mod_LoadPlanes (&header->lumps[LUMP_PLANES]);
	Mod_LoadTexinfo (&header->lumps[LUMP_TEXINFO]);
	Mod_LoadFaces (&header->lumps[LUMP_FACES]);
	Mod_LoadMarksurfaces (&header->lumps[LUMP_MARKSURFACES]);
// 2001-12-28 .VIS support by Maddes  start
	loadmodel->visdata = NULL;
	loadmodel->leafs = NULL;
	loadmodel->numleafs = 0;

	fhandle = Mod_FindExternalVIS (brush_fileinfo);
	if (fhandle >= 0)
	{
		Mod_LoadExternalVisibility (fhandle);
		Mod_LoadExternalLeafs (fhandle);
	}

	if ((loadmodel->visdata == NULL)
	    || (loadmodel->leafs == NULL)
	    || (loadmodel->numleafs == 0))
	{
		if (fhandle >= 0)
		{
			Con_Printf("External VIS data are invalid!!!\n");
		}
// 2001-12-28 .VIS support by Maddes  end
	Mod_LoadVisibility (&header->lumps[LUMP_VISIBILITY]);
	Mod_LoadLeafs (&header->lumps[LUMP_LEAFS]);
// 2001-12-28 .VIS support by Maddes  start
	}

	if (fhandle >= 0)
	{
		COM_CloseFile(fhandle);
	}
// 2001-12-28 .VIS support by Maddes  end
	Mod_LoadNodes (&header->lumps[LUMP_NODES]);
	Mod_LoadClipnodes (&header->lumps[LUMP_CLIPNODES]);
	Mod_LoadEntities (&header->lumps[LUMP_ENTITIES], brush_fileinfo);	// 2001-09-12 .ENT support by Maddes
	Mod_LoadSubmodels (&header->lumps[LUMP_MODELS]);

	Mod_MakeHull0 ();

	mod->numframes = 2;		// regular and alternate animation
	mod->flags = 0;

//
// set up the submodels (FIXME: this is confusing)
//
	for (i=0 ; i<mod->numsubmodels ; i++)
	{
		bm = &mod->submodels[i];

		mod->hulls[0].firstclipnode = bm->headnode[0];
		for (j=1 ; j<MAX_MAP_HULLS ; j++)
		{
			mod->hulls[j].firstclipnode = bm->headnode[j];
			mod->hulls[j].lastclipnode = mod->numclipnodes-1;
		}

		mod->firstmodelsurface = bm->firstface;
		mod->nummodelsurfaces = bm->numfaces;

		VectorCopy (bm->maxs, mod->maxs);
		VectorCopy (bm->mins, mod->mins);
		mod->radius = RadiusFromBounds (mod->mins, mod->maxs);

		mod->numleafs = bm->visleafs;

		if (i < mod->numsubmodels-1)
		{	// duplicate the basic information
			char	name[10];

			sprintf (name, "*%i", i+1);
			loadmodel = Mod_FindName (name);
			*loadmodel = *mod;
			strcpy (loadmodel->name, name);
			mod = loadmodel;
		}
	}
}

//=============================================================================


/*
================
Mod_Print
================
*/
void Mod_Print (void)
{
	int		i;
	model_t	*mod;

	Con_Printf ("Cached models:\n");
	for (i=0, mod=mod_known ; i < mod_numknown ; i++, mod++)
	{
		Con_Printf ("%8p : %s",mod->cache.data, mod->name);
		if (mod->needload & NL_UNREFERENCED)
			Con_Printf (" (!R)");
		if (mod->needload & NL_NEEDS_LOADED)
			Con_Printf (" (!P)");
		Con_Printf ("\n");
	}
}
