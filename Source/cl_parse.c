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
// cl_parse.c  -- parse a message received from the server

#include "quakedef.h"
#include "ptc_file.h" // mankrip
#include "spr_file.h" // mankrip

int	current_protocol; // mankrip - 16-bit angles

char *svc_strings[] =
{
	"svc_bad",
	"svc_nop",
	"svc_disconnect",
	"svc_updatestat",
	"svc_version",		// [long] server version
	"svc_setview",		// [short] entity number
	"svc_sound",			// <see code>
	"svc_time",			// [float] server time
	"svc_print",			// [string] null terminated string
	"svc_stufftext",		// [string] stuffed into client's console buffer
						// the string should be \n terminated
	"svc_setangle",		// [vec3] set the view angle to this absolute value

	"svc_serverinfo",		// [long] version
						// [string] signon string
						// [string]..[0]model cache [string]...[0]sounds cache
						// [string]..[0]item cache
	"svc_lightstyle",		// [byte] [string]
	"svc_updatename",		// [byte] [string]
	"svc_updatefrags",	// [byte] [short]
	"svc_clientdata",		// <shortbits + data>
	"svc_stopsound",		// <see code>
	"svc_updatecolors",	// [byte] [byte]
	"svc_particle",		// [vec3] <variable>
	"svc_damage",			// [byte] impact [byte] blood [vec3] from

	"svc_spawnstatic",
	"OBSOLETE svc_spawnbinary",
	"svc_spawnbaseline",

	"svc_temp_entity",		// <variable>
	"svc_setpause",
	"svc_signonnum",
	"svc_centerprint",
	"svc_killedmonster",
	"svc_foundsecret",
	"svc_spawnstaticsound",
	"svc_intermission",
	"svc_finale",			// [string] music [string] text
	"svc_cdtrack",			// [byte] track [byte] looptrack
	"svc_sellscreen",
	"svc_cutscene",
	// DarkPlaces - begin
	"",//svc_showlmp",	// [string] iconlabel [string] lmpfile [short] x [short] y
	"",//svc_hidelmp",	// [string] iconlabel
	"",//svc_skybox", // [string] skyname
	"", // 38
	"", // 39
	"", // 40
	"", // 41
	"", // 42
	"", // 43
	"", // 44
	"", // 45
	"", // 46
	"", // 47
	"", // 48
	"", // 49
	"",//svc_cgame", //				50		// [short] length [bytes] data
	"",//svc_unusedlh1", //			51		// unused
	"",//svc_effect", //			52		// [vector] org [byte] modelindex [byte] startframe [byte] framecount [byte] framerate
	"",//svc_effect2", //			53		// [vector] org [short] modelindex [short] startframe [byte] framecount [byte] framerate
	"",//svc_sound2", //			54		// short soundindex instead of byte
	"",//svc_spawnbaseline2", //	55		// short modelindex instead of byte
	"",//svc_spawnstatic2", //		56		// short modelindex instead of byte
	"",//svc_entities", //			57		// [int] deltaframe [int] thisframe [float vector] eye [variable length] entitydata
	"",//svc_unusedlh3", //			58
	"",//svc_spawnstaticsound2", //	59		// [coord3] [short] samp [byte] vol [byte] aten
	// DarkPlaces - end
/*60*/"svc_letterbox",
/*61*/"svc_vibrate",
};

//=============================================================================

/*
===============
CL_EntityNum

This error checks and tracks the total number of entities
===============
*/
entity_t	*CL_EntityNum (int num)
{
	if (num >= cl.num_entities)
	{
		if (num >= MAX_EDICTS)
			Host_Error ("CL_EntityNum: %i is an invalid number",num);
		while (cl.num_entities<=num)
		{
//			cl_entities[cl.num_entities].colormap = vid.colormap;
			cl.num_entities++;
		}
	}

	return &cl_entities[num];
}


/*
==================
CL_ParseStartSoundPacket
==================
*/
void CL_ParseStartSoundPacket(void)
{
    vec3_t  pos;
    int 	channel, ent;
    int 	sound_num;
    int 	volume;
    int 	field_mask;
    float 	attenuation;
 	int		i;

    field_mask = MSG_ReadByte();

    if (field_mask & SND_VOLUME)
		volume = MSG_ReadByte ();
	else
		volume = DEFAULT_SOUND_PACKET_VOLUME;

    if (field_mask & SND_ATTENUATION)
		attenuation = MSG_ReadByte () / 64.0;
	else
		attenuation = DEFAULT_SOUND_PACKET_ATTENUATION;

	channel = MSG_ReadShort ();
	sound_num = MSG_ReadByte ();

	ent = channel >> 3;
	channel &= 7;

	if (ent > MAX_EDICTS)
		Host_Error ("CL_ParseStartSoundPacket: ent = %i", ent);

	for (i=0 ; i<3 ; i++)
		pos[i] = MSG_ReadCoord ();

    S_StartSound (ent, channel, cl.sound_precache[sound_num], pos, volume/255.0, attenuation);
}

/*
==================
CL_KeepaliveMessage

When the client is taking a long time to load stuff, send keepalive messages
so the server doesn't disconnect.
==================
*/
void CL_KeepaliveMessage (void)
{
	float	time;
	static float lastmsg;
	int		ret;
	sizebuf_t	old;
	byte		olddata[8192];

	if (sv.active)
		return;		// no need if server is local
	if (cls.demoplayback)
		return;

// read messages from server, should just be nops
	old = net_message;
	memcpy (olddata, net_message.data, net_message.cursize);

	do
	{
		ret = CL_GetMessage ();
		switch (ret)
		{
		default:
			Host_Error ("CL_KeepaliveMessage: CL_GetMessage failed");
		case 0:
			break;	// nothing waiting
		case 1:
			Host_Error ("CL_KeepaliveMessage: received a message");
			break;
		case 2:
			if (MSG_ReadByte() != svc_nop)
				Host_Error ("CL_KeepaliveMessage: datagram wasn't a nop");
			break;
		}
	} while (ret);

	net_message = old;
	memcpy (net_message.data, olddata, net_message.cursize);

// check time
	time = Sys_FloatTime ();
	if (time - lastmsg < 5)
		return;
	lastmsg = time;

// write out a nop
	Con_Printf ("--> client to server keepalive\n");

	MSG_WriteByte (&cls.message, clc_nop);
	NET_SendMessage (cls.netcon, &cls.message);
	SZ_Clear (&cls.message);
}

/*
==================
CL_ParseServerInfo
==================
*/
void CL_ParseServerInfo (void)
{
	char	*str;
	int		i;
	int		nummodels, numsounds;
	char	model_precache[MAX_MODELS][MAX_QPATH];
	char	sound_precache[MAX_SOUNDS][MAX_QPATH];

	Con_DPrintf ("Serverinfo packet received.\n");
//
// wipe the client_state_t struct
//
	CL_ClearState ();

// parse protocol version number
	// mankrip - 16-bit angles - edited - begin
	current_protocol = MSG_ReadLong ();
	if((current_protocol != PROTOCOL_VERSION)
	&& (current_protocol != PROTOCOL_VERSION-1)) // added
	{
		Host_Error/*Con_Printf*/ ("Server returned version %i, not %i\n", current_protocol, PROTOCOL_VERSION);
	// mankrip - 16-bit angles - edited - end
		return;
	}

// parse maxclients
	cl.maxclients = MSG_ReadByte ();
	if (cl.maxclients < 1 || cl.maxclients > MAX_SCOREBOARD)
	{
		Con_Printf("Bad maxclients (%u) from server\n", cl.maxclients);
		return;
	}
	cl.scores = Hunk_AllocName (cl.maxclients*sizeof(*cl.scores), "scores");

// parse gametype
	cl.gametype = MSG_ReadByte ();

// parse signon message
	str = MSG_ReadString ();
	strncpy (cl.levelname, str, sizeof(cl.levelname)-1);

// seperate the printfs so the server message can have a color
	Con_Printf("\n\35\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\37\n\n"); // edited
	Con_Printf ("%c%s\n", 2, str);

//
// first we go through and touch all of the precache data that still
// happens to be in the cache, so precaching something else doesn't
// needlessly purge it
//

// precache models
	memset (cl.model_precache, 0, sizeof(cl.model_precache));
	for (nummodels=1 ; ; nummodels++)
	{
		str = MSG_ReadString ();
		if (!str[0])
			break;
		if (nummodels==MAX_MODELS)
		{
			Con_Printf ("Server sent too many model precaches\n");
			return;
		}
		strcpy (model_precache[nummodels], str);
		Mod_TouchModel (str);
	}

// precache sounds
	memset (cl.sound_precache, 0, sizeof(cl.sound_precache));
	for (numsounds=1 ; ; numsounds++)
	{
		str = MSG_ReadString ();
		if (!str[0])
			break;
		if (numsounds==MAX_SOUNDS)
		{
			Con_Printf ("Server sent too many sound precaches\n");
			return;
		}
		strcpy (sound_precache[numsounds], str);
		S_TouchSound (str);
	}

//
// now we try to load everything else until a cache allocation fails
//

	for (i=1 ; i<nummodels ; i++)
	{
		cl.model_precache[i] = Mod_ForName (model_precache[i], false);
		if (cl.model_precache[i] == NULL)
		{
			Con_Printf("Model %s not found\n", model_precache[i]);
			return;
		}
		CL_KeepaliveMessage ();
	}

	S_BeginPrecaching ();
	for (i=1 ; i<numsounds ; i++)
	{
		cl.sound_precache[i] = S_PrecacheSound (sound_precache[i]);
		CL_KeepaliveMessage ();
	}
	S_EndPrecaching ();


// local state
	cl_entities[0].model = cl.worldmodel = cl.model_precache[1];
	cl_entities[0].colormap = colorshadingmap_world;
	vibration_update[0] = vibration_update[1] = false; // mankrip

	R_NewMap ();

	Hunk_Check ();		// make sure nothing is hurt

	noclip_anglehack = false;		// noclip is turned off at start
}


/*
==================
CL_ParseUpdate

Parse an entity update message from the server
If an entities model or origin changes from frame to frame, it must be
relinked.  Other attributes can change without relinking.
==================
*/
int	bitcounts[16];

void CL_ParseUpdate (int bits)
{
	int			i;
	model_t		*model;
	int			modnum;
	qboolean	forcelink;
	entity_t	*ent;
	int			num;
#ifdef GLQUAKE
	int			skin;
#endif

	if (cls.signon == SIGNONS - 1)
	{	// first update is the final signon stage
		cls.signon = SIGNONS;
		CL_SignonReply ();
	}

	if (bits & U_MOREBITS)
	{
		i = MSG_ReadByte ();
		bits |= (i<<8);
	}

	// Tomaz - QC Control Begin
	if (bits & U_EXTEND1)
		bits |= MSG_ReadByte() << 16;
	// Tomaz - QC Control End
	// mankrip - begin
	if (bits & U_EXTEND2)
		bits |= MSG_ReadByte() << 24;
	// mankrip - end

	num = (bits & U_LONGENTITY) ? MSG_ReadShort () : MSG_ReadByte ();

	ent = CL_EntityNum (num);

	for (i=0 ; i<16 ; i++)
		if (bits&(1<<i))
			bitcounts[i]++;

	forcelink = (ent->msgtime != cl.mtime[1]);	// if true, no previous frame to lerp from

	ent->msgtime = cl.mtime[0];

	if (bits & U_MODEL)
	{
		modnum = MSG_ReadByte ();
		if (modnum >= MAX_MODELS)
			Host_Error ("CL_ParseModel: bad modnum");
	}
	else
		modnum = ent->baseline.modelindex;

	model = cl.model_precache[modnum];
	if (model != ent->model)
	{
		ent->model = model;
	// automatic animation (torches, etc) can be either all together
	// or randomized
		if (model)
			ent->syncbase = (model->synctype == ST_RAND) ? (float) (rand ()&0x7fff) / 0x7fff : 0.0;
		else
			forcelink = true;	// hack to make null model players work
#ifdef GLQUAKE
		if (num > 0 && num <= cl.maxclients)
			R_TranslatePlayerSkin (num - 1);
#endif
	}

	ent->frame = (bits & U_FRAME) ? MSG_ReadByte () : ent->baseline.frame;

	i = (bits & U_COLORMAP) ? MSG_ReadByte () : ent->baseline.colormap;
	if (!i)
		ent->colormap = (model->name[0] == '*') ? colorshadingmap_world : colorshadingmap_entity;//vid.colormap; // mankrip - edited
	else
	{
		if (i > cl.maxclients)
			Sys_Error ("i >= cl.maxclients");
		ent->colormap = cl.scores[i-1].translations;
	}

#ifdef GLQUAKE
	skin = (bits & U_SKIN) ? MSG_ReadByte() : ent->baseline.skin;
	if (skin != ent->skinnum)
	{
		ent->skinnum = skin;
		if (num > 0 && num <= cl.maxclients)
			R_TranslatePlayerSkin (num - 1);
	}

#else

	ent->skinnum = (bits & U_SKIN) ? MSG_ReadByte () : ent->baseline.skin;
#endif

	ent->effects = (bits & U_EFFECTS) ? MSG_ReadByte () : ent->baseline.effects;

	// shift the known values for interpolation
	VectorCopy (ent->msg_origins[0], ent->msg_origins[1]);
	VectorCopy (ent->msg_angles [0], ent->msg_angles [1]);

	ent->msg_origins[0][0] = (bits & U_ORIGIN1) ? MSG_ReadCoord () : ent->baseline.origin[0];
	ent->msg_angles [0][0] = (bits & U_ANGLE1 ) ? MSG_ReadAngle () : ent->baseline.angles[0];

	ent->msg_origins[0][1] = (bits & U_ORIGIN2) ? MSG_ReadCoord () : ent->baseline.origin[1];
	ent->msg_angles [0][1] = (bits & U_ANGLE2 ) ? MSG_ReadAngle () : ent->baseline.angles[1];

	ent->msg_origins[0][2] = (bits & U_ORIGIN3) ? MSG_ReadCoord () : ent->baseline.origin[2];
	ent->msg_angles [0][2] = (bits & U_ANGLE3 ) ? MSG_ReadAngle () : ent->baseline.angles[2];
	// Tomaz - QC Alpha Scale Glow Begin
	ent->alpha = (bits & U_ALPHA) ? MSG_ReadFloat() : 1.0f;
	ent->scale2 = (bits & U_SCALE) ? MSG_ReadFloat() : 1.0f;

	if (bits & U_SCALEV)
	{
		for (i=0 ; i<3 ; i++)
			ent->scalev[i] = MSG_ReadFloat ();
	}
	else
		ent->scalev[0] = ent->scalev[1] = ent->scalev[2] = 1.0f;

	ent->glow_size = (bits & U_GLOW_SIZE) ? MSG_ReadFloat() : 0;
	/*
	ent->glow_red	= (bits & U_GLOW_RED	) ? MSG_ReadFloat() : 0;
	ent->glow_green	= (bits & U_GLOW_GREEN	) ? MSG_ReadFloat() : 0;
	ent->glow_blue	= (bits & U_GLOW_BLUE	) ? MSG_ReadFloat() : 0;
	*/
	// Tomaz - QC Alpha Scale Glow End
	// mankrip - QC frame_interval - begin
//	ent->frame_interval = (bits & U_FRAMEINTERVAL) ? MSG_ReadFloat() : = 0.1f;
	ent->frame_interval = (bits & U_FRAMEINTERVAL) ? MSG_ReadFloat() : 0.1f;
	// mankrip - QC frame_interval - end
	// mankrip - begin
	if (bits & U_EFFECTS2)
		ent->effects = (ent->effects & 0x00FF) | (MSG_ReadByte() << 8); // DarkPlaces
	// TODO: move these to a function  to be called here and in CL_ParseStatic
	if (r_sprite_addblend.value && ent->model->type == mod_sprite)
		ent->effects |= EF_ADDITIVE;
	ent->colorblendingmap = colorblendingmode[ (ent->effects & EF_ADDITIVE) ? BLEND_ADDITIVE : BLEND_WEIGHTEDMEAN];
	if (ent->effects & EF_SHADOW)
	{
		ent->D_PolysetDrawSpans = MDL_DrawSpans_Shadow;
		ent->D_SpriteDrawSpans = D_SpriteDrawSpans_Shadow;
		ent->D_DrawSpans = D_DrawSpans16_Blend;
		ent->D_DrawTurbulent = D_DrawTurbulent16_Blend;
	}
	else if (ent->alpha < 0.5f)
	{
		ent->D_PolysetDrawSpans = MDL_DrawSpans_BlendBackwards;
		ent->D_SpriteDrawSpans = D_SpriteDrawSpans_BlendBackwards;
		ent->D_DrawSpans = D_DrawSpans16_BlendBackwards;
		ent->D_DrawTurbulent = D_DrawTurbulent16_BlendBackwards;
	}
	else if (ent->alpha < 1.0f)
	{
		ent->D_PolysetDrawSpans = MDL_DrawSpans_Blend;
		ent->D_SpriteDrawSpans = D_SpriteDrawSpans_Blend;
		ent->D_DrawSpans = D_DrawSpans16_Blend;
		ent->D_DrawTurbulent = D_DrawTurbulent16_Blend;
	}
	else if (ent->effects & EF_ADDITIVE)
	{
		ent->D_PolysetDrawSpans = MDL_DrawSpans_Blend;
		ent->D_SpriteDrawSpans = D_SpriteDrawSpans_Blend;
		ent->D_DrawSpans = D_DrawSpans16_Blend;
		ent->D_DrawTurbulent = D_DrawTurbulent16_Blend;
	}
	else if (ent->effects & (EF_CELSHADING | EF_REFLECTIVE))
	{
		ent->alpha = 1.0f;
		i = (model->name[0] == '*') ? (int)r_brights_world.value : ( (num > 0 && num <= cl.maxclients) ? (int)r_brights_player.value : (int)r_brights_entity.value);
		if (ent->effects & EF_CELSHADING)
			ent->colormap = &colorshadingmap[i ? CEL_WITH_BRIGHTS : CEL_WITHOUT_BRIGHTS][256];
		if (ent->effects & EF_REFLECTIVE)
			ent->colormap = &colorshadingmap[i ? METAL_WITH_BRIGHTS : METAL_WITHOUT_BRIGHTS][256];
		ent->D_PolysetDrawSpans = MDL_DrawSpans_ColorKeyed;

		#if 1
		if (ent->effects & EF_REFLECTIVE)
		{
			ent->D_PolysetDrawSpans = MDL_DrawSpans_Metal;
			ent->colormap = &colorshadingmap[WITH_BRIGHTS][256];
		}
		#endif

		ent->D_SpriteDrawSpans = D_SpriteDrawSpans16_ColorKeyed;
		ent->D_DrawSpans = D_DrawSpans16_ColorKeyed;
		ent->D_DrawTurbulent = D_DrawTurbulent16_Solid;
	}
	else
	{
		ent->alpha = 1.0f;
		ent->D_PolysetDrawSpans = MDL_DrawSpans_ColorKeyed;
		ent->D_SpriteDrawSpans = D_SpriteDrawSpans16_ColorKeyed;
		ent->D_DrawSpans = D_DrawSpans16_ColorKeyed;
		ent->D_DrawTurbulent = D_DrawTurbulent16_Solid;
	}
	// mankrip - end
	if ( bits & U_NOLERP )
		ent->forcelink = true;

	if ( forcelink )
	{	// didn't have an update last message
		VectorCopy (ent->msg_origins[0], ent->msg_origins[1]);
		VectorCopy (ent->msg_origins[0], ent->origin);
		VectorCopy (ent->msg_angles[0], ent->msg_angles[1]);
		VectorCopy (ent->msg_angles[0], ent->angles);
		ent->forcelink = true;
	}
}

/*
==================
CL_ParseBaseline
==================
*/
void CL_ParseBaseline (entity_t *ent)
{
	int			i;

	ent->baseline.modelindex = MSG_ReadByte ();
	ent->baseline.frame = MSG_ReadByte ();
	ent->baseline.colormap = MSG_ReadByte();
	ent->baseline.skin = MSG_ReadByte();
	for (i=0 ; i<3 ; i++)
	{
		ent->baseline.origin[i] = MSG_ReadCoord ();
		ent->baseline.angles[i] = MSG_ReadAngle ();
	}
}


/*
==================
CL_ParseClientdata

Server information pertaining to this client only
==================
*/
void CL_ParseClientdata (int bits)
{
	int		i, j;
	float newpunchangle, oldpunchangle = Length(cl.punchangle); // mankrip

	if (bits & SU_VIEWHEIGHT)
		cl.viewheight = MSG_ReadChar ();
	else
		cl.viewheight = DEFAULT_VIEWHEIGHT;

	if (bits & SU_IDEALPITCH)
		cl.idealpitch = MSG_ReadChar ();
	else
		cl.idealpitch = 0;

	VectorCopy (cl.mvelocity[0], cl.mvelocity[1]);
	for (i=0 ; i<3 ; i++)
	{
		if (bits & (SU_PUNCH1<<i) )
		// mankrip - 16-bit angles - begin
		{
			if (current_protocol == PROTOCOL_VERSION-1)
				cl.punchangle[i] = MSG_ReadAngle();
			else
		// mankrip - 16-bit angles - end
			cl.punchangle[i] = MSG_ReadChar();
		} // mankrip - 16-bit angles
		else
			cl.punchangle[i] = 0;
		if (bits & (SU_VELOCITY1<<i) )
			cl.mvelocity[0][i] = MSG_ReadChar()*16;
		else
			cl.mvelocity[0][i] = 0;
	}
	// mankrip - begin
	newpunchangle = Length(cl.punchangle);
	if (newpunchangle && !cl.intermission)
	{
		if (oldpunchangle <= newpunchangle && cl_vibration.value != 2) // increasing
		{
#ifdef _arch_dreamcast
//			extern cvar_t gamecfg;
			extern byte dc_vibe[2][4];
			newpunchangle = fabs(newpunchangle);
		/*	if (newpunchangle < 2.0)
			{
		//		if (!oldpunchangle) // make the first vibration stronger
					newpunchangle += 0.1;
			}
			else*/ if (newpunchangle > 7.0)
				newpunchangle = 7.0;
			dc_vibe[0][0] = (1 << 4); // PURUPURU_SPECIAL_MOTOR1
			dc_vibe[0][1] = ((int)newpunchangle << 4); // PURUPURU_EFFECT1_INTENSITY
			dc_vibe[0][2] = 0;
			dc_vibe[0][3] = 6;//(byte)gamecfg.value;//0x10;
//			if ((int)developer.value & 1)
//				dc_vibe[0][0] |= 1; // | PURUPURU_SPECIAL_PULSE
//			if ((int)developer.value & 2)
				dc_vibe[0][1] |= (8 << 4); // | PURUPURU_EFFECT1_PULSE
//			if ((int)developer.value & 4)
//				dc_vibe[0][2] |= ((int)newpunchangle); // PURUPURU_EFFECT2_LINTENSITY
//			if ((int)developer.value & 8)
				dc_vibe[0][2] |= ((int)newpunchangle << 4); // | PURUPURU_EFFECT2_UINTENSITY
//			if ((int)developer.value & 16)
				dc_vibe[0][2] |= 8; // | PURUPURU_EFFECT2_PULSE
//			if ((int)developer.value & 32)
//				dc_vibe[0][2] |= (8 << 4); // | PURUPURU_EFFECT2_DECAY
#endif
			vibration_update[0] = true;
//			Con_Printf("%f\n", newpunchangle);
		}
	}
//	else if (oldpunchangle && cl_vibration.value != 2) // just stopped
//	if (!developer.value)
//		Vibration_Stop (0);
	// mankrip - end

// [always sent]	if (bits & SU_ITEMS)
		i = MSG_ReadLong ();

	if (cl.items != i)
	{	// set flash times
		Sbar_Changed ();
		for (j=0 ; j<32 ; j++)
			if ( (i & (1<<j)) && !(cl.items & (1<<j)))
				cl.item_gettime[j] = cl.time;
		cl.items = i;
	}

	cl.onground = (bits & SU_ONGROUND) != 0;
	cl.inwater = (bits & SU_INWATER) != 0;

	if (bits & SU_WEAPONFRAME)
		cl.stats[STAT_WEAPONFRAME] = MSG_ReadByte ();
	else
		cl.stats[STAT_WEAPONFRAME] = 0;

	if (bits & SU_ARMOR)
		i = MSG_ReadByte ();
	else
		i = 0;
	if (cl.stats[STAT_ARMOR] != i)
	{
		cl.stats[STAT_ARMOR] = i;
		Sbar_Changed ();
	}

	if (bits & SU_WEAPON)
		i = MSG_ReadByte ();
	else
		i = 0;
	if (cl.stats[STAT_WEAPON] != i)
	{
		cl.stats[STAT_WEAPON] = i;
		Sbar_Changed ();
	}

	i = MSG_ReadShort ();
	if (cl.stats[STAT_HEALTH] != i)
	{
		cl.stats[STAT_HEALTH] = i;
		Sbar_Changed ();
	}

	// mankrip - high values in the status bar - begin
	if(current_protocol == PROTOCOL_VERSION-1)
		i = MSG_ReadShort ();
	else
	// mankrip - high values in the status bar - end
	i = MSG_ReadByte ();
	if (cl.stats[STAT_AMMO] != i)
	{
		cl.stats[STAT_AMMO] = i;
		Sbar_Changed ();
	}

	for (i=0 ; i<4 ; i++)
	{
		// mankrip - high values in the status bar - begin
		if(current_protocol == PROTOCOL_VERSION-1)
			j = MSG_ReadShort ();
		else
		// mankrip - high values in the status bar - end
		j = MSG_ReadByte ();
		if (cl.stats[STAT_SHELLS+i] != j)
		{
			cl.stats[STAT_SHELLS+i] = j;
			Sbar_Changed ();
		}
	}

	i = MSG_ReadByte ();

	if (standard_quake)
	{
		if (cl.stats[STAT_ACTIVEWEAPON] != i)
		{
			cl.stats[STAT_ACTIVEWEAPON] = i;
			Sbar_Changed ();
		}
	}
	else
	{
		if (cl.stats[STAT_ACTIVEWEAPON] != (1<<i))
		{
			cl.stats[STAT_ACTIVEWEAPON] = (1<<i);
			Sbar_Changed ();
		}
	}
}

/*
=====================
CL_NewTranslation
=====================
*/
void CL_NewTranslation (int slot)
{
	int		i;
	int		top, bottom;
	byte	*dest, *source;

	if (slot > cl.maxclients)
		Sys_Error ("CL_NewTranslation: slot > cl.maxclients");
	dest = cl.scores[slot].translations;
	source = colorshadingmap_player;
	memcpy (dest, colorshadingmap_player, sizeof(cl.scores[slot].translations));
	top = cl.scores[slot].colors & 0xf0;
	bottom = (cl.scores[slot].colors &15)<<4;
#ifdef GLQUAKE
	R_TranslatePlayerSkin (slot);
#endif

	for (i=0 ; i<VID_GRADES ; i++, dest += 256, source+=256)
	{
		memcpy (dest + TOP_RANGE, source + top, 16); // mankrip
		memcpy (dest + BOTTOM_RANGE, source + bottom, 16); // mankrip
	}
}

/*
=====================
CL_ParseStatic
=====================
*/
void CL_ParseStatic (void)
{
	entity_t *ent;
	int		i;

	i = cl.num_statics;
	if (i >= MAX_STATIC_ENTITIES)
		Host_Error ("Too many static entities");
	ent = &cl_static_entities[i];
	cl.num_statics++;
	CL_ParseBaseline (ent);

// copy it to the current state
	ent->model = cl.model_precache[ent->baseline.modelindex];
	ent->frame = ent->baseline.frame;
	ent->colormap = (ent->model->name[0] == '*') ? colorshadingmap_world : colorshadingmap_staticent;//vid.colormap; // mankrip - edited
	ent->skinnum = ent->baseline.skin;
	ent->effects = ent->baseline.effects;
	// mankrip - model interpolation - begin
	ent->pose1 = ent->pose2 = ent->frame;
	ent->frame_start_time = cl.time;
	ent->translate_start_time = 0;
	ent->rotate_start_time = 0;
	// mankrip - model interpolation - end
	// mankrip - QC Alpha Scale - begin
	if (current_protocol == PROTOCOL_VERSION-1)
	{
		int		bits;
		bits = MSG_ReadLong();
//		if (GetEdictFieldValue(G_EDICT(OFS_PARM0), "alpha"))
		if (bits & U_ALPHA)
			ent->alpha = MSG_ReadFloat ();
//		if (GetEdictFieldValue(G_EDICT(OFS_PARM0), "scale"))
		if (bits & U_SCALE)
			ent->scale2 = MSG_ReadFloat ();
//		if (GetEdictFieldValue(G_EDICT(OFS_PARM0), "scalev"))
		if (bits & U_SCALEV)
			for (i=0 ; i<3 ; i++)
				ent->scalev[i] = MSG_ReadFloat ();
	//	FIXME: make self.glow_size work
//		if (GetEdictFieldValue(G_EDICT(OFS_PARM0), "glow_size"))
		if (bits & U_GLOW_SIZE)
			ent->glow_size = MSG_ReadFloat ();
		ent->effects = MSG_ReadShort();

		if (ent->alpha <= 0)
			ent->alpha = 1.0f;
		else if (ent->alpha > 1.0f)
			ent->alpha = 1.0f;

		if (ent->scale2 <= 0)
			ent->scale2 = 1.0f;
		if (!ent->scalev[0] && !ent->scalev[1] && !ent->scalev[2])
			ent->scalev[0] = ent->scalev[1] = ent->scalev[2] = 1.0f;

		if (ent->glow_size > 250)
			ent->glow_size = 250;
		else if (ent->glow_size < -250)
			ent->glow_size = -250;
	}
	else
	{
		ent->glow_size = 0;
		ent->alpha = 1.0f;
		ent->scale2 = 1.0f;
		ent->scalev[0] = ent->scalev[1] = ent->scalev[2] = 1.0f;
	}
	// TODO: move these to a function  to be called here and in CL_ParseUpdate
	if (r_sprite_addblend.value && ent->model->type == mod_sprite)
		ent->effects |= EF_ADDITIVE;
	ent->colorblendingmap = colorblendingmode[ (ent->effects & EF_ADDITIVE) ? BLEND_ADDITIVE : BLEND_WEIGHTEDMEAN];
	if (ent->effects & EF_SHADOW)
	{
		ent->D_PolysetDrawSpans = MDL_DrawSpans_Shadow;
		ent->D_SpriteDrawSpans = D_SpriteDrawSpans_Shadow;
		ent->D_DrawSpans = D_DrawSpans16_Blend;
		ent->D_DrawTurbulent = D_DrawTurbulent16_Blend;
	}
	else if (ent->alpha < 0.5f)
	{
		ent->D_PolysetDrawSpans = MDL_DrawSpans_BlendBackwards;
		ent->D_SpriteDrawSpans = D_SpriteDrawSpans_BlendBackwards;
		ent->D_DrawSpans = D_DrawSpans16_BlendBackwards;
		ent->D_DrawTurbulent = D_DrawTurbulent16_BlendBackwards;
	}
	else if (ent->alpha < 1.0f)
	{
		ent->D_PolysetDrawSpans = MDL_DrawSpans_Blend;
		ent->D_SpriteDrawSpans = D_SpriteDrawSpans_Blend;
		ent->D_DrawSpans = D_DrawSpans16_Blend;
		ent->D_DrawTurbulent = D_DrawTurbulent16_Blend;
	}
	else if (ent->effects & EF_ADDITIVE)
	{
		ent->D_PolysetDrawSpans = MDL_DrawSpans_Blend;
		ent->D_SpriteDrawSpans = D_SpriteDrawSpans_Blend;
		ent->D_DrawSpans = D_DrawSpans16_Blend;
		ent->D_DrawTurbulent = D_DrawTurbulent16_Blend;
	}
	else if (ent->effects & EF_REFLECTIVE)
	{
		ent->alpha = 1.0f;
		ent->D_PolysetDrawSpans = MDL_DrawSpans_Metal;
		ent->D_SpriteDrawSpans = D_SpriteDrawSpans16_ColorKeyed;
		ent->D_DrawSpans = D_DrawSpans16_ColorKeyed;
		ent->D_DrawTurbulent = D_DrawTurbulent16_Solid;
	}
	else
	{
		ent->alpha = 1.0f;
		ent->D_PolysetDrawSpans = MDL_DrawSpans_ColorKeyed;
		ent->D_SpriteDrawSpans = D_SpriteDrawSpans16_ColorKeyed;
		ent->D_DrawSpans = D_DrawSpans16_ColorKeyed;
		ent->D_DrawTurbulent = D_DrawTurbulent16_Solid;
	}
	// mankrip - QC Alpha Scale - end

	VectorCopy (ent->baseline.origin, ent->origin);
	VectorCopy (ent->baseline.angles, ent->angles);
	R_AddEfrags (ent);
}

/*
===================
CL_ParseStaticSound
===================
*/
void CL_ParseStaticSound (void)
{
	vec3_t		org;
	int			sound_num, vol, atten;
	int			i;

	for (i=0 ; i<3 ; i++)
		org[i] = MSG_ReadCoord ();
	sound_num = MSG_ReadByte ();
	vol = MSG_ReadByte ();
	atten = MSG_ReadByte ();

	S_StaticSound (cl.sound_precache[sound_num], org, vol, atten);
}


#define SHOWNET(x) if(cl_shownet.value==2)Con_Printf ("%3i:%s\n", msg_readcount-1, x);

extern float previous_chasecam_length; // mankrip
/*
=====================
CL_ParseServerMessage
=====================
*/
void CL_ParseServerMessage (void)
{
	int			cmd;
	int			i;

//
// if recording demos, copy the message out
//
	if (cl_shownet.value == 1)
		Con_Printf ("%i ",net_message.cursize);
	else if (cl_shownet.value == 2)
		Con_Printf ("------------------\n");

	cl.onground = false;	// unless the server says otherwise
//
// parse the message
//
	MSG_BeginReading ();

	while (1)
	{
		if (msg_badread)
			Host_Error ("CL_ParseServerMessage: Bad server message");

		cmd = MSG_ReadByte ();

		if (cmd == -1)
		{
			SHOWNET("END OF MESSAGE");
			return;		// end of message
		}

	// if the high bit of the command byte is set, it is a fast update
		if (cmd & 128)
		{
			SHOWNET("fast update");
			CL_ParseUpdate (cmd&127);
			continue;
		}

		SHOWNET(svc_strings[cmd]);

	// other commands
		switch (cmd)
		{
		default:
			Host_Error ("CL_ParseServerMessage: Illegible server message\n");
			break;

		case svc_nop:
//			Con_Printf ("svc_nop\n");
			break;

		case svc_time:
			cl.mtime[1] = cl.mtime[0];
			cl.mtime[0] = MSG_ReadFloat ();
			break;

		case svc_clientdata:
			i = MSG_ReadShort ();
			CL_ParseClientdata (i);
			break;

		case svc_version:
			// mankrip - 16-bit angles - edited - begin
			current_protocol = MSG_ReadLong ();
			if((current_protocol != PROTOCOL_VERSION)
			&& (current_protocol != PROTOCOL_VERSION-1)) // added
				Host_Error ("CL_ParseServerMessage: Server is protocol %i instead of %i\n", current_protocol, PROTOCOL_VERSION);
			// mankrip - 16-bit angles - edited - end
			break;

		case svc_disconnect:
			Host_EndGame ("Server disconnected\n");

		case svc_print:
			Con_Printf ("%s", MSG_ReadString ());
			break;

		case svc_centerprint:
			SCR_CenterPrint (MSG_ReadString ());
			break;

		case svc_stufftext:
			Cbuf_AddText (MSG_ReadString ());
			break;

		case svc_damage:
			V_ParseDamage ();
			break;

		case svc_serverinfo:
			CL_ParseServerInfo ();
			vid.recalc_refdef = true;	// leave intermission full screen
			break;

		case svc_setangle:
			for (i=0 ; i<3 ; i++)
				cl.viewangles[i] = MSG_ReadAngle ();
			break;

		case svc_setview:
			cl.viewentity = MSG_ReadShort ();
			break;

		case svc_lightstyle:
			i = MSG_ReadByte ();
			if (i >= MAX_LIGHTSTYLES)
				Sys_Error ("svc_lightstyle > MAX_LIGHTSTYLES");
			Q_strcpy (cl_lightstyle[i].map,  MSG_ReadString());
			cl_lightstyle[i].length = Q_strlen(cl_lightstyle[i].map);
			// mankrip - begin
			if (cl_lightstyle[i].length)
			{
				int l, a;
				for (a = 0, l = 0 ; l < cl_lightstyle[i].length ; l++)
					a += cl_lightstyle[i].map[l];
				cl_lightstyle[i].average = (byte)( (a / cl_lightstyle[i].length) - 'a');
			}
			// mankrip - end
			break;

		case svc_sound:
			CL_ParseStartSoundPacket();
			break;

		case svc_stopsound:
			i = MSG_ReadShort();
			S_StopSound(i>>3, i&7);
			break;

		case svc_updatename:
			Sbar_Changed ();
			i = MSG_ReadByte ();
			if (i >= cl.maxclients)
				Host_Error ("CL_ParseServerMessage: svc_updatename > MAX_SCOREBOARD");
			strcpy (cl.scores[i].name, MSG_ReadString ());
			break;

		case svc_updatefrags:
			Sbar_Changed ();
			i = MSG_ReadByte ();
			if (i >= cl.maxclients)
				Host_Error ("CL_ParseServerMessage: svc_updatefrags > MAX_SCOREBOARD");
			cl.scores[i].frags = MSG_ReadShort ();
			break;

		case svc_updatecolors:
			Sbar_Changed ();
			i = MSG_ReadByte ();
			if (i >= cl.maxclients)
				Host_Error ("CL_ParseServerMessage: svc_updatecolors > MAX_SCOREBOARD");
			cl.scores[i].colors = MSG_ReadByte ();
			CL_NewTranslation (i);
			break;

		case svc_particle:
			R_ParseParticleEffect ();
			break;

		case svc_spawnbaseline:
			i = MSG_ReadShort ();
			// must use CL_EntityNum() to force cl.num_entities up
			CL_ParseBaseline (CL_EntityNum(i));
			break;
		case svc_spawnstatic:
			CL_ParseStatic ();
			break;
		case svc_temp_entity:
			CL_ParseTEnt ();
			break;

		case svc_setpause:
			{
				cl.paused = MSG_ReadByte ();

				if (cl.paused)
				{
					CDAudio_Pause ();
#ifndef _arch_dreamcast // BlackAura
#ifdef _WIN32
					VID_HandlePause (true);
#endif
#endif // BlackAura
					Vibration_Stop (0); // mankrip
					Vibration_Stop (1); // mankrip
				}
				else
				{
					CDAudio_Resume ();
#ifndef _arch_dreamcast // BlackAura
#ifdef _WIN32
					VID_HandlePause (false);
#endif
#endif // BlackAura
				}
			}
			break;

		case svc_signonnum:
			i = MSG_ReadByte ();
			if (i <= cls.signon)
				Host_Error ("Received signon %i when at %i", i, cls.signon);
			cls.signon = i;
			CL_SignonReply ();
			break;

		case svc_killedmonster:
			cl.stats[STAT_MONSTERS]++;
			break;

		case svc_foundsecret:
			cl.stats[STAT_SECRETS]++;
			break;

		case svc_updatestat:
			i = MSG_ReadByte ();
			if (i < 0 || i >= MAX_CL_STATS)
				Sys_Error ("svc_updatestat: %i is invalid", i);
			cl.stats[i] = MSG_ReadLong ();;
			break;

		case svc_spawnstaticsound:
			CL_ParseStaticSound ();
			break;

		case svc_cdtrack:
			cl.cdtrack = MSG_ReadByte ();
			cl.looptrack = MSG_ReadByte ();
			if ( (cls.demoplayback || cls.demorecording) && (cls.forcetrack != -1) )
				CDAudio_Play ((byte)cls.forcetrack, true);
			else
				CDAudio_Play ((byte)cl.cdtrack, true);
			break;

		case svc_intermission:
			previous_chasecam_length = 0.0f; // mankrip
			cl.letterbox = 0; // mankrip - svc_letterbox
			cl.intermission = 1;
			cl.completed_time = cl.time;
			vid.recalc_refdef = true;	// go to full screen
			break;

		case svc_finale:
			previous_chasecam_length = 0.0f; // mankrip
			cl.letterbox = 0; // mankrip - svc_letterbox
			cl.intermission = 2;
			cl.completed_time = cl.time;
			vid.recalc_refdef = true;	// go to full screen
			SCR_CenterPrint (MSG_ReadString ());
			break;

		case svc_cutscene:
			previous_chasecam_length = 0.0f; // mankrip
			cl.letterbox = 0; // mankrip - svc_letterbox
			cl.intermission = 3;
			cl.completed_time = cl.time;
			vid.recalc_refdef = true;	// go to full screen
			SCR_CenterPrint (MSG_ReadString ());
			break;

		case svc_sellscreen:
			Cmd_ExecuteString ("help", src_command);
			break;

		// mankrip - svc_letterbox - begin
		case svc_letterbox:
			cl.letterbox = (float)(MSG_ReadByte ())/100.0;
			if (cl.letterbox < 0) cl.letterbox = 0;
			if (cl.letterbox > 1) cl.letterbox = 1;
			if (cl.letterbox)
				cl.intermission = 0;
			vid.recalc_refdef = true;
			break;
		// mankrip - svc_letterbox - end

		// mankrip - begin
		case svc_vibrate:
			{
			byte s, e1, e2, d;
			int player;
			s = MSG_ReadByte ();
			e1 = MSG_ReadByte ();
			e2 = MSG_ReadByte ();
			d = MSG_ReadByte ();
			player = MSG_ReadByte ();
			if (player > 1)
				break; // invalid player
#ifdef _arch_dreamcast
			{
			extern byte dc_vibe[2][4];
			dc_vibe[player][0] = s;
			dc_vibe[player][1] = e1;
			dc_vibe[player][2] = e2;
			dc_vibe[player][3] = d;
			}
#endif
			vibration_update[player] = true;
			}
			break;
		// mankrip - end
		}
	}
}

