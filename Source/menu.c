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
#include "quakedef.h"
#include "croshair.h" // mankrip

#ifdef _arch_dreamcast // BlackAura
#define	NET_MENUS 0
#else // BlackAura
#ifdef _WIN32
#include "winquake.h"
#define	NET_MENUS 1
#else
#define	NET_MENUS 0
#endif
#endif // BlackAura

// mankrip - begin
#ifdef _arch_dreamcast
extern void VMU_FindConfig (void);
cvar_t	vmupath = {"vmupath","a1"};
void M_SelectVMU_f (int menu);
void SetVMUpath ()
{
	char	*s;
	// backup old string
	s = Z_Malloc (Q_strlen(vmupath.string)+1);
	Q_strcpy (s, vmupath.string);
	// free the old value string
	Z_Free (vmupath.string);

	vmupath.string = Z_Malloc (2+1);
	if (Q_strlen(s) == 2)
	{
		if (s[0] >= 'a' && s[0] <= 'd')
			vmupath.string[0] = s[0];
		else if (s[0] >= 'A' && s[0] <= 'D')
			vmupath.string[0] = s[0]+('a'-'A');
		else
			vmupath.string[0] = 'a';

		if (s[1] == '1' || s[1] == '2')
			vmupath.string[1] = s[1];
		else
			vmupath.string[1] = '1';
	}
	else
	{
		vmupath.string[0] = 'a';
		vmupath.string[1] = '1';
	}
	vmupath.string[2] = 0; // null termination
	vmupath.value = 0;
	Z_Free(s);
}
#endif
cvar_t	savename = {"savename","QUAKE___"}; // 8 uppercase characters
void SetSavename ()
{
	// savename must always be 8 uppercase alphanumeric characters
	// non-alphanumeric characters (for example, spaces) must be converted to underscores
	int		i;
	char	*s;
	// backup old string
	s = Z_Malloc (Q_strlen(savename.string)+1);
	Q_strcpy (s, savename.string);
	// free the old value string
	Z_Free (savename.string);

	savename.string = Z_Malloc (8+1);
	for (i=0; i<8; i++)
	{
		if (!s[i]) // string was shorter than 8 characters
			break;
		if ((s[i] >= '0' && s[i] <= '9') || (s[i] >= 'A' && s[i] <= 'Z'))
			savename.string[i] = s[i];
		else if (s[i] >= 'a' && s[i] <= 'z')
			savename.string[i] = s[i]-('a'-'A');
		else
			savename.string[i] = '_';
	}
	for (; i<8; i++) // fill the remaining characters, if the string was shorter than 8 characters
		savename.string[i] = '_';
	savename.string[i] = 0; // null termination
	savename.value = 0;
	Z_Free(s);
}
cvar_t	help_pages = {"help_pages","6"};

extern cvar_t
	samelevel

,	sv_aim_h
,	sv_aim
,	scr_ofsy
,	crosshair
,	crosshair_color
,	r_drawviewmodel
,	cl_nobob
,	sv_idealpitchscale
,	sv_enable_use_button
,	chase_active
,	chase_back
,	chase_up
,	chase_speed

,	r_stereo_separation
,	r_drawentities
,	r_fullbright
,	r_waterwarp
,	d_mipscale
,	gl_polyblend
,	r_sprite_addblend
,	r_sprite_lit
,	r_externbsp_lit
	;
/* // mankrip - removed - begin
cvar_t	axis_x_function;
cvar_t	axis_y_function;
cvar_t	axis_l_function;
cvar_t	axis_r_function;
*/ // mankrip - removed - end
cvar_t	axis_x_scale;
cvar_t	axis_y_scale;
cvar_t	axis_l_scale;
cvar_t	axis_r_scale;
cvar_t	axis_x_threshold;
cvar_t	axis_y_threshold;
cvar_t	axis_l_threshold;
cvar_t	axis_r_threshold;
/*/// mankrip
extern	cvar_t	axis_pitch_dz;
extern	cvar_t	axis_yaw_dz;
extern	cvar_t	axis_walk_dz;
extern	cvar_t	axis_strafe_dz;
/*/// mankrip

extern	cvar_t	snd_stereo;
extern	cvar_t	snd_swapstereo;

extern	cvar_t	sbar_show_scores;
extern	cvar_t	sbar_show_ammolist;
extern	cvar_t	sbar_show_weaponlist;
extern	cvar_t	sbar_show_keys;
extern	cvar_t	sbar_show_runes;
extern	cvar_t	sbar_show_powerups;
extern	cvar_t	sbar_show_armor;
extern	cvar_t	sbar_show_health;
extern	cvar_t	sbar_show_ammo;
extern	cvar_t	sbar_show_bg;
extern	cvar_t	sbar;

extern	cvar_t	scr_left;
extern	cvar_t	scr_right;
extern	cvar_t	scr_top;
extern	cvar_t	scr_bottom;

void Host_WriteConfiguration (void);
// mankrip - end

void (*vid_menudrawfn)(void);
void (*vid_menukeyfn)(int key);

enum {
m_none=0,
m_main,
m_singleplayer, m_load, m_save, m_loadsmall, m_savesmall,
m_gameoptions,
m_options,
m_setup,
m_keys,
m_controller, m_mouse,
m_gameplay,
m_display, m_video, m_videomodes, m_videosize,
m_audio,
m_developer,
m_credits, m_help, m_popup,
#if NET_MENUS // mankrip - removed multiplayer menus
m_multiplayer, m_net, m_serialconfig, m_modemconfig, m_lanconfig,
m_search, m_slist,
#endif
// mankrip - VMU saves - begin
#ifdef _arch_dreamcast
m_vmu,
m_saveoptions, // hack
m_loadoptions, // hack
#endif
// mankrip - VMU saves - end
m_fightoon,
m_nummenus // mankrip - replaced cursor variables
} m_state; // Edited by mankrip

int m_cursor[m_nummenus]; // mankrip - replaced cursor variables

void M_Off (void); // mankrip
void M_Main_f (void);
void M_Fightoon_f (void);
	void M_SinglePlayer_f (void);
		void M_LoadSmall_f (void); // mankrip - small saves
		void M_SaveSmall_f (void); // mankrip - small saves
		void M_Load_f (void);
		void M_Save_f (void);
		// mankrip - VMU saves - begin
#ifdef _arch_dreamcast
		void M_VMUsave_f (void);
		void M_VMUsavesmall_f (void);
		void M_VMUload_f (void);
		void M_VMUloadsmall_f (void);
#endif
		// mankrip - VMU saves - end
	void M_GameOptions_f (void);
	void M_Options_f (void);
		void M_Setup_f (void);
		void M_Keys_f (void);
		void M_Controller_f (void);
		void M_Mouse_f (void);
		void M_Gameplay_f (void);
		void M_Display_f (void); // mankrip
		void M_Video_f (void);
		void M_VideoSize_f (void); // mankrip
		void M_VideoModes_f (void); // mankrip - for Windows version only
		void M_Audio_f (void);
		void M_Developer_f (void);
	void M_Help_f (void);
	void M_Quit_f (void);
#if NET_MENUS // mankrip - removed multiplayer menus
	void M_MultiPlayer_f (void);
		void M_Net_f (void);
void M_SerialConfig_f (void);
	void M_ModemConfig_f (void);
void M_LanConfig_f (void);
void M_Search_f (void);
void M_ServerList_f (void);
#endif

void M_Main_Draw (void);
void M_Fightoon_Draw (void);
	void M_SinglePlayer_Draw (void);
		void M_Save_Draw (void);
	void M_GameOptions_Draw (void);
	void M_Options_Draw (void);
		void M_Setup_Draw (void);
		void M_Keys_Draw (void);
		void M_Controller_Draw (void);
		void M_Mouse_Draw (void);
		void M_Gameplay_Draw (void);
		void M_Display_Draw (void); // mankrip
		void M_Video_Draw (void);
		void M_VideoSize_Draw (void); // mankrip
		void M_Audio_Draw (void);
		void M_Developer_Draw (void);
	void M_Help_Draw (void);
#if NET_MENUS // mankrip - removed multiplayer menus
void M_MultiPlayer_Draw (void);
	void M_Net_Draw (void);
void M_SerialConfig_Draw (void);
	void M_ModemConfig_Draw (void);
void M_LanConfig_Draw (void);
void M_Search_Draw (void);
void M_ServerList_Draw (void);
#endif

void M_Main_Key (int key);
void M_Fightoon_Key (int key);
	void M_SinglePlayer_Key (int key);
		void M_Save_Key (int key);
	void M_GameOptions_Key (int key);
	void M_Options_Key (int key);
		void M_Setup_Key (int key);
		void M_Keys_Key (int key);
		void M_Controller_Key (int key);
		void M_Mouse_Key (int key);
		void M_Gameplay_Key (int key);
		void M_Display_Key (int key); // mankrip
		void M_Video_Key (int key);
		void M_VideoSize_Key (int key); // mankrip
		void M_Audio_Key (int key);
		void M_Developer_Key (int key);
	void M_Help_Key (int key);
#if NET_MENUS // mankrip - removed multiplayer menus
	void M_MultiPlayer_Key (int key);
		void M_Net_Key (int key);
void M_SerialConfig_Key (int key);
	void M_ModemConfig_Key (int key);
void M_LanConfig_Key (int key);
void M_Search_Key (int key);
void M_ServerList_Key (int key);
#endif

// mankrip - begin
byte	m_inp_up, m_inp_down, m_inp_left, m_inp_right,
		m_inp_ok, m_inp_cancel, m_inp_no, m_inp_yes,
		m_inp_off, m_inp_backspace
		;
void M_CheckInput (int key)
{
	m_inp_up = m_inp_down = m_inp_left = m_inp_right =
	m_inp_ok = m_inp_cancel = m_inp_no = m_inp_yes =
	m_inp_off = m_inp_backspace = false;

	switch (key)
	{
	case K_UPARROW:
	case K_MWHEELUP:
	case K_JOY1_AXISY_MINUS:
	case K_JOY2_AXISY_MINUS:
#ifndef _arch_dreamcast
	case K_JOY1_POV_UP:
	case K_JOY2_POV_UP:
#else
//	case K_JOY1_AXISZ_MINUS:
	case K_JOY1_DPAD1_UP:
	case K_JOY1_DPAD2_UP:
//	case K_JOY2_AXISZ_MINUS:
	case K_JOY2_DPAD1_UP:
	case K_JOY2_DPAD2_UP:
#endif
		m_inp_up = true;
		break;

	case K_DOWNARROW:
	case K_MWHEELDOWN:
	case K_JOY1_AXISY_PLUS:
	case K_JOY2_AXISY_PLUS:
#ifndef _arch_dreamcast
	case K_JOY1_POV_DOWN:
	case K_JOY2_POV_DOWN:
#else
//	case K_JOY1_AXISZ_PLUS:
	case K_JOY1_DPAD1_DOWN:
	case K_JOY1_DPAD2_DOWN:
//	case K_JOY2_AXISZ_PLUS:
	case K_JOY2_DPAD1_DOWN:
	case K_JOY2_DPAD2_DOWN:
#endif
		m_inp_down = true;
		break;

	case K_LEFTARROW:
	case K_MOUSE3:
	case K_JOY1_AXISX_MINUS:
	case K_JOY2_AXISX_MINUS:
#ifndef _arch_dreamcast
	case K_JOY1_POV_LEFT:
	case K_JOY2_POV_LEFT:
#else
//	case K_JOY1_AXISR_MINUS:
	case K_JOY1_DPAD1_LEFT:
	case K_JOY1_DPAD2_LEFT:
//	case K_JOY2_AXISR_MINUS:
	case K_JOY2_DPAD1_LEFT:
	case K_JOY2_DPAD2_LEFT:
#endif
		m_inp_left = true;
		break;

	case K_RIGHTARROW:
	case K_MOUSE2:
	case K_JOY1_AXISX_PLUS:
	case K_JOY2_AXISX_PLUS:
#ifndef _arch_dreamcast
	case K_JOY1_POV_RIGHT:
	case K_JOY2_POV_RIGHT:
#else
//	case K_JOY1_AXISR_PLUS:
	case K_JOY1_DPAD1_RIGHT:
	case K_JOY1_DPAD2_RIGHT:
//	case K_JOY2_AXISR_PLUS:
	case K_JOY2_DPAD1_RIGHT:
	case K_JOY2_DPAD2_RIGHT:
#endif
		m_inp_right = true;
		break;

	case K_MOUSE1: // Mouse 1 enables both m_inp_left and m_inp_ok
		m_inp_left = true;
	case K_ENTER:
	case K_JOY1_USE:
	case K_JOY2_USE:
		m_inp_ok = true;
		break;

	case K_ESCAPE:
	case K_JOY1_BACK:
	case K_JOY2_BACK:
		m_inp_cancel = true;
	case 'n':
	case 'N':
//	case K_JOY1_NO:
//	case K_JOY2_NO:
		m_inp_no = true;
		break;

	case 'Y':
	case 'y':
	case K_JOY1_YES:
	case K_JOY2_YES:
		m_inp_yes = true;
		break;

	case K_BACKSPACE:
	case K_JOY1_BACKSPACE:
	case K_JOY2_BACKSPACE:
		m_inp_backspace = true;
		break;

#ifdef _arch_dreamcast
	case K_JOY1_ESCAPE:
	case K_JOY2_ESCAPE:
		m_inp_off = true;
		break;
#endif

#if 0
	case K_DEL:				// delete multiple bindings
	case K_SPACE:

	case K_MOUSE3:
#endif

	default:
		break;
	}
	if (key > K_PAUSE && keybindings[key])
		if (!Q_strcmp(keybindings[key], "togglemenu"))
			m_inp_off = true;
};
// mankrip - end

qboolean	m_entersound;		// play after drawing a frame, so caching
								// won't disrupt the sound
qboolean	m_recursiveDraw;

int			m_save_demonum = -1; // mankrip - fix demos on startup

int			m_return_state;
qboolean	m_return_onerror;
char		m_return_reason [32];

#if NET_MENUS // mankrip - removed multiplayer menus
#define StartingGame	(m_multiplayer_cursor == 1)
#define JoiningGame		(m_multiplayer_cursor == 0)
#define SerialConfig	(m_net_cursor == 0)
#define DirectConfig	(m_net_cursor == 1)
#define	IPXConfig		(m_net_cursor == 2)
#define	TCPIPConfig		(m_net_cursor == 3)

void M_ConfigureNetSubsystem(void);
#endif

// BlackAura (28-12-2002) - Alternate menu background fading
void Draw_FadeScreen2 (int level);
int fade_level = 0;


// mankrip - precaching menu pics - begin
void M_Precache (void)
{
	int i;
// text box
	Draw_CachePic ("gfx/box_tl.lmp"); Draw_CachePic ("gfx/box_tm.lmp"); Draw_CachePic ("gfx/box_tr.lmp");
	Draw_CachePic ("gfx/box_ml.lmp"); Draw_CachePic ("gfx/box_mm.lmp"); Draw_CachePic ("gfx/box_mr.lmp");
	Draw_CachePic ("gfx/box_bl.lmp"); Draw_CachePic ("gfx/box_bm.lmp"); Draw_CachePic ("gfx/box_br.lmp");
	Draw_CachePic ("gfx/box_mm2.lmp");
// big "q" cursor
	for (i=1; i<7; i++)
		Draw_CachePic(va("gfx/menudot%i.lmp", i));
// left plaque
	Draw_CachePic ("gfx/qplaque.lmp");
// main menu
	Draw_CachePic ("gfx/ttl_main.lmp");
	Draw_CachePic ("gfx/mainmenu.lmp");
// single player
	Draw_CachePic ("gfx/ttl_sgl.lmp");
//	Draw_CachePic ("gfx/sp_menu.lmp");
// load/save
	Draw_CachePic ("gfx/p_load.lmp");
	Draw_CachePic ("gfx/p_save.lmp");
// options
	Draw_CachePic ("gfx/p_option.lmp");
	Draw_CachePic ("gfx/bigbox.lmp");
	Draw_CachePic ("gfx/menuplyr.lmp");
//	Draw_CachePic ("gfx/ttl_cstm.lmp");
//	Draw_CachePic ("gfx/vidmodes.lmp");
// help
//	for (i=0; i<6; i++)
//		Draw_CachePic(va("gfx/help%i.lmp", i));
// Multiplayer
	Draw_CachePic ("gfx/p_multi.lmp");
	Draw_CachePic ("gfx/mp_menu.lmp");
#if NET_MENUS // mankrip - removed multiplayer menus
	Draw_CachePic ("gfx/netmen1.lmp");
	Draw_CachePic ("gfx/dim_modm.lmp");
	Draw_CachePic ("gfx/netmen2.lmp");
	Draw_CachePic ("gfx/dim_drct.lmp");
	Draw_CachePic ("gfx/netmen3.lmp");
	Draw_CachePic ("gfx/dim_ipx.lmp");
	Draw_CachePic ("gfx/netmen4.lmp");
	Draw_CachePic ("gfx/dim_tcp.lmp");
	Draw_CachePic ("gfx/netmen5.lmp");
#endif

	// sound precaches
	S_PrecacheSound ("misc/menu1.wav");
	S_PrecacheSound ("misc/menu2.wav");
	S_PrecacheSound ("misc/menu3.wav");

	Vibration_Stop (0);
	Vibration_Stop (1);
}
// mankrip - precaching menu pics - end

#define	SLIDER_RANGE	10
void M_DrawSlider (int x, int y, float range)
{
	int	i;

	if (range < 0)
		range = 0;
	if (range > 1)
		range = 1;
	M_DrawCharacter (x-8, y, 128);
	for (i=0 ; i<SLIDER_RANGE ; i++)
		M_DrawCharacter (x + i*8, y, 129);
	M_DrawCharacter (x+i*8, y, 130);
	M_DrawCharacter (x + (SLIDER_RANGE-1)*8 * range, y, 131);
}

void M_DrawCheckbox (int x, int y, int on)
{
#if 0
	if (on)
		M_DrawCharacter (x, y, 131);
	else
		M_DrawCharacter (x, y, 129);
#endif
	if (on)
		M_Print (x, y, "on");
	else
		M_Print (x, y, "off");
}

//=============================================================================
// mankrip - begin
//=============================================================================

char *skilltos(int i)
{
	if (i <= 0) return "Easy";
	if (i == 1) return "Normal";
	if (i == 2) return "Hard";
	if (i >= 3) return "Nightmare";
	return "";
}
void M_DrawPlaque (char *c, qboolean b)
{
	qpic_t	*p;
	if (b)
		M_DrawTransPic (16, 0, Draw_CachePic ("gfx/qplaque.lmp") );
	p = Draw_CachePic (c);
	M_DrawTransPic ( (320-p->width)/2, 0, p);
}
void M_DrawCursor (int x, int y, int itemindex)
{
	static int oldx, oldy, oldindex;
	static float cursortime;
	if ((oldx != x) || (oldy != y) || (oldindex != itemindex))
	{
		oldx = x;
		oldy = y;
		oldindex = itemindex;
		cursortime = realtime;
	}
	if ((cursortime + 0.5) >= realtime)
		M_DrawCharacter (x, y + itemindex*8, 13);
	else
		M_DrawCharacter (x, y + itemindex*8, 12+((int)(realtime*4)&1));
}
#define ALIGN_LEFT 0
#define ALIGN_RIGHT 8
#define ALIGN_CENTER 4
void M_PrintText (char *s, int x, int y, int w, int h, int alignment)
{
	int		l;
	int		line_w[64];
	int		xpos = x;
	char	*s_count = s;

	for (l=0; l<64; l++)
		line_w[l] = 0;

	l = 0;
	while (*s_count)
	{
		if (*s_count == 10)
			l++;
		else
			line_w[l]++;
		s_count++;
	}
	if (line_w[l]) // last line is empty
		l++;
	y += ((h - l)*4); // 0=top, 4=center, 8=bottom

	l = 0;
	while (*s)
	{
		if (*s == 10)
		{
			xpos = x;
			l++;
		}
		else
		{
			M_DrawCharacter (xpos + ((w - line_w[l])*alignment), y + (l*8), (*s)+128);
			xpos += 8;
		}
		s++;
	}
}
void ChangeCVar (char *cvar_s, float current, float interval, float min, float max, qboolean slider)
{
//	if (self.impulse == 21)
//		interval = interval * -1;
	current += interval;
	if (/*interval < 0 &&*/ current < min)
	{
		if (slider)
			current = min;
		else
			current = max;
	}
	else if (/*interval > 0 &&*/ current > max)
	{
		if (slider)
			current = max;
		else
			current = min;
	}
	Cvar_SetValue (cvar_s, current);
};
//=============================================================================
/* ON-SCREEN KEYBOARD */

#define	NUM_KEYCODES	52
int		keyboard_active;
int		keyboard_cursor;
int		keyboard_shift;

int	keycodes[] =
{
	'0','1','2','3','4','5','6','7','8','9','-','+','/','|', 92,'_',
	'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p',
	'q','r','s','t','u','v','w','x','y','z','"',';','.','?','~', K_SPACE,

	'<','>','(',')','[',']','{','}','@','#','$','%','^','&','*','=',
	'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P',
	'Q','R','S','T','U','V','W','X','Y','Z', 39,':',',','!','`', K_SPACE,

	K_BACKSPACE, K_SHIFT, K_TAB, K_ENTER
};

void M_OnScreenKeyboard_Reset (void)
{
	keyboard_active = keyboard_cursor = keyboard_shift = 0;
	m_inp_up = m_inp_down = m_inp_left = m_inp_right = m_inp_ok = m_inp_cancel = false;
}

void M_OnScreenKeyboard_Draw (int y)
{
	int i;
	if (!keyboard_active)
		return;

	M_DrawTextBox (16, y, 33*8, 5*8);
	for (i=0; i<47; i++)
		M_PrintWhite (36 + i*16 - (256*(i/16)), y+12+(8*(i/16)), Key_KeynumToString (keycodes[i+keyboard_shift]));
	i=96;
	M_Print (36			, y+36, Key_KeynumToString (keycodes[i++]));
	M_Print (36+11*8	, y+36, Key_KeynumToString (keycodes[i++]));
	M_Print (36+18*8	, y+36, Key_KeynumToString (keycodes[i++]));
	M_Print (36+23*8	, y+36, Key_KeynumToString (keycodes[i++]));

	if (keyboard_cursor < 48)
	{
		M_DrawCharacter (27+ keyboard_cursor*16 - (256*(keyboard_cursor/16)), y+12+(8*(keyboard_cursor/16)), 16);
		M_DrawCharacter (44+ keyboard_cursor*16 - (256*(keyboard_cursor/16)), y+12+(8*(keyboard_cursor/16)), 17);
	}
	else if (keyboard_cursor == 48)
		M_DrawCharacter (28			, y+36, 12+((int)(realtime*4)&1));
	else if (keyboard_cursor == 49)
		M_DrawCharacter (28+11*8	, y+36, 12+((int)(realtime*4)&1));
	else if (keyboard_cursor == 50)
		M_DrawCharacter (28+18*8	, y+36, 12+((int)(realtime*4)&1));
	else if (keyboard_cursor == 51)
		M_DrawCharacter (28+23*8	, y+36, 12+((int)(realtime*4)&1));
}

int M_OnScreenKeyboard_Key (int key)
{
	if (!keyboard_active)
	{
		if (key == K_JOY1_USE || key == K_JOY2_USE)
		{
			M_OnScreenKeyboard_Reset();
			keyboard_active	= 1;
		}
		else
			return key;
	}
	else
	{
		m_inp_up = m_inp_down = m_inp_left = m_inp_right = m_inp_ok = m_inp_cancel = false;
		switch (key)
		{
		case K_JOY1_BACK:
		case K_JOY2_BACK:
			M_OnScreenKeyboard_Reset();
			break;

		case K_JOY1_AXISX_MINUS:
		case K_JOY2_AXISX_MINUS:
		#ifndef _arch_dreamcast
		case K_JOY1_POV_LEFT:
		case K_JOY2_POV_LEFT:
		#else
		case K_JOY1_DPAD1_LEFT:
		case K_JOY1_DPAD2_LEFT:
		case K_JOY2_DPAD1_LEFT:
		case K_JOY2_DPAD2_LEFT:
		#endif
			if (--keyboard_cursor < 0)
				keyboard_cursor = NUM_KEYCODES-1;
			break;

		case K_JOY1_AXISX_PLUS:
		case K_JOY2_AXISX_PLUS:
		#ifndef _arch_dreamcast
		case K_JOY1_POV_RIGHT:
		case K_JOY2_POV_RIGHT:
		#else
		case K_JOY1_DPAD1_RIGHT:
		case K_JOY1_DPAD2_RIGHT:
		case K_JOY2_DPAD1_RIGHT:
		case K_JOY2_DPAD2_RIGHT:
		#endif
			if (++keyboard_cursor >= NUM_KEYCODES)
				keyboard_cursor = 0;
			break;

		case K_JOY1_AXISY_MINUS:
		case K_JOY2_AXISY_MINUS:
		#ifndef _arch_dreamcast
		case K_JOY1_POV_UP:
		case K_JOY2_POV_UP:
		#else
		case K_JOY1_DPAD1_UP:
		case K_JOY1_DPAD2_UP:
		case K_JOY2_DPAD1_UP:
		case K_JOY2_DPAD2_UP:
		#endif
			if (keyboard_cursor >= 48)
				keyboard_cursor = 48;
			keyboard_cursor-=16;
			if (keyboard_cursor < 0)
				keyboard_cursor = NUM_KEYCODES-4;
			break;

		case K_JOY1_AXISY_PLUS:
		case K_JOY2_AXISY_PLUS:
		#ifndef _arch_dreamcast
		case K_JOY1_POV_DOWN:
		case K_JOY2_POV_DOWN:
		#else
		case K_JOY1_DPAD1_DOWN:
		case K_JOY1_DPAD2_DOWN:
		case K_JOY2_DPAD1_DOWN:
		case K_JOY2_DPAD2_DOWN:
		#endif
			if (keyboard_cursor < 32)
				keyboard_cursor+=16;
			else if (keyboard_cursor < 48)
				keyboard_cursor = NUM_KEYCODES-4;
			else
				keyboard_cursor = 0;
			break;

		case K_JOY1_PGDN:
		case K_JOY2_PGDN:
			return K_BACKSPACE;
		case K_JOY1_PGUP:
		case K_JOY2_PGUP:
			return K_SPACE;
		case K_JOY1_USE:
		case K_JOY2_USE:
			if (keyboard_cursor < 48)
				return keycodes[keyboard_cursor+keyboard_shift];
			else if (keyboard_cursor == 49)
				keyboard_shift = (!keyboard_shift)*48;
			else
				return keycodes[keyboard_cursor+48];
		#ifndef _arch_dreamcast
		case K_JOY1_4:
		case K_JOY2_4:
        #else
		case K_JOY1_X:
		case K_JOY2_X:
        #endif
			return K_TAB;
		case K_JOY1_ENTER:
		case K_JOY2_ENTER:
			return K_ENTER;
		default:
			return key;
		}
	}
	return 0;
}

//=============================================================================
/* POP-UP MENU */
int			m_prevstate;
qboolean	wasInMenus;
char		*popup_message;
char		*popup_command;

void M_PopUp_f (char *s, char *cmd)
{
	if (m_state != m_popup)
	{
		wasInMenus = (key_dest == key_menu);
		key_dest = key_menu;
		m_prevstate = m_state; // set previously active menu
		m_state = m_popup;
	}
	m_entersound = true;
	popup_message = s;
	if (Q_strlen(cmd))
	{
		popup_command = Z_Malloc (Q_strlen(cmd)+1);
		Q_strcpy (popup_command, cmd);
	}
	else
		popup_command = NULL;
}

void M_PopUp_Draw (void)
{
	if (wasInMenus)
	{
		m_state = m_prevstate;
		m_recursiveDraw = true;
		M_Draw ();
		m_state = m_popup;
	}
	M_DrawTextBox (56, (200/*vid.height*/ - 48) / 2, 24*8, 4*8);
	M_PrintText (popup_message, 64, (200/*vid.height*/ - 32) / 2, 24, 4, ALIGN_CENTER);
}

void M_PopUp_Key (int key)
{
	if (m_inp_yes && popup_command)
		Cbuf_AddText(popup_command);
	else if (!m_inp_no)
		return;

	if (wasInMenus)
	{
		m_state = m_prevstate;
		m_entersound = true;
	}
	else
	{
		fade_level = 0; // BlackAura - Menu background fading
		key_dest = key_game;
		m_state = m_none;
	}
	if (popup_command)
		Z_Free(popup_command);
}

void M_Off (void) // turns the menu off
{
	key_dest = key_game;
	if (m_state == m_credits) // no previous demo number had been set before the credits menu appeared
		cls.demonum = 0;
	else
	{
		if (m_save_demonum != -1) // a previous demo number has been set
			cls.demonum = m_save_demonum;
		if (!cls.demoplayback && cls.state != ca_connected)
			CL_NextDemo ();
	}
	m_state = m_none;
}
//=============================================================================
// mankrip - end
//=============================================================================

/*
================
M_ToggleMenu_f
================
*/
void M_ToggleMenu_f (void)
{
	m_entersound = true;

	if (key_dest == key_menu)
	{
		if (m_state != m_main)
		{
			// mankrip - begin
			if (!strcmp (keybindings[K_ESCAPE], "menu_fightoon"))
				M_Fightoon_f ();
			else
			// mankrip - end
			M_Main_f ();
			return;
		}

		key_dest = key_game;
		m_state = m_none;
		return;
	}
	if (key_dest == key_console)
	{
		Con_ToggleConsole_f ();
	}
	else
	{
		// mankrip - begin
		if (!strcmp (keybindings[K_ESCAPE], "menu_fightoon"))
			M_Fightoon_f ();
		else
		// mankrip - end
		M_Main_f ();
	}
}

//=============================================================================
/* MAIN MENU */

#define	MAIN_ITEMS	5


void M_Main_f (void)
{
	if (key_dest != key_menu)
	{
		m_save_demonum = cls.demonum;
		cls.demonum = -1;
	}
	key_dest = key_menu;
	m_state = m_main;
	m_entersound = true;
	M_Precache(); // mankrip - precaching menu pics
}


void M_Main_Draw (void)
{
	M_DrawPlaque ("gfx/ttl_main.lmp", true); // mankrip

	M_DrawTransPic (72, 28, Draw_CachePic ("gfx/mainmenu.lmp") );

	M_DrawTransPic (54, 28 + m_cursor[m_state] * 20, Draw_CachePic( va("gfx/menudot%i.lmp", 1+((int)(realtime * 10)%6) ) ) ); // edited by mankrip
}


void M_Main_Key (int key)
{
	// The DC_B shouldn't turn the main menu off
	if (key==K_ESCAPE)
		M_Off ();
	else if (m_inp_down)
	{
		S_LocalSound ("misc/menu1.wav");
		if (++m_cursor[m_state] >= MAIN_ITEMS)
			m_cursor[m_state] = 0;
	}
	else if (m_inp_up)
	{
		S_LocalSound ("misc/menu1.wav");
		if (--m_cursor[m_state] < 0)
			m_cursor[m_state] = MAIN_ITEMS - 1;
	}
	else if (m_inp_ok)
	{
		switch (m_cursor[m_state]) // mankrip - replaced cursor variables
		{
		case 0:
			M_SinglePlayer_f ();
			break;

		case 1:
#if NET_MENUS // mankrip - removed multiplayer menus
			M_MultiPlayer_f ();
#else
			M_GameOptions_f (); // mankrip
#endif
			break;

		case 2:
			M_Options_f ();
			break;

		case 3:
			M_Help_f ();
			break;

		case 4:
			M_Quit_f ();
			break;
		}
	}
}

// mankrip - begin
//=============================================================================
/* FIGHTOON MENU */

#define	FIGHTOON_ITEMS	9

void M_Fightoon_f (void)
{
	key_dest = key_menu;
	m_state = m_fightoon;
	m_entersound = true;
	M_Precache(); // mankrip - precaching menu pics
}


void M_Fightoon_Draw (void)
{
	int y = 20;

	qpic_t	*pic;
	pic = Draw_CachePic ("gfx/pause.lmp");
	M_DrawTransPic (160 - pic->width / 2, 200 - pic->height, pic);
//	M_DrawPlaque ("gfx/ttl_sgl.lmp", true);

	y += 80;

	M_Print (16, y += 8, "               Restart");
	M_Print (16, y += 8, "   Character Selection");
	M_Print (16, y += 8, "  Reset Player 1 Score");
    if (!sbar.value)
    {
        extern cvar_t sbar_lose1;
        extern cvar_t sbar_win1;
        M_Print (27*8, y, va("(L:%i,", (int)(sbar_lose1.value)));
		M_Print (27*8+6*8, y, va("W:%i)", (int)(sbar_win1.value)));
    }
	M_Print (16, y += 8, "  Reset Player 2 Score");
    if (!sbar.value)
    {
        extern cvar_t sbar_lose2;
        extern cvar_t sbar_win2;
        M_Print (27*8, y, va("(L:%i,", (int)(sbar_lose2.value)));
		M_Print (27*8+6*8, y, va("W:%i)", (int)(sbar_win2.value)));
    }
	M_Print (16, y += 8, "    Customize Controls    ...");
	M_Print (16, y += 8, "                 Audio    ...");
	M_Print (16, y += 8, "         Adjust Screen    ...");
	M_Print (16, y += 8, "            Save setup");
	M_Print (16, y += 8, "            Load setup");

	M_DrawCursor (200, 28+80, m_cursor[m_state]);
}


void M_Fightoon_Key (int key)
{
	// The DC_B shouldn't turn the main menu off
	if (key==K_ESCAPE)
		M_Off ();
	else if (m_inp_up)
	{
		S_LocalSound ("misc/menu1.wav");
		if (--m_cursor[m_state] < 0)
			m_cursor[m_state] = FIGHTOON_ITEMS - 1;
	}
	else if (m_inp_down)
	{
		S_LocalSound ("misc/menu1.wav");
		if (++m_cursor[m_state] >= FIGHTOON_ITEMS)
			m_cursor[m_state] = 0;
	}
	else if (m_inp_ok)
	{
		m_entersound = true;
		switch (m_cursor[m_state])
		{
		case 0:
			M_Off ();
			Cbuf_AddText("restart\n");
			break;
		case 1:
			Cbuf_AddText("disconnect\nmaxplayers 1\nmap start\n");
			break;
		case 2:
			Cbuf_AddText("sbar_lose1 0; sbar_win1 0\n");
			break;
		case 3:
			Cbuf_AddText("sbar_lose2 0; sbar_win2 0\n");
			break;
		case 4:
			M_Keys_f ();
			break;
		case 5:
			M_Audio_f ();
			break;
		case 6:
			M_VideoSize_f ();
			break;
		case 7:
			#ifdef _arch_dreamcast
			M_SelectVMU_f (m_saveoptions);
			#else
			Host_WriteConfiguration ();
			#endif
			break;
		case 8:
			#ifdef _arch_dreamcast
			M_SelectVMU_f (m_loadoptions);
			#else
			M_PopUp_f("Do you wish to load the\nsaved settings?", "exec config.cfg\n");
			#endif
			break;

		default:
			break;
		}
	}
}
// mankrip - end

//=============================================================================
/* SINGLE PLAYER MENU */

#define	SINGLEPLAYER_ITEMS	5 // mankrip - edited

void M_SinglePlayer_f (void)
{
	key_dest = key_menu;
	m_state = m_singleplayer;
	m_entersound = true;
}


void M_SinglePlayer_Draw (void)
{
	// mankrip - begin
	int y = 20;

	M_DrawPlaque ("gfx/ttl_sgl.lmp", true);

	M_Print (16, y += 8, "              New game");
	M_Print (16, y += 8, "       Load checkpoint    ...");
	M_Print (16, y += 8, "       Save checkpoint    ...");
	M_Print (16, y += 8, "            Load state    ...");
	M_Print (16, y += 8, "            Save state    ...");

	if (m_cursor[m_state])
	{
		M_DrawTextBox (4*8, 200 - 48 + 16, 29*8, 2*8);
		switch (m_cursor[m_state]) // mankrip - replaced cursor variables
		{
			case 1:
				M_PrintText ("Load progress across levels,\nat the beginning of the level.", 12, 200 - 48 + 24, 37, 2, ALIGN_CENTER);
				break;
			case 2:
				M_PrintText ("Save progress across levels,\nat the beginning of the level.", 12, 200 - 48 + 24, 37, 2, ALIGN_CENTER);
				break;
			case 3:
				M_PrintText ("Load progress within a level,\nanywhere.", 12, 200 - 48 + 24, 37, 2, ALIGN_CENTER);
				break;
			case 4:
				M_PrintText ("Save progress within a level,\nanywhere.", 12, 200 - 48 + 24, 37, 2, ALIGN_CENTER);
				break;
			default: break;
		}
	}

	M_DrawCursor (200, 28, m_cursor[m_state]);
	// mankrip - end
}


void M_SinglePlayer_Key (int key)
{
	if (m_inp_cancel)
		M_Main_f ();
	else if (m_inp_up)
	{
		S_LocalSound ("misc/menu1.wav");
		if (--m_cursor[m_state] < 0)
			m_cursor[m_state] = SINGLEPLAYER_ITEMS - 1;
	}
	else if (m_inp_down)
	{
		S_LocalSound ("misc/menu1.wav");
		if (++m_cursor[m_state] >= SINGLEPLAYER_ITEMS)
			m_cursor[m_state] = 0;
	}
	else if (m_inp_ok)
	{
		m_entersound = true;
		switch (m_cursor[m_state]) // mankrip - replaced cursor variables
		{
		case 0:
			if (sv.active)
				M_PopUp_f("Are you sure you want to\nstart a new game?", "disconnect\nmaxplayers 1\nmap start\n"); // mankrip
			else
				Cbuf_AddText("disconnect\nmaxplayers 1\nmap start\n"); // mankrip
			break;
		// mankrip - small saves - begin
		case 1:
			// mankrip - VMU saves - begin
			#ifdef _arch_dreamcast
			M_VMUloadsmall_f ();
			#else
			// mankrip - VMU saves - end
			M_LoadSmall_f ();
			#endif // mankrip - VMU saves
			break;

		case 2:
			// mankrip - VMU saves - begin
			#ifdef _arch_dreamcast
			M_VMUsavesmall_f ();
			#else
			// mankrip - VMU saves - end
			M_SaveSmall_f ();
			#endif // mankrip - VMU saves
			break;
		// mankrip - small saves - end
		case 3:
			// mankrip - VMU saves - begin
			#ifdef _arch_dreamcast
			M_VMUload_f ();
			#else
			// mankrip - VMU saves - end
			M_Load_f ();
			#endif // mankrip - VMU saves
			break;

		case 4:
			// mankrip - VMU saves - begin
			#ifdef _arch_dreamcast
			M_VMUsave_f ();
			#else
			// mankrip - VMU saves - end
			M_Save_f ();
			#endif // mankrip - VMU saves
			break;

		default:
			break;
		}
	}
}

//=============================================================================
/* LOAD/SAVE MENU */

// mankrip - VMU saves - begin
#ifdef _arch_dreamcast
extern int VMU_CountFreeBlocks(int controller, int slot);
int	vmufreeblocks[4][2], vmu_cursor[2];
int	previousmenu, nextmenu;
void M_SelectVMU_f (int menu)
{
	int		i, i2;

	if (key_dest == key_menu && m_state == m_vmu)
	{
		M_Off ();
		return;
	}
	if (menu == m_save || menu == m_savesmall)
	{
		if (!sv.active)
			return;
		if (svs.maxclients != 1)
			return;
	}
	for (i =0; i <4; i ++)
		for (i2=1; i2<3; i2++)
			vmufreeblocks[i][i2-1] = VMU_CountFreeBlocks(i, i2);
	vmu_cursor[0] = vmupath.string[0]-'a';
	vmu_cursor[1] = vmupath.string[1]-'1';
	m_entersound = true;
	key_dest = key_menu;
	previousmenu = m_state;
	nextmenu = menu;
	m_state = m_vmu;
	M_Precache(); // mankrip - precaching menu pics
}
void M_VMUsave_f (void)
{
	if (cl.stats[STAT_HEALTH] <= 0) // the player is dead
		return;
	if (cl.intermission)
		return;
/*	if (m_state == m_save)
	{
		m_state = m_none;
		M_Save_f ();
	}
	else
*/		M_SelectVMU_f (m_save);
}
void M_VMUsavesmall_f (void)
{
/*	if (m_state == m_savesmall)
	{
		m_state = m_none;
		M_SaveSmall_f ();
	}
	else
*/		M_SelectVMU_f (m_savesmall);
}
void M_VMUload_f (void)
{
	M_SelectVMU_f (m_load);
}
void M_VMUloadsmall_f (void)
{
	M_SelectVMU_f (m_loadsmall);
}
void M_SelectVMU_Draw (void)
{
	int i, i2, y = 20;

	M_DrawPlaque ((nextmenu==m_save || nextmenu==m_savesmall || nextmenu==m_saveoptions) ? "gfx/p_save.lmp" : "gfx/p_load.lmp", true);

	M_PrintWhite (64+48, y+=8, "Select a VMU");
	M_Print (64+36, y+=16, "A      B      C      D");

	for (i2=1; i2<3; i2++)
	{
		M_Print (56, y + 16*i2, va("%i", i2));
		for (i=0;i<4;i++)
		{
			M_DrawTextBox (64+16 + 56*i, y-8 + 16*i2, 3*8, 8);
			if (vmufreeblocks[i][i2-1] >= 0)
				M_PrintWhite (64+28 + 56*i, y + 16*i2, va("%3i", vmufreeblocks[i][i2-1]));
		}
	}
	M_DrawCursor (64+8 + vmu_cursor[0]*56, 60 + vmu_cursor[1]*16, 0);
}

void M_SelectVMU_Key (int k)
{
	if (m_inp_cancel)
	{
		if (!previousmenu)
			M_Off ();
		else if (previousmenu == m_singleplayer)
			M_SinglePlayer_f ();
		else if (previousmenu == m_options)
			M_Options_f ();
	}
	else if (m_inp_ok)
	{
		if (vmufreeblocks[vmu_cursor[0]][vmu_cursor[1]] >= 0)
		{
			vmupath.string[0] = vmu_cursor[0]+'a';
			vmupath.string[1] = vmu_cursor[1]+'1';
			if (nextmenu == m_save)
				M_Save_f();
			if (nextmenu == m_savesmall)
				M_SaveSmall_f();
			if (nextmenu == m_load)
				M_Load_f();
			if (nextmenu == m_loadsmall)
				M_LoadSmall_f();
			if (nextmenu == m_saveoptions)
			{
				M_Options_f();
				Host_WriteConfiguration();
			}
			if (nextmenu == m_loadoptions)
			{
				M_Options_f();
				M_PopUp_f("Do you wish to load the\nsaved settings?", "exec config.cfg\n");
			}
		}
	}
	else
	{
		S_LocalSound ("misc/menu1.wav");
		if (m_inp_up || m_inp_down)
			vmu_cursor[1] = !vmu_cursor[1];
		else if (m_inp_right)
		{
			if (++vmu_cursor[0] > 3)
				vmu_cursor[0] = 0;
		}
		else if (m_inp_left)
		{
			if (--vmu_cursor[0] < 0)
				vmu_cursor[0] = 3;
		}
	}
}
#endif
// mankrip - VMU saves - end

// -------------------------------------------------
// BlackAura (08-12-2002) - Replacing fscanf (start)
// -------------------------------------------------
static void DCM_ScanString(FILE *file, char *string)
{
	char newchar;
	fread(&newchar, 1, 1, file);
	while(newchar != '\n')
	{
		*string++ = newchar;
		fread(&newchar, 1, 1, file);
	}
	*string++ = '\0';
}

static int DCM_ScanInt(FILE *file)
{
	char sbuf[32768];
	DCM_ScanString(file, sbuf);
	return Q_atoi(sbuf);
}
// -------------------------------------------------
//  BlackAura (08-12-2002) - Replacing fscanf (end)
// -------------------------------------------------

int		liststart[4]; // mankrip [m_state - m_load]
int		load_cursormax; // mankrip
#define	MAX_SAVEGAMES		100 // mankrip - edited
char	m_filenames[MAX_SAVEGAMES][SAVEGAME_COMMENT_LENGTH+1];
char	*m_fileinfo[MAX_SAVEGAMES]; // mankrip
int		loadable[MAX_SAVEGAMES];

void M_ScanSaves (qboolean smallsave) // mankrip - edited
{
	int		i, j;
	char	name[MAX_OSPATH];
	FILE	*f;
	int		version;
	// mankrip - begin
	int		i1, i2;
	char	*s;
	char	t_enemies[16], k_enemies[16], t_secrets[16], f_secrets[16], str[64], fileinfo[64];
	// mankrip - end
#ifdef _arch_dreamcast
	extern byte dc_foundsave[MAX_SAVEGAMES];
	extern void VMU_List (char c);
	VMU_List(smallsave?'G':'S');
#endif

	for (i=0 ; i<MAX_SAVEGAMES ; i++)
	{
		strcpy (m_filenames[i], "- empty -"); // mankrip - edited
		loadable[i] = false;

		// mankrip - begin
		if (m_fileinfo[i])
			Z_Free(m_fileinfo[i]);
		m_fileinfo[i] = NULL;

		//"%s/s%i.sav" - renamed savegames' names
#ifdef _arch_dreamcast
		if (!dc_foundsave[i])
			continue;
		sprintf (name, "/ram/%s.%c%i%i", savename.string, smallsave?'G':'S', i/10, i%10);
		VMU_Load(name, COM_SkipPath(name));
#else
		sprintf (name, "%s/%s.%c%i%i", com_gamedir, savename.string, smallsave?'G':'S', i/10, i%10);
#endif
		// mankrip - end

		f = fopen (name, "r");
		if (!f)
			continue;

		// mankrip - begin
		// set everything to "0"
		t_enemies[0] = k_enemies[0] = t_secrets[0] = f_secrets[0] = '0';
		t_enemies[1] = k_enemies[1] = t_secrets[1] = f_secrets[1] = 0;
		// mankrip - end

		//  BlackAura (08-12-2002) - Replacing fscanf
		// fscanf (f, "%i\n", &version);
		// fscanf (f, "%79s\n", name);
		version = DCM_ScanInt(f);
		DCM_ScanString(f, name);
		strncpy (m_filenames[i], name, sizeof(m_filenames[i])-1 -17); // mankrip - edited

	// change _ back to space
		for (j=0 ; j<SAVEGAME_COMMENT_LENGTH ; j++)
			if (m_filenames[i][j] == '_')
				m_filenames[i][j] = ' ';

		// mankrip - begin
		// skill
		for (i1=0 ; i1 < 17; i1++)
			i2 = DCM_ScanInt(f);
		s = skilltos(i2);

		DCM_ScanString(f, fileinfo); // map
		strcat (fileinfo, va("\n%s\n", s)); // add skill
		if (!smallsave)
		{
			strcat (fileinfo, timetos(DCM_ScanInt(f))); // add time
			for (i2=0 ; i2 < 200 ; i2++)
			{
				DCM_ScanString(f, str);
				s = COM_Parse (str);
				if (com_token[0] == '}')
					break;
				if (!Q_strcmp(com_token, "total_secrets"))
				{
					s = COM_Parse (s);
					Q_strcpy(t_secrets, com_token);
					continue;
				}
				if (!Q_strcmp(com_token, "total_monsters"))
				{
					s = COM_Parse (s);
					Q_strcpy(t_enemies, com_token);
					continue;
				}
				if (!Q_strcmp(com_token, "found_secrets"))
				{
					s = COM_Parse (s);
					Q_strcpy(f_secrets, com_token);
					continue;
				}
				if (!Q_strcmp(com_token, "killed_monsters"))
				{
					s = COM_Parse (s);
					Q_strcpy(k_enemies, com_token);
					break;
				}
			}
			Q_strcat(fileinfo, va("\n%s / %s", k_enemies, t_enemies));
			Q_strcat(fileinfo, va("\n%s / %s", f_secrets, t_secrets));
		}
		m_fileinfo[i] = Z_Malloc(Q_strlen(fileinfo)+1);
		Q_strcpy(m_fileinfo[i], fileinfo);
		// mankrip - end

		loadable[i] = true;
		fclose (f);
#ifdef _arch_dreamcast
		fs_unlink(name); // delete save from RAM
#endif
	}
}

// mankrip - begin
void M_RefreshSaveList (void)
{
	int		i, i2;
	qboolean saving = (m_state == m_save || m_state == m_savesmall);
	qboolean smallsave = (m_state == m_loadsmall || m_state == m_savesmall);
	if (key_dest == key_menu && (smallsave || m_state == m_save || m_state == m_load))
	{
		M_ScanSaves (smallsave);
		for (i=i2=0; i< MAX_SAVEGAMES; i++)
			if (loadable[i] || (saving && (loadable[i-1] || i == 0)))
				i2++;
		if (m_cursor[m_state] >= i2)
			m_cursor[m_state] = i2 - 1;
		if (m_cursor[m_state] < 0)
			m_cursor[m_state] = 0;
		m_entersound = true;
	}
}
// info panel
void M_DrawFileInfo (int i, int y)
{
	if (m_state == m_loadsmall || m_state == m_savesmall)
	{
		M_DrawTextBox (72, y, 20*8, 2*8);
		M_PrintText ("map\nskill", 88, y+8, 20, 2, ALIGN_LEFT);
		M_PrintText (m_fileinfo[i], 160, y+8, 20, 2, ALIGN_LEFT);
	}
	else
	{
		M_DrawTextBox (72, y, 20*8, 5*8);
		M_PrintText ("map\nskill\ntime\nenemies\nsecrets", 88, y+8, 20, 5, ALIGN_LEFT);
		M_PrintText (m_fileinfo[i], 160, y+8, 20, 5, ALIGN_LEFT);
	}
}

// merged save/load menu functions

void M_Save_Common_f (int menustate)
{
	int		i, i2;
	qboolean saving = (menustate == m_save || menustate == m_savesmall);

	if (key_dest == key_menu && m_state == menustate)
	{
		M_Off ();
		return;
	}
	if (saving)
	{
		if (!sv.active)
			return;
		if (svs.maxclients != 1)
			return;
	}
	M_ScanSaves (menustate == m_savesmall || menustate == m_loadsmall);
	for (i=i2=0; i< MAX_SAVEGAMES; i++)
		if (loadable[i] || (saving && (loadable[i-1] || i == 0)))
			i2++;
	if (!saving && i2 == 0)
	{
		M_PopUp_f((menustate==m_loadsmall) ? "There isn't any save\nto load." : "There isn't any state\nto load.", "");
		return;
	}
	m_entersound = true;
	key_dest = key_menu;
	m_state = menustate;
	if (m_cursor[m_state] >= i2)
		m_cursor[m_state] = i2 - 1;
	if (m_cursor[m_state] < 0)
		m_cursor[m_state] = 0;
	M_Precache(); // mankrip - precaching menu pics
}
void M_Save_f (void)
{
	if (cl.stats[STAT_HEALTH] <= 0) // the player is dead
		return;
	if (cl.intermission)
		return;
	M_Save_Common_f(m_save);
}
void M_SaveSmall_f (void)
{
	M_Save_Common_f(m_savesmall);
}
void M_Load_f (void)
{
	M_Save_Common_f(m_load);
}
void M_LoadSmall_f (void)
{
	M_Save_Common_f(m_loadsmall);
}

void M_Save_Draw (void)
{
	int		i, i2, numsaves;
	qboolean saving = (m_state == m_save || m_state == m_savesmall);
	qboolean smallsave = (m_state == m_loadsmall || m_state == m_savesmall);

	M_DrawPlaque (saving ? "gfx/p_save.lmp" : "gfx/p_load.lmp", true);

	numsaves = (int)((200 - 28 - 8*7) / 8);
	if (smallsave)
		numsaves += 3;

	for (i=i2=0 ; i< MAX_SAVEGAMES; i++)
		if (loadable[i] || (saving && (loadable[i-1] || i == 0)))
		{
			if ((i >= liststart[m_state - m_load]) && (i2 - liststart[m_state - m_load] >= 0) && (i2 - liststart[m_state - m_load] < numsaves))
			{
				M_Print (56 + (8*(i < 10)), 28 + 8*(i2 - liststart[m_state - m_load]), va("%i", i));
				if (loadable[i] == true)
					M_Print		 (96, 28 + 8*(i2 - liststart[m_state - m_load]), m_filenames[i]);
				else
					M_PrintWhite (96, 28 + 8*(i2 - liststart[m_state - m_load]), m_filenames[i]);
			}
			i2++;
		}
	load_cursormax = i2;

	if (load_cursormax >= numsaves)
	if (liststart[m_state - m_load] > load_cursormax - numsaves)
		liststart[m_state - m_load] = load_cursormax - numsaves;

	for (i=i2=0 ; i< MAX_SAVEGAMES; i++)
		if (loadable[i] || (saving && (loadable[i-1] || i == 0)))
		{
			if (m_cursor[m_state] == i2)
			{
				if (loadable[i])
					M_DrawFileInfo(i, 28 + (numsaves*8));
				break;
			}
			i2++;
		}
	M_DrawCursor (80, 28, (m_cursor[m_state] - liststart[m_state - m_load])); // mankrip
}
// mankrip - end

void M_Save_Key (int k)
{
	// mankrip - begin
	qboolean saving = (m_state == m_save || m_state == m_savesmall);
	int		i, i2, numsaves;
	numsaves = (int)((200 - 28 - 8*7) / 8) - 1;
	if (m_state == m_savesmall || m_state == m_loadsmall)
		numsaves += 3;
	// mankrip - end

	if (m_inp_cancel)
		M_SinglePlayer_f ();
	else if (m_inp_up)
	{
		S_LocalSound ("misc/menu1.wav");
		// mankrip - begin
		if (--m_cursor[m_state] < 0)
		{
			m_cursor[m_state] = load_cursormax-1;
			liststart[m_state - m_load] = m_cursor[m_state] - numsaves;
		}
		if (m_cursor[m_state] < liststart[m_state - m_load])
			liststart[m_state - m_load] = m_cursor[m_state];
		if (liststart[m_state - m_load] < 0)
			liststart[m_state - m_load] = 0;
		// mankrip - end
	}
	else if (m_inp_down)
	{
		S_LocalSound ("misc/menu1.wav");
		// mankrip - begin
		if (++m_cursor[m_state] >= load_cursormax)
			liststart[m_state - m_load] = m_cursor[m_state] = 0;
		if (m_cursor[m_state] > liststart[m_state - m_load] + numsaves)
			liststart[m_state - m_load] = m_cursor[m_state] - numsaves;
		// mankrip - end
	}
	else if (m_inp_ok)
	{
		// mankrip - begin
		for (i=i2=0 ; i< MAX_SAVEGAMES; i++)
			if (loadable[i] || (saving && (loadable[i-1] || i == 0)))
			{
				if (m_cursor[m_state] == i2)
				{
				if (saving)
				{
					// the "menu_main;menu_save" part is a hack to refresh the list of saves
					if (loadable[i] == false)
// mankrip - VMU saves - begin
#ifdef _arch_dreamcast
						Cbuf_AddText (va ("save%s %s.%c%i%i\n", (m_state==m_savesmall)?"small":"", savename.string, (m_state==m_savesmall)?'G':'S', i/10, i%10));
					else
						M_PopUp_f ((m_state==m_savesmall)?"Overwrite saved game?":"Overwrite saved state?", va ("save%s %s.%c%i%i\n", (m_state==m_savesmall)?"small":"", savename.string, (m_state==m_savesmall)?'G':'S', i/10, i%10));
#else
// mankrip - VMU saves - end
						Cbuf_AddText (va ("save%s %s.%c%i%i\nmenu_main;menu_save%s\n", (m_state==m_savesmall)?"small":"", savename.string, (m_state==m_savesmall)?'G':'S', i/10, i%10, (m_state==m_savesmall)?"small":""));
					else
						M_PopUp_f ((m_state==m_savesmall)?"Overwrite saved game?":"Overwrite saved state?", va ("save%s %s.%c%i%i\nmenu_main;menu_save%s\n", (m_state==m_savesmall)?"small":"", savename.string, (m_state==m_savesmall)?'G':'S', i/10, i%10, (m_state==m_savesmall)?"small":""));
#endif // mankrip - VMU saves
					return;
				}
				else
				{
		// mankrip - end
					// Host_Loadgame_f can't bring up the loading plaque because too much
					// stack space has been used, so do it now
					SCR_BeginLoadingPlaque ();
					Cbuf_AddText (va ("load%s %s.%c%i%i\n", (m_state==m_loadsmall)?"small":"", savename.string, (m_state==m_loadsmall)?'G':'S', i/10, i%10) );
					fade_level = 0; // BlackAura - Menu background fading
					m_state = m_none;
					key_dest = key_game;
					return;
		// mankrip - begin
				}
				}
				i2++;
			}
	}
	else if (k == K_DEL || k == K_BACKSPACE || k == K_JOY1_BACKSPACE || k == K_JOY2_BACKSPACE)
	{
		for (i=i2=0 ; i< MAX_SAVEGAMES; i++)
			if (loadable[i] || (saving && (loadable[i-1] || i == 0)))
			{
				if (loadable[i] && m_cursor[m_state] == i2)
				{
					M_PopUp_f ("Are you sure you want to\ndelete this save?", va ("del %s.%c%i%i\n",
						savename.string, (m_state==m_loadsmall||m_state==m_savesmall)?'G':'S', i/10, i%10));
					return;
				}
				i2++;
			}
	}
	// mankrip - end
}

//=============================================================================
/* GAME OPTIONS MENU */

typedef struct
{
	char	*description;
	int		firstLevel;
	int		levels;
} episode_t;

typedef struct
{
	char	*name;
	char	*description;
} level_t;

level_t		id1levels[] = // mankrip - renamed
{
	{"start", "Entrance"},				// 0

	{"e1m1", "Slipgate Complex"},		// 1
	{"e1m2", "Castle of the Damned"},
	{"e1m3", "The Necropolis"},
	{"e1m4", "The Grisly Grotto"},
	{"e1m5", "Gloom Keep"},
	{"e1m6", "The Door To Chthon"},
	{"e1m7", "The House of Chthon"},
	{"e1m8", "Ziggurat Vertigo"},

	{"e2m1", "The Installation"},		// 9
	{"e2m2", "Ogre Citadel"},
	{"e2m3", "Crypt of Decay"},
	{"e2m4", "The Ebon Fortress"},
	{"e2m5", "The Wizard's Manse"},
	{"e2m6", "The Dismal Oubliette"},
	{"e2m7", "Underearth"},

	{"e3m1", "Termination Central"},	// 16
	{"e3m2", "The Vaults of Zin"},
	{"e3m3", "The Tomb of Terror"},
	{"e3m4", "Satan's Dark Delight"},
	{"e3m5", "Wind Tunnels"},
	{"e3m6", "Chambers of Torment"},
	{"e3m7", "The Haunted Halls"},

	{"e4m1", "The Sewage System"},		// 23
	{"e4m2", "The Tower of Despair"},
	{"e4m3", "The Elder God Shrine"},
	{"e4m4", "The Palace of Hate"},
	{"e4m5", "Hell's Atrium"},
	{"e4m6", "The Pain Maze"},
	{"e4m7", "Azure Agony"},
	{"e4m8", "The Nameless City"},

	{"end", "Shub-Niggurath's Pit"},	// 31

	{"dm1", "Place of Two Deaths"},		// 32
	{"dm2", "Claustrophobopolis"},
	{"dm3", "The Abandoned Base"},
	{"dm4", "The Bad Place"},
	{"dm5", "The Cistern"},
	{"dm6", "The Dark Zone"}
};

//MED 01/06/97 added hipnotic levels
level_t     hipnoticlevels[] =
{
   {"start", "Command HQ"},				// 0

   {"hip1m1", "The Pumping Station"},	// 1
   {"hip1m2", "Storage Facility"},
   {"hip1m3", "The Lost Mine"},
   {"hip1m4", "Research Facility"},
   {"hip1m5", "Military Complex"},

   {"hip2m1", "Ancient Realms"},		// 6
   {"hip2m2", "The Black Cathedral"},
   {"hip2m3", "The Catacombs"},
   {"hip2m4", "The Crypt"},
   {"hip2m5", "Mortum's Keep"},
   {"hip2m6", "The Gremlin's Domain"},

   {"hip3m1", "Tur Torment"},			// 12
   {"hip3m2", "Pandemonium"},
   {"hip3m3", "Limbo"},
   {"hip3m4", "The Gauntlet"},

   {"hipend", "Armagon's Lair"},		// 16

   {"hipdm1", "The Edge of Oblivion"}	// 17
};

//PGM 01/07/97 added rogue levels
//PGM 03/02/97 added dmatch level
level_t		roguelevels[] =
{
	{"start",	"Split Decision"},
	{"r1m1",	"Deviant's Domain"},
	{"r1m2",	"Dread Portal"},
	{"r1m3",	"Judgement Call"},
	{"r1m4",	"Cave of Death"},
	{"r1m5",	"Towers of Wrath"},
	{"r1m6",	"Temple of Pain"},
	{"r1m7",	"Tomb of the Overlord"},
	{"r2m1",	"Tempus Fugit"},
	{"r2m2",	"Elemental Fury I"},
	{"r2m3",	"Elemental Fury II"},
	{"r2m4",	"Curse of Osiris"},
	{"r2m5",	"Wizard's Keep"},
	{"r2m6",	"Blood Sacrifice"},
	{"r2m7",	"Last Bastion"},
	{"r2m8",	"Source of Evil"},
	{"ctf1",    "Division of Change"}
};

episode_t	id1episodes[] = // mankrip - renamed
{
	{"Welcome to Quake", 0, 1},
	{"Doomed Dimension", 1, 8},
	{"Realm of Black Magic", 9, 7},
	{"Netherworld", 16, 7},
	{"The Elder World", 23, 8},
	{"Final Level", 31, 1},
	{"Deathmatch Arena", 32, 6}
};

//MED 01/06/97  added hipnotic episodes
episode_t   hipnoticepisodes[] =
{
   {"Scourge of Armagon", 0, 1},
   {"Fortress of the Dead", 1, 5},
   {"Dominion of Darkness", 6, 6},
   {"The Rift", 12, 4},
   {"Final Level", 16, 1},
   {"Deathmatch Arena", 17, 1}
};

//PGM 01/07/97 added rogue episodes
//PGM 03/02/97 added dmatch episode
episode_t	rogueepisodes[] =
{
	{"Introduction", 0, 1},
	{"Hell's Fortress", 1, 7},
	{"Corridors of Time", 8, 8},
	{"Deathmatch Arena", 16, 1}
};

// mankrip - begin
level_t		levels[1024];
int			nummaps;

episode_t	episodes[16];
int			numepisodes = -1;

void M_Maps_Clear (void)
{
	int i;

	if ((key_dest == key_menu) && (m_state == m_gameoptions))
		return;
	if (nummaps)
		for (i=0; i<=nummaps; i++)
		{
			Z_Free (levels[i].name);
			Z_Free (levels[i].description);
		}
	if (numepisodes >= 0)
		for (i=0; i<=numepisodes; i++)
			Z_Free (episodes[i].description);
	nummaps = 0;
	numepisodes = -1;
}

void M_AddEpisode (void)
{
	if (Cmd_Argc() != 2)
	{
		Con_Printf ("menu_addepisode <name>\n");
		return;
	}
	if (numepisodes >= 0)
		if (episodes[numepisodes].levels == 0)
		{
			Con_Printf (va("menu_addepisode: first you must add map(s) to episode \"%s\"\n"), episodes[numepisodes].description);
			return;
		}
	if ((key_dest == key_menu) && (m_state == m_gameoptions))
		return;

	numepisodes++;

	episodes[numepisodes].description = Z_Malloc (strlen(Cmd_Argv(1))+1);
	strcpy (episodes[numepisodes].description, Cmd_Argv(1));
	episodes[numepisodes].firstLevel = nummaps;
}

void M_AddMap (void)
{
	char	*s;

	if (Cmd_Argc() != 3)
	{
		Con_Printf ("menu_addmap <filename> <description>\n");
		return;
	}
	if (numepisodes < 0)
	{
		Con_Printf ("menu_addmap: you must add an episode first\n");
		return;
	}
	if ((key_dest == key_menu) && (m_state == m_gameoptions))
		return;

	s = Z_Malloc (strlen(Cmd_Argv(1))+1);
	strcpy (s, Cmd_Argv(1));
	levels[nummaps].name = s;

	s = Z_Malloc (strlen(Cmd_Argv(2))+1);
	strcpy (s, Cmd_Argv(2));
	levels[nummaps].description = s;

	nummaps++;
	episodes[numepisodes].levels++;
}

void M_SetDefaultEpisodes (episode_t *myepisodes, int epcount, level_t *mylevels, int levcount)
{
	int i;
	numepisodes = epcount;
	nummaps = levcount;
	for (i=0; i<=numepisodes; i++)
	{
		episodes[i].description = Z_Malloc (strlen(myepisodes[i].description)+1);
		strcpy (episodes[i].description, myepisodes[i].description);
		episodes[i].firstLevel = myepisodes[i].firstLevel;
		episodes[i].levels = myepisodes[i].levels;
	}
	for (i=0; i<=nummaps; i++)
	{
		levels[i].name			= Z_Malloc (strlen(mylevels[i].name)+1);
		strcpy (levels[i].name			, mylevels[i].name);
		levels[i].description	= Z_Malloc (strlen(mylevels[i].description)+1);
		strcpy (levels[i].description	, mylevels[i].description);
	}
}

int o_deathmatch = 1;
int o_teamplay;
int o_skill = 1;
int o_fraglimit = 10;
int o_timelimit;
int o_samelevel;
// mankrip - end

int	startepisode;
int	startlevel;
int maxplayers;
qboolean m_serverInfoMessage = false;
double m_serverInfoMessageTime;

void M_GameOptions_f (void)
{
	// mankrip - begin
	if (key_dest == key_menu && m_state == m_gameoptions)
	{
		M_Off ();
		return;
	}
	// mankrip - end
	key_dest = key_menu;
	m_state = m_gameoptions;
	m_entersound = true;
	if (maxplayers == 0)
		maxplayers = svs.maxclients;
	if (maxplayers < 2)
		maxplayers = svs.maxclientslimit;
	// mankrip - begin
	startepisode = startlevel = 0;
	if (nummaps)
	{
		if (episodes[numepisodes].levels == 0) // remove last episode if empty
		{
			Z_Free (episodes[numepisodes].description);
			numepisodes--;
		}
	}
	else if (hipnotic)
		M_SetDefaultEpisodes(hipnoticepisodes, 5, hipnoticlevels, 17);
	else if (rogue)
		M_SetDefaultEpisodes(rogueepisodes, 3, roguelevels, 16);
	else
	{
		if (registered.value)
			M_SetDefaultEpisodes(id1episodes, 6, id1levels, 37);
		else
			M_SetDefaultEpisodes(id1episodes, 1, id1levels, 8);
	}
	// mankrip - end
}

#define	NUM_GAMEOPTIONS	10 // mankrip - edited

void M_GameOptions_Draw (void)
{
	int y = 20; // mankrip
	char *msg;

	M_DrawPlaque ("gfx/p_multi.lmp", true); // mankrip

	// mankrip - edited - begin
	M_Print (160, y+=8, "begin game");
	M_Print (0, y+=8, va("      Max players   %i", maxplayers));
	M_Print (0, y+=8, "        Game Type"); M_Print (160, y, !o_deathmatch ? "Cooperative" : va("Deathmatch %i", o_deathmatch));
	M_Print (0, y+=8, "         Teamplay");
	// mankrip - edited - end
	if (rogue)
		switch(o_teamplay)
		{
			case 1: msg = "No Friendly Fire"; break;
			case 2: msg = "Friendly Fire"; break;
			case 3: msg = "Tag"; break;
			case 4: msg = "Capture the Flag"; break;
			case 5: msg = "One Flag CTF"; break;
			case 6: msg = "Three Team CTF"; break;
			default: msg = "Off"; break;
		}
	else
		switch(o_teamplay)
		{
			case 1: msg = "No Friendly Fire"; break;
			case 2: msg = "Friendly Fire"; break;
			default: msg = "Off"; break;
		}
	M_Print (160, y, msg);

	// mankrip - begin
	M_Print (0, y+=8, "            Skill");	M_Print (160, y, skilltos(o_skill));
	M_Print (0, y+=8, "       Frag Limit"); M_Print (160, y, !o_fraglimit ? "none" : va("%i frags", o_fraglimit));
	M_Print (0, y+=8, "       Time Limit"); M_Print (160, y, !o_timelimit ? "none" : va("%i minutes", o_timelimit));
	M_Print (0, y+=8, "       Same Level"); M_DrawCheckbox (160, y, o_samelevel);
	// mankrip - end
	// mankrip - edited - begin
	M_Print (0, y+=8, "          Episode"); M_Print (160, y, episodes[startepisode].description);
	M_Print (0, y+=8, "            Level"); M_Print (160, y, levels[episodes[startepisode].firstLevel + startlevel].description);
	M_PrintWhite (160, y+=8, levels[episodes[startepisode].firstLevel + startlevel].name);
	// mankrip - edited - end

	// mankrip - begin
	M_DrawTextBox (56, 200 - 32, 24*8, 2*8);
	M_PrintText ("Changes take effect\nwhen starting a new game", 64, 200 - 24, 24, 2, ALIGN_CENTER);

	M_DrawCursor (144, 28, m_cursor[m_state]);
	// mankrip - end
}

void M_GameOptions_Change (int dir)
{
	// mankrip - begin
	int c = m_cursor[m_state];
	int i = 1;

	S_LocalSound ("misc/menu3.wav");

	if (c == i++)
	{
	// mankrip - end
		maxplayers += dir;
		if (maxplayers > svs.maxclientslimit)
		{
			maxplayers = svs.maxclientslimit;
			m_serverInfoMessage = true;
			m_serverInfoMessageTime = realtime;
		}
		if (maxplayers < 2)
			maxplayers = 2;
	// mankrip - begin
	}
	if (c == i++)
	{
		if ((o_deathmatch += dir) < 0)
			o_deathmatch = 3;
		if (o_deathmatch > 3)
			o_deathmatch = 0;
	}
	if (c == i++)
	{
		if ((o_teamplay += dir) < 0)
			o_teamplay = (rogue ? 6 : 2);
		if (o_teamplay > (rogue ? 6 : 2))
			o_teamplay = 0;
	}
	if (c == i++)
	{
		if ((o_skill += dir) < 0)
			o_skill = 3;
		if (o_skill > 3)
			o_skill = 0;
	}
	if (c == i++)
	{
		if ((o_fraglimit += dir*5) < 0)
			o_fraglimit = 100;
		if (o_fraglimit > 100)
			o_fraglimit = 0;
	}
	if (c == i++)
	{
		if ((o_timelimit += dir*5) < 0)
			o_timelimit = 60;
		if (o_timelimit > 60)
			o_timelimit = 0;
	}
	if (c == i++) o_samelevel = !o_samelevel;
	if (c == i++)
	{
	// mankrip - end
		startepisode += dir;
		if (startepisode < 0)
			startepisode = numepisodes; // mankrip - edited
		if (startepisode > numepisodes) // mankrip - edited
			startepisode = 0;
		startlevel = 0;
	// mankrip - begin
	}
	if (c == i++)
	{
	// mankrip - end
		startlevel += dir;
		if (startlevel < 0)
			startlevel = episodes[startepisode].levels - 1; // mankrip - edited
		if (startlevel >= episodes[startepisode].levels) // mankrip - edited
			startlevel = 0;
	} // mankrip
}

void M_GameOptions_Key (int key)
{
	if (m_inp_cancel)
#if NET_MENUS // mankrip - removed multiplayer menus
		M_Net_f ();
#else
		M_Main_f (); // mankrip
#endif
	else if (m_inp_up)
	{
		S_LocalSound ("misc/menu1.wav");
		if (--m_cursor[m_state] < 0) // mankrip
			m_cursor[m_state] = NUM_GAMEOPTIONS-1; // mankrip - edited
	}
	else if (m_inp_down)
	{
		S_LocalSound ("misc/menu1.wav");
		if (++m_cursor[m_state] >= NUM_GAMEOPTIONS) // mankrip
			m_cursor[m_state] = 0; // mankrip - edited
	}
	else if (m_inp_left)
		M_GameOptions_Change (-1);
	else if (m_inp_right)
		M_GameOptions_Change (1);
	if (m_inp_ok)
	{
		if (m_cursor[m_state] == 0) // mankrip - edited
		{
			// mankrip - begin
			char *s;
			s = Z_Malloc (Q_strlen(levels[episodes[startepisode].firstLevel + startlevel].name)+1);
			Q_strcpy (s, levels[episodes[startepisode].firstLevel + startlevel].name);
			// run "listen 0", so host_netport will be re-examined
			if (sv.active)
				M_PopUp_f("Are you sure you want to\nstart a new game?",
				va("disconnect\nlisten 0\nmaxplayers %u\ncoop %i\ndeathmatch %i\nteamplay %i\nskill %i\nfraglimit %i\ntimelimit %i\nsamelevel %i\nmap %s\n",
				maxplayers, !o_deathmatch, o_deathmatch, o_teamplay, o_skill, o_fraglimit, o_timelimit, o_samelevel, s));
			else
				Cbuf_AddText(va("disconnect\nlisten 0\nmaxplayers %u\ncoop %i\ndeathmatch %i\nteamplay %i\nskill %i\nfraglimit %i\ntimelimit %i\nsamelevel %i\nmap %s\n",
				maxplayers, !o_deathmatch, o_deathmatch, o_teamplay, o_skill, o_fraglimit, o_timelimit, o_samelevel, s));
			Z_Free(s);
			// mankrip - end
		}
		else if (!m_inp_left) // mankrip
			M_GameOptions_Change (1);
	}
}

//=============================================================================
/* OPTIONS MENU */

#ifdef _arch_dreamcast
#define	OPTIONS_ITEMS	12
#define QUICKHACK	1
#else
#define	OPTIONS_ITEMS	11
#define QUICKHACK	0
#endif

void M_Options_f (void)
{
	// mankrip - begin
	if (key_dest == key_menu && m_state == m_options)
	{
		M_Off ();
		return;
	}
	// mankrip - end
	key_dest = key_menu;
	m_state = m_options;
	m_entersound = true;
	M_Precache(); // mankrip - precaching menu pics
}

void M_Options_Draw (void)
{
	// mankrip - begin
	int y = 20;

	M_DrawPlaque ("gfx/p_option.lmp", true);

	M_Print (16, y += 8, "          Player setup    ...");
	// mankrip - end
	M_Print (16, y += 8, "    Customize controls    ..."); // mankrip - edited
	// mankrip - begin
#ifdef _arch_dreamcast
	M_Print (16, y += 8, "            Controller    ..."); // disabled - only works in the DC
#endif
	M_Print (16, y += 8, "                 Mouse    ...");
	M_Print (16, y += 8, "              Gameplay    ...");
	M_Print (16, y += 8, "               Display    ...");
	M_Print (16, y += 8, "                 Video    ...");
	M_Print (16, y += 8, "                 Audio    ...");
	// mankrip - end
	M_Print (16, y += 8, "         Go to console"); // mankrip - edited
	M_Print (16, y += 8, "            Save setup"); // mankrip - edited
	M_Print (16, y += 8, "            Load setup"); // mankrip - edited
	M_Print (16, y += 8, "     Reset to defaults"); // mankrip - edited

	M_DrawCursor (200, 28, m_cursor[m_state]); // mankrip
}


void M_Options_Key (int k)
{
	if (m_inp_cancel)
		M_Main_f ();
	else if (m_inp_up)
	{
		S_LocalSound ("misc/menu1.wav");
		if (--m_cursor[m_state] < 0) // mankrip
			m_cursor[m_state] = OPTIONS_ITEMS-1; // mankrip - edited
	}
	else if (m_inp_down)
	{
		S_LocalSound ("misc/menu1.wav");
		if (++m_cursor[m_state] >= OPTIONS_ITEMS) // mankrip
			m_cursor[m_state] = 0; // mankrip - edited
	}
	else if (m_inp_ok)
	{
		switch (m_cursor[m_state]) // mankrip - replaced cursor variables
		{
		// mankrip - begin
		case 0:
			M_Setup_f ();
			break;
		// mankrip - end
		case 1:
			M_Keys_f ();
			break;
		// mankrip - begin
#ifdef _arch_dreamcast
		case 2:
			M_Controller_f ();
			break;
#endif
		case 2+QUICKHACK:
			M_Mouse_f ();
			break;
		case 3+QUICKHACK:
			M_Gameplay_f ();
			break;
		case 4+QUICKHACK:
			M_Display_f ();
			break;
		case 5+QUICKHACK:
			M_Video_f ();
			break;
		case 6+QUICKHACK:
			M_Audio_f ();
			break;
		// mankrip - end
		case 7+QUICKHACK:
			m_state = m_none;
			Con_ToggleConsole_f ();
			break;
		// mankrip - begin
		case 8+QUICKHACK:
		// mankrip - VMU saves - begin
#ifdef _arch_dreamcast
			M_SelectVMU_f (m_saveoptions);
#else
		// mankrip - VMU saves - end
			m_entersound = true;
			Host_WriteConfiguration ();
#endif	// mankrip - VMU saves
			break;
		case 9+QUICKHACK:
		// mankrip - VMU saves - begin
#ifdef _arch_dreamcast
			M_SelectVMU_f (m_loadoptions);
#else
		// mankrip - VMU saves - end
			M_PopUp_f("Do you wish to load the\nsaved settings?", "exec config.cfg\n"); // mankrip
#endif	// mankrip - VMU saves
			break;
		case 10+QUICKHACK:
			M_PopUp_f("Do you wish to load the\ndefault settings?", "exec default.cfg\n"); // mankrip
			break;
		// mankrip - end
		default:
			break;
		}
	}
	else if (m_inp_backspace)
		if (m_cursor[m_state] == 7) // "Go to console" option
			M_Developer_f ();
}

//=============================================================================
/* SETUP MENU */

#if NET_MENUS // mankrip - removed multiplayer menus
int		setup_cursor_table[] = {8, 24, 48, 72, 104, 112}; // mankrip - edited
#else
int		setup_cursor_table[] = {8, 32, 56, 88, 96}; // mankrip - edited
#endif

#if NET_MENUS // mankrip - removed multiplayer menus
char	setup_hostname[16]; // Commented out by mankrip
#endif
char	setup_myname[16];
int		setup_oldtop;
int		setup_oldbottom;
int		setup_top;
int		setup_bottom;
int		setup_crosshair; // mankrip
int		setup_crosshair_color; // mankrip

#if NET_MENUS // mankrip - removed multiplayer menus
#define	NUM_SETUP_CMDS	6
#else
#define	NUM_SETUP_CMDS	5
#endif

void M_Setup_f (void)
{
	key_dest = key_menu;
	m_state = m_setup;
	m_entersound = true;
	Q_strcpy(setup_myname, cl_name.string);
#if NET_MENUS // mankrip - removed multiplayer menus
	Q_strcpy(setup_hostname, hostname.string);
#endif
	setup_top = setup_oldtop = ((int)cl_color.value) >> 4;
	setup_bottom = setup_oldbottom = ((int)cl_color.value) & 15;
	setup_crosshair = crosshair.value; // mankrip
	setup_crosshair_color = crosshair_color.value; // mankrip
	M_OnScreenKeyboard_Reset(); // mankrip
	M_Precache(); // mankrip - precaching menu pics
}

void M_Setup_Draw (void)
{
	int i=0, i1, i2;
	int y = 28 - 4; // mankrip

	M_DrawPlaque ("gfx/p_option.lmp", true); // mankrip

	// mankrip - edited - begin
#if NET_MENUS // mankrip - removed multiplayer menus
	M_Print (64, y + setup_cursor_table[i], "Hostname");
	M_DrawTextBox (160, y + setup_cursor_table[i] - 8, 16*8, 1*8);
	M_Print (168, y + setup_cursor_table[i++], setup_hostname);
#endif
	M_Print (64, y + setup_cursor_table[i], "Your name");
	M_DrawTextBox (160, y + setup_cursor_table[i] - 8, 16*8, 1*8);
	M_Print (168, y + setup_cursor_table[i++], setup_myname);

	M_Print (64, y + setup_cursor_table[i++]/*32*/, "Shirt color");
	M_Print (64, y + setup_cursor_table[i]/*56*/, "Pants color");

	M_DrawTransPic (160, y + setup_cursor_table[i]-40/*16*/, Draw_CachePic ("gfx/bigbox.lmp"));
	BuildColorTranslationTable (setup_top, setup_bottom, identityTable, translationTable);
	M_DrawTransPicTranslate (172, y + setup_cursor_table[i++]-32/*24*/, Draw_CachePic ("gfx/menuplyr.lmp"));
	// mankrip - edited - end
	// mankrip - crosshair - begin
	M_Print (64, y + setup_cursor_table[i++]/*88*/, "Crosshair");
	M_Print (64, y + setup_cursor_table[i]/*96*/, "Color");
	M_DrawTextBox (160, y + setup_cursor_table[i]-16/*80*/, 2*8 - 1, 2*8 - 1);
//	M_DrawTextBox (160, y + setup_cursor_table[i]-16/*80*/, ceil(1.5*(vid.width/320))*8 - 1, ceil(1.5*(vid.height<480 ? vid.height/200 : vid.height/240))*8 - 1);
//	if (!setup_crosshair_color)
//		Draw_Fill  (169 + ((vid.width - 320)>>1), y + setup_cursor_table[i]-7/*80*/, 4+11*(vid.width/320), 4+11*(vid.height<480 ? vid.height/200 : vid.height/240), 8);
	i1 = crosshair.value;
	i2 = crosshair_color.value;
	crosshair.value = setup_crosshair;
	crosshair_color.value = setup_crosshair_color;
	{
		extern void Crosshair_Set (void);
		extern byte crosshair_colormap[256];
		Crosshair_Set ();
		Crosshair_Draw (scr_2d_padding_x + 170 * scr_2d_scale_h, scr_2d_padding_y + (y + setup_cursor_table[i]-6/*91*/) * scr_2d_scale_v, crosshair_colormap);
		crosshair.value = i1;
		crosshair_color.value = i2;
		Crosshair_Set ();
	}
	// mankrip - crosshair - end

	M_DrawCursor (56, y + setup_cursor_table [m_cursor[m_state]], 0); // mankrip

	// mankrip - edited - begin
	if (m_cursor[m_state] == 0)
#if NET_MENUS // mankrip - removed multiplayer menus
		M_DrawCharacter (168 + 8*strlen(setup_hostname), y + setup_cursor_table [m_cursor[m_state]], 10+((int)(realtime*4)&1));

	if (m_cursor[m_state] == 1)
#endif
		M_DrawCharacter (168 + 8*strlen(setup_myname  ), y + setup_cursor_table [m_cursor[m_state]], 10+((int)(realtime*4)&1));
	// mankrip - edited - end
//24
	M_OnScreenKeyboard_Draw (setup_cursor_table [m_cursor[m_state]] + 12+20/*28 + 16*/); // mankrip
}


void M_Setup_Key (int k)
{
	int			l;

	// mankrip - begin
	#if NET_MENUS
	if (m_cursor[m_state] < 2)
	#else
	if (m_cursor[m_state] == 0)
	#endif
		k = M_OnScreenKeyboard_Key (k);
	// mankrip - end

	if (m_inp_off)
		m_inp_cancel = true;

	if (m_inp_cancel)
	{
		// mankrip - update player setup on exit - begin
		#if NET_MENUS // mankrip - removed multiplayer menus
		if (Q_strcmp(cl_name.string, setup_hostname) != 0)
			Cbuf_AddText ( va ("hostname \"%s\"\n", setup_hostname) );
		#endif
		if (Q_strcmp(cl_name.string, setup_myname) != 0)
			Cbuf_AddText ( va ("name \"%s\"\n", setup_myname) );
		if (setup_top != setup_oldtop || setup_bottom != setup_oldbottom)
			Cbuf_AddText( va ("color %i %i\n", setup_top, setup_bottom) );
		Cvar_SetValue ("crosshair", setup_crosshair);
		Cvar_SetValue ("crosshair_color", setup_crosshair_color);
		if (m_inp_off)
			M_Off ();
		else
			M_Options_f ();
		// mankrip - update player setup on exit - end
//		M_MultiPlayer_f (); // Commented out by mankrip
	}
	else if (m_inp_up)
	{
		S_LocalSound ("misc/menu1.wav");
		if (--m_cursor[m_state] < 0) // mankrip
			m_cursor[m_state] = NUM_SETUP_CMDS-1; // mankrip - edited
	}
	else if (m_inp_down)
	{
		S_LocalSound ("misc/menu1.wav");
		if (++m_cursor[m_state] >= NUM_SETUP_CMDS) // mankrip
			m_cursor[m_state] = 0; // mankrip - edited
	}
	else if (m_inp_left)
	{
		if (m_cursor[m_state] < NUM_SETUP_CMDS-4) // mankrip
			return;
		S_LocalSound ("misc/menu3.wav");
		if (m_cursor[m_state] == NUM_SETUP_CMDS-4) // mankrip
			setup_top = setup_top - 1;
		if (m_cursor[m_state] == NUM_SETUP_CMDS-3) // mankrip
			setup_bottom = setup_bottom - 1;
		// mankrip - begin
		if (m_cursor[m_state] == NUM_SETUP_CMDS-2)
		{
			setup_crosshair -= 1;
			if (setup_crosshair < 0)
				setup_crosshair = 5;
		}
		if (m_cursor[m_state] == NUM_SETUP_CMDS-1)
		{
			setup_crosshair_color -= 1;
			if (setup_crosshair_color < 0)
				setup_crosshair_color = 17;
		}
		// mankrip - end
	}
	else if (m_inp_right)
	{
		if (m_cursor[m_state] < NUM_SETUP_CMDS-4) // mankrip
			return;
forward:
		S_LocalSound ("misc/menu3.wav");
		if (m_cursor[m_state] == NUM_SETUP_CMDS-4) // mankrip
			setup_top = setup_top + 1;
		if (m_cursor[m_state] == NUM_SETUP_CMDS-3) // mankrip
			setup_bottom = setup_bottom + 1;
		// mankrip - begin
		if (m_cursor[m_state] == NUM_SETUP_CMDS-2)
		{
			setup_crosshair += 1;
			if (setup_crosshair > 5)
				setup_crosshair = 0;
		}
		if (m_cursor[m_state] == NUM_SETUP_CMDS-1)
		{
			setup_crosshair_color += 1;
			if (setup_crosshair_color > 17)
				setup_crosshair_color = 0;
		}
		// mankrip - end
	}
	else if (m_inp_ok)
	{
#if NET_MENUS // mankrip - removed multiplayer menus
		if (m_cursor[m_state] == 0 || m_cursor[m_state] == 1) // mankrip
#else
		if (m_cursor[m_state] == 0) // mankrip
#endif
			return;
		if (m_cursor[m_state] > 0 && m_cursor[m_state] < NUM_SETUP_CMDS) // mankrip
			goto forward;
	}
	else if (k == K_BACKSPACE)
	{
		if (m_cursor[m_state] == 0) // mankrip - edited
#if NET_MENUS // mankrip - removed multiplayer menus
		{
			if (strlen(setup_hostname))
				setup_hostname[strlen(setup_hostname)-1] = 0;
		}

		if (m_cursor[m_state] == 1) // mankrip - edited
#endif
		{
			if (strlen(setup_myname))
				setup_myname[strlen(setup_myname)-1] = 0;
		}
	}
	else if (k > 31 && k < 128)
	{
		if (m_cursor[m_state] == 0) // mankrip - edited
#if NET_MENUS // mankrip - removed multiplayer menus
		{
			l = strlen(setup_hostname);
			if (l < 15)
			{
				setup_hostname[l+1] = 0;
				setup_hostname[l] = k;
			}
		}
		if (m_cursor[m_state] == 1) // mankrip - edited
#endif
		{
			l = strlen(setup_myname);
			if (l < 15)
			{
				setup_myname[l+1] = 0;
				setup_myname[l] = k;
			}
		}
	}
	if (m_cursor[m_state] != 0) // mankrip
#if NET_MENUS // mankrip - removed multiplayer menus
	if (m_cursor[m_state] != 1) // mankrip
#endif
		M_OnScreenKeyboard_Reset(); // mankrip

	if (setup_top > 13)
		setup_top = 0;
	if (setup_top < 0)
		setup_top = 13;
	if (setup_bottom > 13)
		setup_bottom = 0;
	if (setup_bottom < 0)
		setup_bottom = 13;
}

//=============================================================================
/* KEYS MENU */

// mankrip - begin
#define	MAXKEYS 16
#define MAXCMDS 256

qboolean dont_bind[K_NUMKEYS];//256
char	*bindnames[K_NUMKEYS][2];//256
int		numcommands;
int		m_keys_pages;
//int		m_keys_page_start[16]; // first command on the page
int		cmd_position[256];//[NUMCOMMANDS]; TO DO: replace this
int		cmd_for_position[256]; // TO DO: replace by [NUMCOMMANDS]
int		keys_for_cmd[MAXCMDS];
int		keyliststart;
// mankrip - end
int		bind_grab;

// mankrip - begin
void M_Keys_Bindable (void)
{
	if (Cmd_Argc() != 3)
	{
		Con_Printf ("bindable <key> <0/1>\n");
		return;
	}
	dont_bind[Key_StringToKeynum(Cmd_Argv(1))] = !Q_atof(Cmd_Argv(2));
}
// mankrip - end

void M_UnbindCommand (char *command)
{
	// unbinds all keys bound to the specified command
	int		j;
	int		l;
	char	*b;

	l = strlen(command);

	for (j=0 ; j<K_NUMKEYS ; j++)
	{
		if (dont_bind[j]) // mankrip
			continue; // mankrip
		b = keybindings[j];
		if (b) // mankrip - edited
			if (!strcmp (b, command))//, l) ) // mankrip - edited
				Key_SetBinding (j, "");
		// mankrip - begin
		b = shiftbindings[j];
		if (b)
			if (!strcmp (b, command))//, l) )
				Key_SetShiftBinding (j, "");
		// mankrip - end
	}
}

void M_FindKeysForCommand (char *command, int *keys) // mankrip - edited
{
	// Finds how many keys are bound to the specified command
	int		count;
	int		j;
	int		l;
	char	*b;

	// mankrip - begin
	for (count=0 ; count<MAXKEYS ; count++)
		keys[count] = -1;
	// mankrip - end

	l = strlen(command);
	for (count=j=0 ; (j<K_NUMKEYS) || (count == MAXKEYS) ; j++) // mankrip - edited
	{
		// look at each key
		b = keybindings[j];
		// mankrip - edited - begin
		if (b) // if there is any command bound to this key,
		if (!strcmp (b, command))//, l) ) //strncmp // compare the string bound to the key with the string of the command
		{
			keys[count] = j; // puts the number of the key into the slot of the array
		// mankrip - edited - end
			count++;		 // select the next slot in the array
			if (count == MAXKEYS) // mankrip - edited
				break;
		}
		// mankrip - begin
		// same stuff, now for shifted keys
		b = shiftbindings[j];
		if (b)
		if (!strcmp (b, command))//, l) ) //strncmp
		{
			keys[count] = -j; // dirty hack, using negative integers to identify the shifted keys
			count++;
		}
		// mankrip - end
	}
}

// mankrip - begin
void M_FillKeyList (void)
{
	int		i, i2, position;
	int		keys[MAXKEYS];
	position = 0;

	// finds how many keys are set for each command

	// it assumes that at least one key is bound, for practicity purposes, because the menu
	// always displays at least one line (either the key or the "---") for each command
	for (i=0 ; i<MAXCMDS ; i++)
		keys_for_cmd[i] = 1;
	for (i=0 ; i<numcommands ; i++)
	{
		cmd_for_position[position] = i;
		M_FindKeysForCommand (bindnames[i][0], keys);
		for (i2=1 ; i2<MAXKEYS ; i2++)
		{
			if (keys[i2] != -1)
			{
				keys_for_cmd[i]++;
				cmd_for_position[position + i2] = i;
			}
			else break;
		}
		cmd_position[i] = position; // the position of the command in the menu
		position += keys_for_cmd[i];
	}
}

void M_Keys_Clear (void)
{
	int i;
	if ((key_dest == key_menu) && (m_state == m_keys))
		return;
	for (i=0; i<numcommands; i++)
	{
		Z_Free (bindnames [i][0]);
		Z_Free (bindnames [i][1]);
	}
	keyliststart = m_cursor[m_state] = numcommands = 0;
}

void M_Keys_AddCmd (void)
{
	char	*s;

	if (Cmd_Argc() != 3)
	{
		Con_Printf ("menu_keys_addcmd <command> <description>\n");
		return;
	}
	if ((key_dest == key_menu) && (m_state == m_keys))
		return;

	s = Z_Malloc (strlen(Cmd_Argv(1))+1);
	strcpy (s, Cmd_Argv(1));
	bindnames [numcommands][0] = s;

	s = Z_Malloc (strlen(Cmd_Argv(2))+1);
	strcpy (s, Cmd_Argv(2));
	bindnames [numcommands][1] = s;

	numcommands++;
}
// mankrip - end

// mankrip - reordered the default commands
char *defaultbindnames[][2] = // mankrip - renamed
{
{"+shift",			"function shift"}, // mankrip
{"+use",			"use"}, // mankrip
{"+attack", 		"attack"},
{"impulse 10", 		"next weapon"},
{"impulse 12",		"previous weapon"}, // BlackAura (11-12-2002) - Next/prev weapons
// mankrip - begin
{"impulse 1",		"Axe"},
{"impulse 2",		"Shotgun"},
{"impulse 3",		"Super shotgun"},
{"impulse 4",		"Nailgun"},
{"impulse 5",		"Super nailgun"},
{"impulse 6",		"Grenade launcher"},
{"impulse 7",		"Rocket launcher"},
{"impulse 8",		"Thunderbolt"},
// mankrip - end
{"+jump", 			"jump / swim up"},
{"+moveup",			"swim up"},
{"+movedown",		"swim down"},
{"+lookup", 		"look up"},
{"+lookdown", 		"look down"},
{"+left", 			"turn left"},
{"+right", 			"turn right"},
{"+forward", 		"walk forward"},
{"+back", 			"backpedal"},
{"+moveleft", 		"step left"},
{"+moveright", 		"step right"},
{"+strafe", 		"sidestep"},
{"+speed", 			"run"},
{"+klook", 			"keyboard look"},
{"centerview", 		"center view"},
{"+showscores",		"show scores"} // mankrip
};
#define	NUMCOMMANDS (sizeof(defaultbindnames)/sizeof(defaultbindnames[0]))

// mankrip - begin
void M_Keys_SetDefaultCmds (void)
{
	int i;

	keyliststart = m_cursor[m_state] = 0;
	numcommands = NUMCOMMANDS;
	for (i=0; i<numcommands; i++)
	{
		bindnames[i][0] = Z_Malloc (strlen(defaultbindnames[i][0])+1);
		strcpy (bindnames[i][0], defaultbindnames[i][0]);
		bindnames[i][1] = Z_Malloc (strlen(defaultbindnames[i][1])+1);
		strcpy (bindnames[i][1], defaultbindnames[i][1]);
	}
}
// mankrip - end

void M_Keys_f (void)
{
	// mankrip - begin
	if (key_dest == key_menu && m_state == m_keys)
	{
		M_Off ();
		return;
	}
	// mankrip - end
	key_dest = key_menu;
	m_state = m_keys;
	m_entersound = true;
	// mankrip - begin
	bind_grab = false;
	if (numcommands == 0)
		M_Keys_SetDefaultCmds();
	// mankrip - end
	M_Precache(); // mankrip - precaching menu pics
}

void M_Keys_Draw (void)
{
	// mankrip - begin
	int		i, i2;
	int		keys[MAXKEYS];
	int	/*	x,*/ y;
	int		top, bottom;

	M_DrawPlaque ("gfx/p_option.lmp", true); // ttl_cstm mankrip

//	m_keys_pages = 1; // for testing purposes

	if (m_keys_pages > 0)
		M_Print (128, 28, "Page 1/1");
	top = 28 + (m_keys_pages > 0)*16;
	bottom = 200 - 48; // vid.height, textbox height

	M_DrawTextBox (0, bottom + 8, 37*8, 3*8);
	if (bind_grab)
		M_PrintText ("Hold Function Shift for \"FS + button\"\nPress a key or button for this action\nPress Start or ESC to cancel", 12, bottom + 16, 37, 3, ALIGN_CENTER);
	else
		M_PrintText ("A button or Enter to change\nX button or Backspace to clear\nDelete to clear all keys for command", 12, bottom + 16, 37, 3, ALIGN_CENTER);

	// search for known bindings
	y = top - (keyliststart*8) - 8;
	// mankrip - end
	for (i=0 ; i<numcommands; i++) // mankrip - edited
	{
		M_FindKeysForCommand (bindnames[i][0], keys);
		// mankrip - begin
		if (((y+16) > top) && (y+7 < bottom))
		{
			// prints the description of the command
			M_Print (56, y += 8, bindnames[i][1]);
			// prints the first key bound to this command
			if (keys[0] == -1)
				M_PrintWhite (196, y, "---");
			else if (keys[0] < 0)
				M_Print (196, y, va("FS + %s", Key_KeynumToString (-keys[0])));
			else
				M_Print (196, y, Key_KeynumToString (keys[0]));
		}
		else
			y += 8;
		// prints the other keys bound to this command
		for (i2=1 ; i2<MAXKEYS ; i2++)
			if ((keys[i2] != -1) && (y+7 < bottom))
			{
				if ((y + 16) > top)
				{
					if (keys[i2] < 0)
						M_Print (196, y += 8, va("FS + %s", Key_KeynumToString (-keys[i2])));
					else
						M_Print (196, y += 8, Key_KeynumToString (keys[i2]));
				}
				else
					y += 8;
			}
			else break;
		// mankrip - end
	}

	M_FillKeyList(); // mankrip
	if (bind_grab)
		M_DrawCharacter (186, top + (cmd_position[cmd_for_position[m_cursor[m_state]]] - keyliststart)*8, '='); // mankrip
	else
		M_DrawCursor (186, top, (m_cursor[m_state] - keyliststart)); // mankrip
}

void M_Keys_Key (int k)
{
	char	cmd[80];
	int		keys[MAXKEYS]; // mankrip
	int		menulines = (200 - 28 - (m_keys_pages>0)*16 - 48) / 8; // mankrip

	M_FillKeyList(); // mankrip

	if (bind_grab)
	{	// defining a key
		S_LocalSound ("misc/menu3.wav"); // Edited by mankrip
		// mankrip - begin
		if (k>='A' && k<='Z')
			k = k + 'a' - 'A'; // convert upper case characters to lower case
		// mankrip - end
		if ((k == K_ESCAPE) || m_inp_off) // mankrip - edited
			bind_grab = false;
		// mankrip - begin
		else if (keybindings[k] && !Q_strcmp(keybindings[k], "+shift"))
			return;
		else if (dont_bind[k] == false) // (k != '`')
		{
			if (shift_function)
				sprintf (cmd, "bindshift \"%s\" \"%s\"\n", Key_KeynumToString (k), bindnames[cmd_for_position[m_cursor[m_state]]][0]);
			else
		// mankrip - end
				sprintf (cmd, "bind \"%s\" \"%s\"\n", Key_KeynumToString (k), bindnames[cmd_for_position[m_cursor[m_state]]][0]); // mankrip - edited
			Cbuf_InsertText (cmd);
			bind_grab = false;
		}
		return;
	}

	if (m_inp_off)
		M_Off ();
	else if (m_inp_cancel)
		M_Options_f ();
	else if (m_inp_up)
	{
		S_LocalSound ("misc/menu1.wav");
		// mankrip - begin
		if (--m_cursor[m_state] < 0)
		{
			m_cursor[m_state] = cmd_position[numcommands-1] + keys_for_cmd[numcommands-1] -1;
			keyliststart = m_cursor[m_state] - menulines;
		}
		if (m_cursor[m_state] < keyliststart)
			keyliststart = m_cursor[m_state];
		if (keyliststart < 0)
			keyliststart = 0;
		// mankrip - end
	}
	else if (m_inp_down)
	{
		S_LocalSound ("misc/menu1.wav");
		// mankrip - begin
		if (++m_cursor[m_state] >= cmd_position[numcommands-1] + keys_for_cmd[numcommands-1])
			keyliststart = m_cursor[m_state] = 0;
		if (m_cursor[m_state] > keyliststart + menulines)
			keyliststart = m_cursor[m_state] - menulines;
		// mankrip - end
	}
	else if (m_inp_ok)
	{
		S_LocalSound ("misc/menu3.wav"); // Edited by mankrip
		bind_grab = true;
		if (keyliststart > cmd_position[cmd_for_position[m_cursor[m_state]]]) // mankrip
			keyliststart = cmd_position[cmd_for_position[m_cursor[m_state]]]; // mankrip
	}
	else if (m_inp_backspace) // delete binding
	{
		// mankrip - begin
		S_LocalSound ("misc/menu3.wav");
		M_FindKeysForCommand (bindnames[cmd_for_position[m_cursor[m_state]]][0], keys);
		k = keys[m_cursor[m_state] - cmd_position[cmd_for_position[m_cursor[m_state]]]];
		if (dont_bind[k * -(k < 0)])
			return;
		if (k < 0)
			Key_SetShiftBinding (-k, "");
		else
			Key_SetBinding (k, "");
		M_FillKeyList();
		if (m_cursor[m_state] >= cmd_position[numcommands-1] + keys_for_cmd[numcommands-1])
		{
			m_cursor[m_state]  = cmd_position[numcommands-1] + keys_for_cmd[numcommands-1] -1;
			keyliststart = m_cursor[m_state] - menulines;
		}
		// mankrip - end
	}
	else if (k == K_DEL) // delete multiple bindings
	{
		S_LocalSound ("misc/menu3.wav"); // mankrip - edited
		M_UnbindCommand (bindnames[cmd_for_position[m_cursor[m_state]]][0]); // mankrip - edited
	}
}

// mankrip - begin
//=============================================================================
/* CONTROLLER OPTIONS MENU */
#define	CONTROLLER_ITEMS	8

void M_Controller_f (void)
{
	key_dest = key_menu;
	m_state = m_controller;
	m_entersound = true;
}
void M_Controller_Draw (void)
{
	int y = 20;

	M_DrawPlaque ("gfx/p_option.lmp", true);

	M_Print (16, y+=8, "       Axis X Deadzone"); M_DrawSlider (220, y, axis_x_threshold.value * 2);
	M_Print (16, y+=8, "       Axis Y Deadzone"); M_DrawSlider (220, y, axis_y_threshold.value * 2);
	M_Print (16, y+=8, "    Trigger L Deadzone"); M_DrawSlider (220, y, axis_l_threshold.value * 2);
	M_Print (16, y+=8, "    Trigger R Deadzone"); M_DrawSlider (220, y, axis_r_threshold.value * 2);
	M_Print (16, y+=8, "    Axis X Sensitivity"); M_DrawSlider (220, y, axis_x_scale.value - 0.5);
	M_Print (16, y+=8, "    Axis Y Sensitivity"); M_DrawSlider (220, y, axis_y_scale.value - 0.5);
	M_Print (16, y+=8, "         L Sensitivity"); M_DrawSlider (220, y, axis_l_scale.value - 0.5);
	M_Print (16, y+=8, "         R Sensitivity"); M_DrawSlider (220, y, axis_r_scale.value - 0.5);

	M_DrawCursor (200, 28, m_cursor[m_state]);
}
void M_Controller_Change (int dir)
{
	int c = m_cursor[m_state];
	int i = 0;

	S_LocalSound ("misc/menu3.wav");

	// threshold
	if (c == i++) ChangeCVar("dc_axisx_threshold", axis_x_threshold.value, dir * 0.05, 0, 0.5, true);
	if (c == i++) ChangeCVar("dc_axisy_threshold", axis_y_threshold.value, dir * 0.05, 0, 0.5, true);
	if (c == i++) ChangeCVar("dc_axisl_threshold", axis_l_threshold.value, dir * 0.05, 0, 0.5, true);
	if (c == i++) ChangeCVar("dc_axisr_threshold", axis_r_threshold.value, dir * 0.05, 0, 0.5, true);
	// sensitivity
	if (c == i++) ChangeCVar("dc_axisx_scale", axis_x_scale.value, dir * 0.1, 0.5, 1.5, true);
	if (c == i++) ChangeCVar("dc_axisy_scale", axis_y_scale.value, dir * 0.1, 0.5, 1.5, true);
	if (c == i++) ChangeCVar("dc_axisl_scale", axis_l_scale.value, dir * 0.1, 0.5, 1.5, true);
	if (c == i++) ChangeCVar("dc_axisr_scale", axis_r_scale.value, dir * 0.1, 0.5, 1.5, true);
}
void M_Controller_Key (int k)
{
	if (m_inp_cancel)
		M_Options_f ();
	else if (m_inp_up)
	{
		S_LocalSound ("misc/menu1.wav");
		if (--m_cursor[m_state] < 0)
			m_cursor[m_state] = CONTROLLER_ITEMS-1;
	}
	else if (m_inp_down)
	{
		S_LocalSound ("misc/menu1.wav");
		if (++m_cursor[m_state] >= CONTROLLER_ITEMS)
			m_cursor[m_state] = 0;
	}
	else if (m_inp_left)
		M_Controller_Change (-1);
	else if (m_inp_right || m_inp_ok)
		M_Controller_Change (1);
}
//=============================================================================
/* MOUSE CONFIG MENU */
#define	MOUSE_ITEMS	5

void M_Mouse_f (void)
{
	key_dest = key_menu;
	m_state = m_mouse;
	m_entersound = true;
}
void M_Mouse_Draw (void)
{
	int y = 20;

	M_DrawPlaque ("gfx/p_option.lmp", true);

	M_Print (16, y+=8, "           Sensitivity"); M_DrawSlider (220, y, (sensitivity.value - 1)/10);
	M_Print (16, y+=8, "            Walk Speed"); M_DrawSlider (220, y, m_forward.value - 0.5);
	M_Print (16, y+=8, "          Strafe Speed"); M_DrawSlider (220, y, m_side.value - 0.5);
	M_Print (16, y+=8, "          Invert Mouse"); M_DrawCheckbox (220, y, m_pitch.value < 0);
	M_Print (16, y+=8, "            Mouse Look"); M_DrawCheckbox (220, y, m_look.value);

	M_DrawCursor (200, 28, m_cursor[m_state]);
}
void M_Mouse_Change (int dir)
{
	int c = m_cursor[m_state];
	int i = 0;

	S_LocalSound ("misc/menu3.wav");

	if (c == i++) ChangeCVar("sensitivity", sensitivity.value, dir * 0.5, 1, 11, true);
	if (c == i++) ChangeCVar("m_forward", m_forward.value, dir * 0.1, 0.5, 1.5, true);
	if (c == i++) ChangeCVar("m_side", m_side.value, dir * 0.1, 0.5, 1.5, true);
	if (c == i++) Cvar_SetValue ("m_pitch", -m_pitch.value);
	if (c == i++) Cvar_SetValue ("m_look", !m_look.value);
}
void M_Mouse_Key (int k)
{
	if (m_inp_cancel)
		M_Options_f ();
	else if (m_inp_up)
	{
		S_LocalSound ("misc/menu1.wav");
		if (--m_cursor[m_state] < 0)
			m_cursor[m_state] = MOUSE_ITEMS-1;
	}
	else if (m_inp_down)
	{
		S_LocalSound ("misc/menu1.wav");
		if (++m_cursor[m_state] >= MOUSE_ITEMS)
			m_cursor[m_state] = 0;
	}
	else if (m_inp_left)
		M_Mouse_Change (-1);
	else if (m_inp_right || m_inp_ok)
		M_Mouse_Change (1);
}
//=============================================================================
/* GAMEPLAY OPTIONS MENU */
#ifdef _arch_dreamcast
#define	GAMEPLAY_ITEMS	14
#else
#define	GAMEPLAY_ITEMS	13
#endif

void M_Gameplay_f (void)
{
	key_dest = key_menu;
	m_state = m_gameplay;
	m_entersound = true;
}
void M_Gameplay_Draw (void)
{
	int y = 20;

	M_DrawPlaque ("gfx/p_option.lmp", true);

	M_Print (16, y+=8, "        Aim Assistance");// M_DrawCheckbox (220, y, sv_aim.value < 1);
	if (sv_aim_h.value < 1)
		M_Print (220, y, "on");
	else if (sv_aim.value < 1)
		M_Print (220, y, "vertical");
	else
		M_Print (220, y, "off");
	M_Print (16, y+=8, "       Weapon Position");
	if (scr_ofsy.value > 0)
		M_Print (220, y, "Left");
	else if (scr_ofsy.value < 0)
		M_Print (220, y, "Right");
	else
		M_Print (220, y, "Center");
	M_Print (16, y+=8, "    View Player Weapon"); M_DrawCheckbox (220, y, r_drawviewmodel.value);
	M_Print (16, y+=8, "               Bobbing"); M_DrawCheckbox (220, y, !cl_nobob.value);
	M_Print (16, y+=8, "            Slope Look"); M_DrawCheckbox (220, y, sv_idealpitchscale.value > 0);
	M_Print (16, y+=8, "      Auto Center View"); M_DrawCheckbox (220, y, lookspring.value);
	M_Print (16, y+=8, "            Lookstrafe"); M_DrawCheckbox (220, y, lookstrafe.value);
	M_Print (16, y+=8, "            Always Run"); M_DrawCheckbox (220, y, cl_forwardspeed.value > 200);
	M_Print (16, y+=8, "    Allow \"use\" button"); M_DrawCheckbox (220, y, sv_enable_use_button.value);
#ifdef _arch_dreamcast
	M_Print (16, y+=8, "             Vibration");
	if (cl_vibration.value == 2)
		M_Print (220, y, "QC only");
	else
		M_DrawCheckbox (220, y, cl_vibration.value);
#endif
	M_Print (16, y+=8, "             Body View"); M_DrawCheckbox (220, y, chase_active.value);
	M_Print (16, y+=8, "    Body View Distance"); M_DrawSlider (220, y, (chase_back.value - 50) / 100);
	M_Print (16, y+=8, "      Body View Height"); M_DrawSlider (220, y, chase_up.value / 50);
	M_Print (16, y+=8, "       Body View Speed"); M_DrawSlider (220, y, (chase_speed.value -10) / 200);

	M_DrawCursor (200, 28, m_cursor[m_state]);
}
void M_Gameplay_Change (int dir)
{
	int c = m_cursor[m_state];
	int i = 0;

	S_LocalSound ("misc/menu3.wav");

	if (c == i++)
	{
//		(sv_aim.value < 1) ? Cvar_SetValue ("sv_aim", 1) : Cvar_SetValue ("sv_aim", 0.93);
		if (dir == 1)
		{
			if (sv_aim_h.value < 1) // on
			{
				Cvar_SetValue ("sv_aim_h", 1);
				Cvar_SetValue ("sv_aim", 1);
			}
			else if (sv_aim.value < 1) // vertical
				Cvar_SetValue ("sv_aim_h", 0.98);
			else // off
				Cvar_SetValue ("sv_aim", 0.93);
		}
		else
		{
			if (sv_aim_h.value < 1) // on
			{
				Cvar_SetValue ("sv_aim_h", 1);
				Cvar_SetValue ("sv_aim", 0.93);
			}
			else if (sv_aim.value < 1) // vertical
				Cvar_SetValue ("sv_aim", 1);
			else // off
				Cvar_SetValue ("sv_aim_h", 0.98);
		}
	}
	if (c == i++) ChangeCVar("scr_ofsy", scr_ofsy.value, dir * -7, -7, 7, false);
	if (c == i++) Cvar_SetValue ("r_drawviewmodel", !r_drawviewmodel.value);
	if (c == i++) Cvar_SetValue ("cl_nobob", !cl_nobob.value);
	if (c == i++) Cvar_SetValue ("sv_idealpitchscale", !sv_idealpitchscale.value * 0.8);
	if (c == i++) Cvar_SetValue ("lookspring", !lookspring.value);
	if (c == i++) Cvar_SetValue ("lookstrafe", !lookstrafe.value);
	if (c == i++) // always run
	{
		if (cl_forwardspeed.value > 200)
		{
			Cvar_SetValue ("cl_forwardspeed", 200);
			Cvar_SetValue ("cl_backspeed", 200);
		}
		else
		{
			Cvar_SetValue ("cl_forwardspeed", 400);
			Cvar_SetValue ("cl_backspeed", 400);
		}
	}
	if (c == i++) Cvar_SetValue ("sv_enable_use_button", !sv_enable_use_button.value);
#ifdef _arch_dreamcast
	if (c == i++) ChangeCVar("cl_vibration", cl_vibration.value, dir, 0, 2, false);
#endif
	if (c == i++) Cvar_SetValue ("chase_active", !chase_active.value);
	if (c == i++) ChangeCVar("chase_back", chase_back.value, dir * 10, 50, 150, true);
	if (c == i++) ChangeCVar("chase_up", chase_up.value, dir * 5, 0, 50, true);
	if (c == i++) ChangeCVar("chase_speed", chase_speed.value, dir * 10, 10, 210, true);
}
void M_Gameplay_Key (int k)
{
	if (m_inp_cancel)
		M_Options_f ();
	else if (m_inp_up)
	{
		S_LocalSound ("misc/menu1.wav");
		if (--m_cursor[m_state] < 0)
			m_cursor[m_state] = GAMEPLAY_ITEMS-1;
	}
	else if (m_inp_down)
	{
		S_LocalSound ("misc/menu1.wav");
		if (++m_cursor[m_state] >= GAMEPLAY_ITEMS)
			m_cursor[m_state] = 0;
	}
	else if (m_inp_left)
		M_Gameplay_Change (-1);
	else if (m_inp_right || m_inp_ok)
		M_Gameplay_Change (1);
}
//=============================================================================
/* AUDIO OPTIONS MENU */
#ifndef _arch_dreamcast
#ifndef _WIN32
#define	AUDIO_ITEMS	13
#else
#define	AUDIO_ITEMS	12
#endif
#else
#define	AUDIO_ITEMS	9
#endif
byte cd_cursor;

void M_Audio_f (void)
{
	key_dest = key_menu;
	m_state = m_audio;
	m_entersound = true;
	cd_cursor = playTrack;
}

void M_Audio_Draw (void)
{
	int y = 20;

	M_DrawPlaque ("gfx/p_option.lmp", true);

#ifndef _arch_dreamcast
	M_Print (16, y+=8, "    load audio as 8bit"); M_DrawCheckbox (220, y, loadas8bit.value);
#endif
	M_Print (16, y+=8, "          Stereo Sound"); M_DrawCheckbox (220, y, snd_stereo.value);
	M_Print (16, y+=8, "     Swap L/R Channels"); M_DrawCheckbox (220, y, snd_swapstereo.value);
	M_Print (16, y+=8, "          Sound Volume"); M_DrawSlider (220, y, volume.value);
#ifndef _WIN32
	M_Print (16, y+=8, "       CD Music Volume"); M_DrawSlider (220, y, bgmvolume.value);
#endif
	M_Print (16, y+=8, "              CD Music"); M_DrawCheckbox (220, y, cd_enabled.value);
	M_Print (16, y+=8, "            Play Track");
	if (cdValid)
	{
		(cd_cursor == playTrack) ? M_Print (220, y, va("%u", cd_cursor)) : M_PrintWhite (220, y, va("%u", cd_cursor)); M_Print (244, y, va("/ %u", maxTrack));
	}
	else if (maxTrack)
		M_Print (220, y, "No tracks");
	else
		M_Print (220, y, "Drive empty");
	M_Print (16, y+=8, "                  Loop"); M_DrawCheckbox (220, y, playLooping);
	M_Print (16, y+=8, "                 Pause"); M_DrawCheckbox (220, y, !playing && wasPlaying);
	M_Print (16, y+=8, "                  Stop");
#ifndef _arch_dreamcast
	M_Print (16, y+=8, "            Open Drive");
	M_Print (16, y+=8, "           Close Drive");
	M_Print (16, y+=8, "        Reset CD Audio");
#endif
	M_DrawCursor (200, 28, m_cursor[m_state]);
}
void M_Audio_Change (int dir)
{
	int c = m_cursor[m_state];
	int i = 0;

	S_LocalSound ("misc/menu3.wav");

#ifndef _arch_dreamcast
	if (c == i++) Cvar_SetValue ("loadas8bit", !loadas8bit.value);
#endif
	if (c == i++) Cvar_SetValue ("snd_stereo", !snd_stereo.value);
	if (c == i++) Cvar_SetValue ("snd_swapstereo", !snd_swapstereo.value);
	if (c == i++) ChangeCVar("volume", volume.value, dir * 0.1, 0, 1, true);
#ifndef _WIN32
	if (c == i++) ChangeCVar("bgmvolume", bgmvolume.value, dir * 0.1, 0, 1, true);
#endif
	if (c == i++) Cvar_SetValue ("cd_enabled", !cd_enabled.value);
	if (c == i++)
	{
		if (!cdValid)
			return;
		do
		{
			cd_cursor += dir;
			if (cd_cursor > maxTrack)
				cd_cursor = 1;
			if (cd_cursor < 1)
				cd_cursor = maxTrack;
		}
		while (audioTrack[cd_cursor] == false);
	}
	if (c == i++) playLooping = !playLooping;
	if (c == i++) (!playing && wasPlaying) ? CDAudio_Resume() : CDAudio_Pause();
	if (c == i++) CDAudio_Stop();
#ifndef _arch_dreamcast
	if (c == i++) Cbuf_AddText ("cd eject\n");
	if (c == i++) Cbuf_AddText ("cd close\n");
	if (c == i++) Cbuf_AddText ("cd reset\n");
#endif
}
void M_Audio_Key (int k)
{
	if (m_inp_cancel)
		M_Options_f ();
	else if (m_inp_up)
	{
		S_LocalSound ("misc/menu1.wav");
		if (--m_cursor[m_state] < 0)
			m_cursor[m_state] = AUDIO_ITEMS-1;
	}
	else if (m_inp_down)
	{
		S_LocalSound ("misc/menu1.wav");
		if (++m_cursor[m_state] >= AUDIO_ITEMS)
			m_cursor[m_state] = 0;
	}
#ifndef _arch_dreamcast
#ifndef _WIN32
	else if (m_cursor[m_state] == 6 && m_inp_ok && cdValid && audioTrack[cd_cursor])
#else
	else if (m_cursor[m_state] == 5 && m_inp_ok && cdValid && audioTrack[cd_cursor])
#endif
#else
	else if (m_cursor[m_state] == 5 && m_inp_ok && cdValid && audioTrack[cd_cursor])
#endif
		CDAudio_Play(cd_cursor, playLooping);
	else if (m_inp_left)
		M_Audio_Change (-1);
	else if (m_inp_right || m_inp_ok)
		M_Audio_Change (1);
}

// mankrip - begin
//=============================================================================
/* DISPLAY MENU */

#ifdef _arch_dreamcast
#define	DISPLAY_ITEMS	11
#else
#define	DISPLAY_ITEMS	12
#endif

void M_Display_f (void)
{
	if (key_dest == key_menu && m_state == m_display)
	{
		M_Off ();
		return;
	}
	key_dest = key_menu;
	m_state = m_display;
	m_entersound = true;
}

void M_Display_Draw (void)
{
	int y = 20;

	M_DrawPlaque ("gfx/p_option.lmp", true);

#ifndef _arch_dreamcast
	M_Print (16, y+=8, "             Framerate"); M_DrawCheckbox (220, y, cl_showfps.value);
#endif
	M_Print (16, y+=8, "            Status bar"); M_DrawSlider (220, y, sbar.value / 4.0);
	M_Print (16, y+=8, "     Status Background"); M_DrawCheckbox (220, y, sbar_show_bg.value);
	M_Print (16, y+=8, "          Level status"); M_DrawCheckbox (220, y, sbar_show_scores.value);
	M_Print (16, y+=8, "           Weapon list"); M_DrawCheckbox (220, y, sbar_show_weaponlist.value);
	M_Print (16, y+=8, "             Ammo list"); M_DrawCheckbox (220, y, sbar_show_ammolist.value);
	M_Print (16, y+=8, "                  Ammo"); M_DrawCheckbox (220, y, sbar_show_ammo.value);
	M_Print (16, y+=8, "                  Keys"); M_DrawCheckbox (220, y, sbar_show_keys.value);
	M_Print (16, y+=8, "                 Runes"); M_DrawCheckbox (220, y, sbar_show_runes.value);
	M_Print (16, y+=8, "              Powerups"); M_DrawCheckbox (220, y, sbar_show_powerups.value);
	M_Print (16, y+=8, "                 Armor"); M_DrawCheckbox (220, y, sbar_show_armor.value);
	M_Print (16, y+=8, "                Health"); M_DrawCheckbox (220, y, sbar_show_health.value);

	M_DrawCursor (200, 28, m_cursor[m_state]);
}

void M_Display_Change (int dir)
{
	int c = m_cursor[m_state];
	int i = 0;

	S_LocalSound ("misc/menu3.wav");

#ifndef _arch_dreamcast
	if (c == i++) Cvar_SetValue ("cl_showfps", !cl_showfps.value);
#endif
	if (c == i++) ChangeCVar("sbar", sbar.value, dir, 0, 4, true);
	if (c == i++) Cvar_SetValue ("sbar_show_bg", !sbar_show_bg.value);
	if (c == i++) Cvar_SetValue ("sbar_show_scores", !sbar_show_scores.value);
	if (c == i++) Cvar_SetValue ("sbar_show_weaponlist", !sbar_show_weaponlist.value);
	if (c == i++) Cvar_SetValue ("sbar_show_ammolist", !sbar_show_ammolist.value);
	if (c == i++) Cvar_SetValue ("sbar_show_ammo", !sbar_show_ammo.value);
	if (c == i++) Cvar_SetValue ("sbar_show_keys", !sbar_show_keys.value);
	if (c == i++) Cvar_SetValue ("sbar_show_runes", !sbar_show_runes.value);
	if (c == i++) Cvar_SetValue ("sbar_show_powerups", !sbar_show_powerups.value);
	if (c == i++) Cvar_SetValue ("sbar_show_armor", !sbar_show_armor.value);
	if (c == i++) Cvar_SetValue ("sbar_show_health", !sbar_show_health.value);
}

void M_Display_Key (int k)
{
	if (m_inp_cancel)
		M_Options_f ();
	else if (m_inp_up)
	{
		S_LocalSound ("misc/menu1.wav");
		if (--m_cursor[m_state] < 0)
			m_cursor[m_state] = DISPLAY_ITEMS-1;
	}
	else if (m_inp_down)
	{
		S_LocalSound ("misc/menu1.wav");
		if (++m_cursor[m_state] >= DISPLAY_ITEMS)
			m_cursor[m_state] = 0;
	}
	else if (m_inp_left)
		M_Display_Change (-1);
	else if (m_inp_right || m_inp_ok)
		M_Display_Change (1);
}
// mankrip - end

//=============================================================================
/* VIDEO MENU */
// mankrip - begin
#ifdef	GLQUAKE
#define	VIDEO_ITEMS	18
#else

#ifndef _arch_dreamcast
#define	VIDEO_ITEMS	20
#else
#define	VIDEO_ITEMS	19
#endif

#endif

void M_Video_f (void)
{
	if (key_dest == key_menu && m_state == m_video)
	{
		M_Off ();
		return;
	}
	key_dest = key_menu;
	m_state = m_video;
	m_entersound = true;
}

char * m_blendmap[COLORBLENDING_MAPS] =
{
	"Weighted mean"
,	"Additive"
,	"Smooth"
};
// mankrip - end


void M_Video_Draw (void)
{
	// mankrip - begin
	int y = 20;

	M_DrawPlaque ("gfx/p_option.lmp", true);

//	M_Print (16, y+=8, "           Screen size"); M_DrawSlider (220, y, (scr_viewsize.value - 50) / 50);
#ifndef _arch_dreamcast
	M_Print (16, y+=8, "           Video modes    ...");
#endif
	M_Print (16, y+=8, "         Adjust screen    ...");
	M_Print (16, y+=8, "            Brightness"); M_DrawSlider (220, y, (1.0 - v_gamma.value) / 0.5);
#ifndef GLQUAKE
	M_Print (16, y+=8, "           Chiaroscuro"); M_DrawCheckbox (220, y, r_light_style.value);
#endif
	M_Print (16, y+=8, "           Water blend"); M_Print (220, y, m_blendmap[minmaxclamp (0, (int)r_water_blend_map.value, COLORBLENDING_MAPS-1)]);
	M_Print (16, y+=8, "         Water opacity"); M_DrawSlider (220, y, r_water_blend_alpha.value);
	M_Print (16, y+=8, "           Slime blend"); M_Print (220, y, m_blendmap[minmaxclamp (0, (int)r_slime_blend_map.value, COLORBLENDING_MAPS-1)]);
	M_Print (16, y+=8, "         Slime opacity"); M_DrawSlider (220, y, r_slime_blend_alpha.value);
	M_Print (16, y+=8, "            Lava blend"); M_Print (220, y, m_blendmap[minmaxclamp (0, (int)r_lava_blend_map.value, COLORBLENDING_MAPS-1)]);
	M_Print (16, y+=8, "          Lava opacity"); M_DrawSlider (220, y, r_lava_blend_alpha.value);
	M_Print (16, y+=8, "          Portal blend"); M_Print (220, y, m_blendmap[minmaxclamp (0, (int)r_portal_blend_map.value, COLORBLENDING_MAPS-1)]);
	M_Print (16, y+=8, "        Portal opacity"); M_DrawSlider (220, y, r_portal_blend_alpha.value);
	M_Print (16, y+=8, "              Sky mode"); M_Print (220, y, (!r_sky_blend_mode.value) ? "Stippled" : "Blended");
	M_Print (16, y+=8, "             Sky blend"); M_Print (220, y, m_blendmap[minmaxclamp (0, (int)r_sky_blend_map.value, COLORBLENDING_MAPS-1)]);
	M_Print (16, y+=8, "           Sky opacity"); M_DrawSlider (220, y, r_sky_blend_alpha.value);
	M_Print (16, y+=8, "        Lit BSP models"); M_DrawCheckbox (220, y, r_externbsp_lit.value);
	M_Print (16, y+=8, "      Additive sprites"); M_DrawCheckbox (220, y, r_sprite_addblend.value);
	M_Print (16, y+=8, "           Lit sprites"); M_DrawCheckbox (220, y, r_sprite_lit.value);
	M_Print (16, y+=8, "       Blend particles"); M_DrawCheckbox (220, y, r_particle_blend_mode.value);
	M_Print (16, y+=8, "         Lit particles"); M_DrawCheckbox (220, y, r_particle_lit.value);

	M_DrawCursor (200, 28, m_cursor[m_state]);
	// mankrip - end
}
// mankrip - begin
void M_Video_Change (int dir)
{
	int c = m_cursor[m_state];
#ifndef _arch_dreamcast
	int i = 2;
#else
	int i = 1;
#endif

	S_LocalSound ("misc/menu3.wav");

//	if (c == i++) ChangeCVar("viewsize", scr_viewsize.value, dir * 5, 50, 100, true);
	if (c == i++) ChangeCVar("gamma", v_gamma.value, dir * -0.05, 0.5, 1, true);
#ifndef GLQUAKE
	if (c == i++) Cvar_SetValue ("r_light_style", !r_light_style.value);
#endif
	if (c == i++) ChangeCVar("r_water_blend_map"	,  r_water_blend_map  .value, dir, 0, COLORBLENDING_MAPS-1, false);
	if (c == i++) ChangeCVar("r_water_blend_alpha"	,  r_water_blend_alpha.value, dir*0.34f, 0, 1, true);
	if (c == i++) ChangeCVar("r_slime_blend_map"	,  r_slime_blend_map  .value, dir, 0, COLORBLENDING_MAPS-1, false);
	if (c == i++) ChangeCVar("r_slime_blend_alpha"	,  r_slime_blend_alpha.value, dir*0.34f, 0, 1, true);
	if (c == i++) ChangeCVar("r_lava_blend_map"		,	r_lava_blend_map  .value, dir, 0, COLORBLENDING_MAPS-1, false);
	if (c == i++) ChangeCVar("r_lava_blend_alpha"	,	r_lava_blend_alpha.value, dir*0.34f, 0, 1, true);
	if (c == i++) ChangeCVar("r_portal_blend_map"	, r_portal_blend_map  .value, dir, 0, COLORBLENDING_MAPS-1, false);
	if (c == i++) ChangeCVar("r_portal_blend_alpha"	, r_portal_blend_alpha.value, dir*0.34f, 0, 1, true);
//	if (c == i++) ChangeCVar("r_sky_blend_mode"		,	 r_sky_blend_mode .value, dir, 0, 1, false);
	if (c == i++) Cvar_SetValue ("r_sky_blend_mode", !r_sky_blend_mode.value);
	if (c == i++) ChangeCVar("r_sky_blend_map"		,	 r_sky_blend_map  .value, dir, 0, COLORBLENDING_MAPS-1, false);
	if (c == i++) ChangeCVar("r_sky_blend_alpha"	,	 r_sky_blend_alpha.value, dir*0.34f, 0, 1, true);
	if (c == i++) Cvar_SetValue ("r_externbsp_lit", !r_externbsp_lit.value);
	if (c == i++) Cvar_SetValue ("r_sprite_addblend", !r_sprite_addblend.value);
	if (c == i++) Cvar_SetValue ("r_sprite_lit", !r_sprite_lit.value);
	if (c == i++) Cvar_SetValue ("r_particle_blend_mode", !r_particle_blend_mode.value);
	if (c == i++) Cvar_SetValue ("r_particle_lit", !r_particle_lit.value);
}
// mankrip - end
void M_Video_Key (int k) // int key Edited by mankrip
{
//	(*vid_menukeyfn) (key);
	// mankrip - begin
	if (m_inp_cancel)
		M_Options_f ();
	else if (m_inp_up)
	{
		S_LocalSound ("misc/menu1.wav");
		if (--m_cursor[m_state] < 0)
			m_cursor[m_state] = VIDEO_ITEMS-1;
	}
	else if (m_inp_down)
	{
		S_LocalSound ("misc/menu1.wav");
		if (++m_cursor[m_state] >= VIDEO_ITEMS)
			m_cursor[m_state] = 0;
	}
	else if (m_cursor[m_state] == 0 && m_inp_ok)
#ifndef _arch_dreamcast
		M_VideoModes_f ();
	else if (m_cursor[m_state] == 1 && m_inp_ok)
#endif
		M_VideoSize_f ();
	else if (m_inp_left)
		M_Video_Change (-1);
	else if (m_inp_right || m_inp_ok)
		M_Video_Change (1);
	// mankrip - end
}

// mankrip - begin
//=============================================================================
/* VIDEO SIZE MENU */
#define	VIDEOSIZE_ITEMS	2

byte adjusting;

void M_VideoSize_f (void)
{
	if (key_dest == key_menu && m_state == m_videosize)
	{
		M_Off ();
		return;
	}
	key_dest = key_menu;
	m_state = m_videosize;
	m_entersound = true;
	adjusting = 0;
}

void M_VideoSize_Draw (void)
{
	int y = 20;

	Draw_Fill (0, 0, vid.width, vid.height, 0);
	Draw_Fill (screen_left					, 0, 1, vid.height, 15);
	Draw_Fill (vid.width - screen_right - 1 , 0, 1, vid.height, 15);
	Draw_Fill (0, screen_top					, vid.width, 1, 15);
	Draw_Fill (0, vid.height - screen_bottom - 1, vid.width, 1, 15);

	M_Print (72, y+=8, "Adjust top / left");
	M_Print (72, y+=8, "Adjust bottom / right");

	if (adjusting == 1)
		M_PrintText (va("[ %2.1f %% ]\n\n[ %2.1f %% ]      %2.1f %%  \n\n%2.1f %%", scr_top.value, scr_left.value, scr_right.value, scr_bottom.value), 0, y+=64, 38, 3, ALIGN_CENTER);
	else if (adjusting == 2)
		M_PrintText (va("%2.1f %%\n\n  %2.1f %%      [ %2.1f %% ]\n\n[ %2.1f %% ]", scr_top.value, scr_left.value, scr_right.value, scr_bottom.value), 0, y+=64, 38, 3, ALIGN_CENTER);
	else
	{
		M_DrawCursor (56, 28, m_cursor[m_state]);
		M_PrintText (va("%2.1f %%\n\n%2.1f %%        %2.1f %%\n\n%2.1f %%", scr_top.value, scr_left.value, scr_right.value, scr_bottom.value), 0, y+=64, 38, 3, ALIGN_CENTER);
	}
}
void M_VideoSize_Key (int k)
{
	if (!adjusting)
	{
		if (m_inp_cancel)
			M_Video_f ();
		else if (m_inp_up)
		{
			S_LocalSound ("misc/menu1.wav");
			if (--m_cursor[m_state] < 0)
				m_cursor[m_state] = VIDEOSIZE_ITEMS-1;
		}
		else if (m_inp_down)
		{
			S_LocalSound ("misc/menu1.wav");
			if (++m_cursor[m_state] >= VIDEOSIZE_ITEMS)
				m_cursor[m_state] = 0;
		}
		else if (m_inp_ok)
		{
			S_LocalSound ("misc/menu3.wav");
			adjusting = m_cursor[m_state] + 1;
		}
	}
	else if (adjusting == 1) // top left
	{
		if (m_inp_up)
			ChangeCVar("scr_top", scr_top.value, -0.5, 0, 12, true);
		else if (m_inp_down)
			ChangeCVar("scr_top", scr_top.value, 0.5, 0, 12, true);
		else if (m_inp_left)
			ChangeCVar("scr_left", scr_left.value, -0.5, 0, 12, true);
		else if (m_inp_right)
			ChangeCVar("scr_left", scr_left.value, 0.5, 0, 12, true);
		else if (m_inp_cancel || m_inp_ok)
		{
			S_LocalSound ("misc/menu3.wav");
			adjusting = 0;
		}
	}
	else if (adjusting == 2) // bottom right
	{
		if (m_inp_up)
			ChangeCVar("scr_bottom", scr_bottom.value, 0.5, 0, 15, true);
		else if (m_inp_down)
			ChangeCVar("scr_bottom", scr_bottom.value, -0.5, 0, 15, true);
		else if (m_inp_left)
			ChangeCVar("scr_right", scr_right.value, 0.5, 0, 12, true);
		else if (m_inp_right)
			ChangeCVar("scr_right", scr_right.value, -0.5, 0, 12, true);
		else if (m_inp_cancel || m_inp_ok)
		{
			S_LocalSound ("misc/menu3.wav");
			adjusting = 0;
		}
	}
}
// mankrip - end

//=============================================================================
/* VIDEO MODES MENU */
// mankrip - re-added the video modes menu for the PC version
void M_VideoModes_f (void)
{
	key_dest = key_menu;
	m_state = m_videomodes;
	m_entersound = true;
}

// mankrip - begin
//=============================================================================
/* DEVELOPER MENU */
#ifndef GLQUAKE
#define	DEVELOPER_ITEMS	17
#else
#define	DEVELOPER_ITEMS	15
#endif

void M_Developer_f (void)
{
	if (key_dest == key_menu && m_state == m_developer)
	{
		M_Off ();
		return;
	}
	key_dest = key_menu;
	m_state = m_developer;
	m_entersound = true;
}


cvar_t	loadas8bit; // workaround for Dreamcast only =P
void M_Developer_Draw (void)
{
	int y = 20;

	M_DrawPlaque ("gfx/p_option.lmp", false);

	M_Print (16, y+=8, "        developer mode"); M_DrawCheckbox (220, y, developer.value);
	M_Print (16, y+=8, "       registered mode"); M_DrawCheckbox (220, y, registered.value);
	M_Print (16, y+=8, "        Enable console"); M_DrawCheckbox (220, y, con_enable.value);
	M_Print (16, y+=8, "          Mute console"); M_DrawCheckbox (220, y, con_mute.value);
	M_Print (16, y+=8, "  3D stereo separation"); M_DrawSlider (220, y, (r_stereo_separation.value + 10.0) / 20.0);
	M_Print (16, y+=8, "                   fog"); M_DrawSlider (220, y, (developer.value-100.0)/255.0);
	M_Print (16, y+=8, "         draw entities"); M_DrawCheckbox (220, y, r_drawentities.value);
	M_Print (16, y+=8, "           full bright"); M_DrawCheckbox (220, y, r_fullbright.value);
#ifndef GLQUAKE
	M_Print (16, y+=8, "underwater warp effect"); M_DrawCheckbox (220, y, r_waterwarp.value);
#endif
	M_Print (16, y+=8, "  screen color effects"); M_DrawCheckbox (220, y, gl_polyblend.value);
	M_Print (16, y+=8, "              Show FPS"); M_DrawCheckbox (220, y, cl_showfps.value);
	M_Print (16, y+=8, "              timedemo");
	M_Print (16, y+=8, "                   fly");
	M_Print (16, y+=8, "                noclip");
	M_Print (16, y+=8, "              notarget");
	M_Print (16, y+=8, "             impulse 9");
	M_Print (16, y+=8, "                   god");

	M_DrawCursor (200, 28, m_cursor[m_state]);
}
void M_Developer_Change (int dir)
{
	int c = m_cursor[m_state];
	int i = 0;

	S_LocalSound ("misc/menu3.wav");

	if (c == i++) Cvar_SetValue ("developer", !developer.value);
	if (c == i++) Cvar_SetValue ("registered", !registered.value);
	if (c == i++) Cvar_SetValue ("con_enable", !con_enable.value);
	if (c == i++) Cvar_SetValue ("con_mute", !con_mute.value);
	if (c == i++) ChangeCVar("r_stereo_separation", r_stereo_separation.value, dir*0.5, -10.0, 10.0, true);
	if (c == i++) ChangeCVar("developer", developer.value, dir, 100, 355, false);
	if (c == i++) Cvar_SetValue ("r_drawentities", !r_drawentities.value);
	if (c == i++) Cvar_SetValue ("r_fullbright", !r_fullbright.value);
#ifndef GLQUAKE
	if (c == i++) Cvar_SetValue ("r_waterwarp", !r_waterwarp.value);
#endif
	if (c == i++) Cvar_SetValue ("gl_polyblend", !gl_polyblend.value);
	if (c == i++) Cvar_SetValue ("cl_showfps", !cl_showfps.value);
	if (c == i++){M_Off(); Cbuf_AddText ("timedemo demo1.dem\n");}
	if (c == i++) Cbuf_AddText ("fly\n");
	if (c == i++) Cbuf_AddText ("noclip\n");
	if (c == i++) Cbuf_AddText ("notarget\n");
	if (c == i++){M_Off(); Cbuf_AddText ("impulse 9\n");}
	if (c == i++) Cbuf_AddText ("god\n");
}
void M_Developer_Key (int k)
{
	if (m_inp_cancel)
		M_Options_f ();
	else if (m_inp_up)
	{
		S_LocalSound ("misc/menu1.wav");
		if (--m_cursor[m_state] < 0)
			m_cursor[m_state] = DEVELOPER_ITEMS-1;
	}
	else if (m_inp_down)
	{
		S_LocalSound ("misc/menu1.wav");
		if (++m_cursor[m_state] >= DEVELOPER_ITEMS)
			m_cursor[m_state] = 0;
	}
	else if (m_inp_left)
		M_Developer_Change (-1);
	else if (m_inp_right || m_inp_ok)
		M_Developer_Change (1);
}
// mankrip - end
//=============================================================================
/* HELP MENU */
double m_credits_starttime;
void M_Credits_f (void)
{
	key_dest = key_menu;
	m_state = m_credits;
	m_cursor[m_state] = 1;//help_pages.value;
	m_credits_starttime = realtime - 5.0; // the first screen has less text to read, so let's cut the waiting in half
}

void M_Credits_Key (int key)
{
	if (m_cursor[m_state] == 1)
	{
		m_cursor[m_state]++;
		m_credits_starttime = realtime;
	}
	else if (m_cursor[m_state] == 2)
	{
		m_cursor[m_state]++;
		m_credits_starttime = realtime - 9.0; // 1 segundo de intervalo para exibir o console background antes de carregar o quake.rc
//		Cbuf_InsertText ("exec quake.rc\n"); // loading config files from devices such as the Dreamcast VMU is slow, so let's load them while the screen is displayed
	}
	else //if (m_cursor[m_state] == 3)
	{
		M_Off ();
		Cbuf_InsertText ("exec quake.rc\n");
	}
}

void M_Help_f (void)
{
	// mankrip - begin
	int i;
	if (key_dest == key_menu && m_state == m_help)
	{
		M_Off ();
		return;
	}
	for (i=0; i<help_pages.value; i++)
		Draw_CachePic(va("gfx/help%i.lmp", i));
	m_cursor[m_state] = 0;
	// mankrip - end
	key_dest = key_menu;
	m_state = m_help;
	m_entersound = true;
}

void M_Help_Draw (void)
{
	// mankrip - begin
	qpic_t	*p;
	int y = 0;

	if (m_state == m_credits)
	{
		if ((int)(realtime-m_credits_starttime) > 9)
			M_Credits_Key(0);
		if (m_cursor[m_state] == 1)
			goto credits1;
		else if (m_cursor[m_state] == 2)
			goto credits2;
		return;
	}

	if (m_cursor[m_state] < help_pages.value)
	{
		p = Draw_CachePic ( va("gfx/help%i.lmp", m_cursor[m_state]));
		M_DrawTransPic (0, 0, p);
	}
	else
	{
		if (m_cursor[m_state] == help_pages.value)
		{
credits1:
			if (m_state == m_credits)
			{
				// center the credits menu vertically
				y += 7*8;
				M_DrawTextBox (0, y, 38*8, 23*8 - 14*8);
			}
			else // only goes full screen if in help menu
				M_DrawTextBox (0, y, 38*8, 23*8);
			M_PrintWhite(16+4,	y+=12, "       Makaqu version 1.5.2        ");

			M_PrintWhite(16,	y+=16, "Programming");
			M_Print		(16,	 y+=8, " mankrip");
			M_Print		(16,	 y+=8, " mankrip@gmail.com");

			M_PrintWhite(16,	y+=16, "Thanks to everyone else who made GPL");
			M_PrintWhite(16,	 y+=8, "licensed code used in this engine.");
		}
		else
		{
credits2:
		//	if (m_state != m_credits)
				M_DrawTextBox (0, y, 38*8, 23*8);
			M_PrintWhite(16+4,	y+=12, "     Quake engine version 1.09     ");

			M_PrintWhite(16,	y+=16, "Programming");
			M_Print		(16,	 y+=8, " John Carmack");
			M_Print		(16,	 y+=8, " Michael Abrash");
			M_Print		(16,	 y+=8, " John Cash");
			M_Print		(16,	 y+=8, " Dave 'Zoid' Kirsch");

			M_PrintWhite(16,	y+=16, "Quake is a trademark of Id Software,");
			M_PrintWhite(16,	 y+=8, "inc., (c)1996-1997 Id Software, inc.");
			M_PrintWhite(16,	 y+=8, "All rights reserved.");

			M_PrintWhite(16,	y+=16, "This engine is distributed in the");
			M_PrintWhite(16,	 y+=8, "hope that it will be useful, but");
			M_PrintWhite(16,	 y+=8, "WITHOUT ANY WARRANTY.");
			M_PrintWhite(16,	 y+=8, "Its code is licensed under the terms");
			M_PrintWhite(16,	 y+=8, "of the GNU General Public License.");
			M_PrintWhite(16,	 y+=8, "If you distribute modified binary");
			M_PrintWhite(16,	 y+=8, "versions of this engine, you must");
			M_PrintWhite(16,	 y+=8, "also make its source code available.");
			M_PrintWhite(16,	 y+=8, "See the GNU General Public License");
			M_PrintWhite(16,	 y+=8, "for more details.");
		}
	}
	// mankrip - end
}

void M_Help_Key (int key)
{
	if (m_inp_cancel)
		M_Main_f ();
	else if (m_inp_down || m_inp_right)
	{
		S_LocalSound ("misc/menu1.wav");
		if (++m_cursor[m_state] >= (help_pages.value+2))
			m_cursor[m_state] = 0;
	}
	else if (m_inp_up || m_inp_left)
	{
		S_LocalSound ("misc/menu1.wav");
		if (--m_cursor[m_state] < 0)
			m_cursor[m_state] = help_pages.value+1;
	}

}

//=============================================================================
/* QUIT MENU */

char *quitMessage [] =
{
// mankrip - edited - begin
"Are you gonna quit\n\
this game just like\n\
everything else?",

"Milord, methinks that\n\
thou art a lowly\n\
quitter. Is this true?",

"Do I need to bust your\n\
face open for trying\n\
to quit?",

"Man, I oughta smack you\n\
for trying to quit!\n\
Press Y to get\n\
smacked out.",

"Press Y to quit like a\n\
big loser in life.\n\
Press N to stay proud\n\
and successful!",

"If you press Y to\n\
quit, I will summon\n\
Satan all over your\n\
hard drive!",

"Um, Asmodeus dislikes\n\
his children trying to\n\
quit. Press Y to return\n\
to your Tinkertoys.",

"If you quit now, I'll\n\
throw a blanket-party\n\
for you next time!"
// mankrip - edited - end
};

void M_Quit_f (void)
{
	M_PopUp_f (quitMessage[rand()&7], "quit"); // mankrip
}
// fazer menu c/ opções de reiniciar o mapa atual, resetar o jogo, resetar o console
// e sair pra BIOS
// usar arch_reboot pra resetar o DC
#if NET_MENUS // mankrip - removed multiplayer menus
//void _ (){}

//=============================================================================
/* MULTIPLAYER MENU */

#define	MULTIPLAYER_ITEMS	3
int m_multiplayer_cursor;

void M_MultiPlayer_f (void)
{
	key_dest = key_menu;
	m_state = m_multiplayer;
	m_entersound = true;
}

void M_MultiPlayer_Draw (void)
{
	M_DrawPlaque ("gfx/p_multi.lmp", true); // mankrip

	M_DrawTransPic (72, 28/*32*/, Draw_CachePic ("gfx/mp_menu.lmp") ); // mankrip - edited

	M_DrawTransPic (54, 28/*32*/ + m_multiplayer_cursor * 20,Draw_CachePic( va("gfx/menudot%i.lmp", 1+((int)(realtime * 10)%6) ) ) ); // mankrip - edited

	if (serialAvailable || ipxAvailable || tcpipAvailable)
		return;
	M_PrintWhite ((320/2) - ((27*8)/2), 148, "No Communications Available");
}

void M_MultiPlayer_Key (int key)
{
	// mankrip - begin
	if (m_inp_cancel)
		M_Main_f ();
	else if (m_inp_up)
	{
		S_LocalSound ("misc/menu1.wav");
		if (--m_multiplayer_cursor < 0)
			m_multiplayer_cursor = MULTIPLAYER_ITEMS - 1;
	}
	else if (m_inp_down)
	{
		S_LocalSound ("misc/menu1.wav");
		if (++m_multiplayer_cursor >= MULTIPLAYER_ITEMS)
			m_multiplayer_cursor = 0;
	}
	else if (m_inp_ok)
	// mankrip - end
	{
		m_entersound = true;
		switch (m_multiplayer_cursor)
		{
		case 0:
			if (serialAvailable || ipxAvailable || tcpipAvailable)
				M_Net_f ();
			break;
		case 1:
			if (serialAvailable || ipxAvailable || tcpipAvailable)
				M_Net_f ();
			break;
		case 2:
			M_Setup_f ();
			break;
		}
	}
}

//=============================================================================
/* NET MENU */

int	m_net_cursor;
int m_net_items;
int m_net_saveHeight;

char *net_helpMessage [] =
{
/* .........1.........2.... */
  "                        ",
  " Two computers connected",
  "   through two modems.  ",
  "                        ",

  "                        ",
  " Two computers connected",
  " by a null-modem cable. ",
  "                        ",

  " Novell network LANs    ",
  " or Windows 95 DOS-box. ",
  "                        ",
  "(LAN=Local Area Network)",

  " Commonly used to play  ",
  " over the Internet, but ",
  " also used on a Local   ",
  " Area Network.          "
};

void M_Net_f (void)
{
	key_dest = key_menu;
	m_state = m_net;
	m_entersound = true;
	m_net_items = 4;

	if (m_net_cursor >= m_net_items)
		m_net_cursor = 0;
	m_net_cursor--;
	M_Net_Key (K_DOWNARROW);
}


void M_Net_Draw (void)
{
	int		f;
	qpic_t	*p;

	M_DrawPlaque ("gfx/p_multi.lmp", true); // mankrip

	f = 28;//32; // mankrip

	if (serialAvailable)
	{
		p = Draw_CachePic ("gfx/netmen1.lmp");
	}
	else
	{
#ifndef _arch_dreamcast // BlackAura
#ifdef _WIN32
		p = NULL;
#else
		p = Draw_CachePic ("gfx/dim_modm.lmp");
#endif
#else // BlackAura
		p = Draw_CachePic ("gfx/dim_modm.lmp");
#endif // BlackAura
	}

	if (p)
		M_DrawTransPic (72, f, p);

	f += 19;

	if (serialAvailable)
	{
		p = Draw_CachePic ("gfx/netmen2.lmp");
	}
	else
	{
#ifndef _arch_dreamcast // BlackAura
#ifdef _WIN32
		p = NULL;
#else
		p = Draw_CachePic ("gfx/dim_drct.lmp");
#endif
#else // BlackAura
		p = Draw_CachePic ("gfx/dim_drct.lmp");
#endif // BlackAura
	}

	if (p)
		M_DrawTransPic (72, f, p);

	f += 19;
	if (ipxAvailable)
		p = Draw_CachePic ("gfx/netmen3.lmp");
	else
		p = Draw_CachePic ("gfx/dim_ipx.lmp");
	M_DrawTransPic (72, f, p);

	f += 19;
	if (tcpipAvailable)
		p = Draw_CachePic ("gfx/netmen4.lmp");
	else
		p = Draw_CachePic ("gfx/dim_tcp.lmp");
	M_DrawTransPic (72, f, p);

	if (m_net_items == 5)	// JDC, could just be removed
	{
		f += 19;
		p = Draw_CachePic ("gfx/netmen5.lmp");
		M_DrawTransPic (72, f, p);
	}

	f = (320-26*8)/2;
	M_DrawTextBox (f, /*130*/134, 24*8, 4*8);
	f += 8;
	M_Print (f, 142, net_helpMessage[m_net_cursor*4+0]);
	M_Print (f, 150, net_helpMessage[m_net_cursor*4+1]);
	M_Print (f, 158, net_helpMessage[m_net_cursor*4+2]);
	M_Print (f, 166, net_helpMessage[m_net_cursor*4+3]);

	M_DrawTransPic (54, 28/*32*/ + m_net_cursor * 20,Draw_CachePic( va("gfx/menudot%i.lmp", 1+((int)(realtime * 10)%6) ) ) ); // mankrip - edited
}


void M_Net_Key (int k)
{
	// mankrip - begin
	if (m_inp_cancel)
		M_MultiPlayer_f ();
	// mankrip - end
again:
	switch (k)
	{
	case K_DOWNARROW:
		S_LocalSound ("misc/menu1.wav");
		if (++m_net_cursor >= m_net_items)
			m_net_cursor = 0;
		break;

	case K_UPARROW:
		S_LocalSound ("misc/menu1.wav");
		if (--m_net_cursor < 0)
			m_net_cursor = m_net_items - 1;
		break;

	case K_ENTER:
		m_entersound = true;

		switch (m_net_cursor)
		{
		case 0:
			M_SerialConfig_f ();
			break;

		case 1:
			M_SerialConfig_f ();
			break;

		case 2:
			M_LanConfig_f ();
			break;

		case 3:
			M_LanConfig_f ();
			break;

		case 4:
// multiprotocol
			break;
		}
	}

	if (m_net_cursor == 0 && !serialAvailable)
		goto again;
	if (m_net_cursor == 1 && !serialAvailable)
		goto again;
	if (m_net_cursor == 2 && !ipxAvailable)
		goto again;
	if (m_net_cursor == 3 && !tcpipAvailable)
		goto again;
}

//=============================================================================

/* SERIAL CONFIG MENU */

int		serialConfig_cursor;
int		serialConfig_cursor_table[] = {48, 64, 80, 96, 112, 132};
#define	NUM_SERIALCONFIG_CMDS	6

static int ISA_uarts[]	= {0x3f8,0x2f8,0x3e8,0x2e8};
static int ISA_IRQs[]	= {4,3,4,3};
int serialConfig_baudrate[] = {9600,14400,19200,28800,38400,57600};

int		serialConfig_comport;
int		serialConfig_irq ;
int		serialConfig_baud;
char	serialConfig_phone[16];

void M_SerialConfig_f (void)
{
	int		n;
	int		port;
	int		baudrate;
	qboolean	useModem;

	key_dest = key_menu;
	m_state = m_serialconfig;
	m_entersound = true;
	if (JoiningGame && SerialConfig)
		serialConfig_cursor = 4;
	else
		serialConfig_cursor = 5;

	(*GetComPortConfig) (0, &port, &serialConfig_irq, &baudrate, &useModem);

	// map uart's port to COMx
	for (n = 0; n < 4; n++)
		if (ISA_uarts[n] == port)
			break;
	if (n == 4)
	{
		n = 0;
		serialConfig_irq = 4;
	}
	serialConfig_comport = n + 1;

	// map baudrate to index
	for (n = 0; n < 6; n++)
		if (serialConfig_baudrate[n] == baudrate)
			break;
	if (n == 6)
		n = 5;
	serialConfig_baud = n;

	m_return_onerror = false;
	m_return_reason[0] = 0;
}


void M_SerialConfig_Draw (void)
{
	qpic_t	*p;
	int		basex;
	char	*startJoin;
	char	*directModem;

	M_DrawTransPic (16, 4, Draw_CachePic ("gfx/qplaque.lmp") );
	p = Draw_CachePic ("gfx/p_multi.lmp");
	basex = (320-p->width)/2;
	M_DrawTransPic (basex, 4, p);

	if (StartingGame)
		startJoin = "New Game";
	else
		startJoin = "Join Game";
	if (SerialConfig)
		directModem = "Modem";
	else
		directModem = "Direct Connect";
	M_Print (basex, 32, va ("%s - %s", startJoin, directModem));
	basex += 8;

	M_Print (basex, serialConfig_cursor_table[0], "Port");
	M_DrawTextBox (160, 40, 4*8, 1*8);
	M_Print (168, serialConfig_cursor_table[0], va("COM%u", serialConfig_comport));

	M_Print (basex, serialConfig_cursor_table[1], "IRQ");
	M_DrawTextBox (160, serialConfig_cursor_table[1]-8, 1*8, 1*8);
	M_Print (168, serialConfig_cursor_table[1], va("%u", serialConfig_irq));

	M_Print (basex, serialConfig_cursor_table[2], "Baud");
	M_DrawTextBox (160, serialConfig_cursor_table[2]-8, 5*8, 1*8);
	M_Print (168, serialConfig_cursor_table[2], va("%u", serialConfig_baudrate[serialConfig_baud]));

	if (SerialConfig)
	{
		M_Print (basex, serialConfig_cursor_table[3], "Modem Setup...");
		if (JoiningGame)
		{
			M_Print (basex, serialConfig_cursor_table[4], "Phone number");
			M_DrawTextBox (160, serialConfig_cursor_table[4]-8, 16*8, 1*8);
			M_Print (168, serialConfig_cursor_table[4], serialConfig_phone);
		}
	}

	if (JoiningGame)
	{
		M_DrawTextBox (basex, serialConfig_cursor_table[5]-8, 7*8, 1*8);
		M_Print (basex+8, serialConfig_cursor_table[5], "Connect");
	}
	else
	{
		M_DrawTextBox (basex, serialConfig_cursor_table[5]-8, 2*8, 1*8);
		M_Print (basex+8, serialConfig_cursor_table[5], "OK");
	}

	M_DrawCharacter (basex-8, serialConfig_cursor_table [serialConfig_cursor], 12+((int)(realtime*4)&1));

	if (serialConfig_cursor == 4)
		M_DrawCharacter (168 + 8*strlen(serialConfig_phone), serialConfig_cursor_table [serialConfig_cursor], 10+((int)(realtime*4)&1));

	if (*m_return_reason)
		M_PrintWhite (basex, 148, m_return_reason);
}


void M_SerialConfig_Key (int key)
{
	int		l;

	// mankrip - begin
	if (m_inp_cancel)
		M_Net_f ();
	// mankrip - end
	switch (key)
	{
	case K_UPARROW:
		S_LocalSound ("misc/menu1.wav");
		serialConfig_cursor--;
		if (serialConfig_cursor < 0)
			serialConfig_cursor = NUM_SERIALCONFIG_CMDS-1;
		break;

	case K_DOWNARROW:
		S_LocalSound ("misc/menu1.wav");
		serialConfig_cursor++;
		if (serialConfig_cursor >= NUM_SERIALCONFIG_CMDS)
			serialConfig_cursor = 0;
		break;

	case K_LEFTARROW:
		if (serialConfig_cursor > 2)
			break;
		S_LocalSound ("misc/menu3.wav");

		if (serialConfig_cursor == 0)
		{
			serialConfig_comport--;
			if (serialConfig_comport == 0)
				serialConfig_comport = 4;
			serialConfig_irq = ISA_IRQs[serialConfig_comport-1];
		}

		if (serialConfig_cursor == 1)
		{
			serialConfig_irq--;
			if (serialConfig_irq == 6)
				serialConfig_irq = 5;
			if (serialConfig_irq == 1)
				serialConfig_irq = 7;
		}

		if (serialConfig_cursor == 2)
		{
			serialConfig_baud--;
			if (serialConfig_baud < 0)
				serialConfig_baud = 5;
		}

		break;

	case K_RIGHTARROW:
		if (serialConfig_cursor > 2)
			break;
forward:
		S_LocalSound ("misc/menu3.wav");

		if (serialConfig_cursor == 0)
		{
			serialConfig_comport++;
			if (serialConfig_comport > 4)
				serialConfig_comport = 1;
			serialConfig_irq = ISA_IRQs[serialConfig_comport-1];
		}

		if (serialConfig_cursor == 1)
		{
			serialConfig_irq++;
			if (serialConfig_irq == 6)
				serialConfig_irq = 7;
			if (serialConfig_irq == 8)
				serialConfig_irq = 2;
		}

		if (serialConfig_cursor == 2)
		{
			serialConfig_baud++;
			if (serialConfig_baud > 5)
				serialConfig_baud = 0;
		}

		break;

	case K_ENTER:
		if (serialConfig_cursor < 3)
			goto forward;

		m_entersound = true;

		if (serialConfig_cursor == 3)
		{
			(*SetComPortConfig) (0, ISA_uarts[serialConfig_comport-1], serialConfig_irq, serialConfig_baudrate[serialConfig_baud], SerialConfig);

			M_ModemConfig_f ();
			break;
		}

		if (serialConfig_cursor == 4)
		{
			serialConfig_cursor = 5;
			break;
		}

		// serialConfig_cursor == 5 (OK/CONNECT)
		(*SetComPortConfig) (0, ISA_uarts[serialConfig_comport-1], serialConfig_irq, serialConfig_baudrate[serialConfig_baud], SerialConfig);

		M_ConfigureNetSubsystem ();

		if (StartingGame)
		{
			M_GameOptions_f ();
			break;
		}

		m_return_state = m_state;
		m_return_onerror = true;

		// BlackAura - Menu background fading
		fade_level = 0;
		key_dest = key_game;
		m_state = m_none;

		if (SerialConfig)
			Cbuf_AddText (va ("connect \"%s\"\n", serialConfig_phone));
		else
			Cbuf_AddText ("connect\n");
		break;

	case K_BACKSPACE:
		if (serialConfig_cursor == 4)
		{
			if (strlen(serialConfig_phone))
				serialConfig_phone[strlen(serialConfig_phone)-1] = 0;
		}
		break;

	default:
		if (key < 32 || key > 127)
			break;
		if (serialConfig_cursor == 4)
		{
			l = strlen(serialConfig_phone);
			if (l < 15)
			{
				serialConfig_phone[l+1] = 0;
				serialConfig_phone[l] = key;
			}
		}
	}

	if (DirectConfig && (serialConfig_cursor == 3 || serialConfig_cursor == 4))
		if (key == K_UPARROW)
			serialConfig_cursor = 2;
		else
			serialConfig_cursor = 5;

	if (SerialConfig && StartingGame && serialConfig_cursor == 4)
		if (key == K_UPARROW)
			serialConfig_cursor = 3;
		else
			serialConfig_cursor = 5;
}

//=============================================================================
/* MODEM CONFIG MENU */

int		modemConfig_cursor;
int		modemConfig_cursor_table [] = {40, 56, 88, 120, 156};
#define NUM_MODEMCONFIG_CMDS	5

char	modemConfig_dialing;
char	modemConfig_clear [16];
char	modemConfig_init [32];
char	modemConfig_hangup [16];

void M_ModemConfig_f (void)
{
	key_dest = key_menu;
	m_state = m_modemconfig;
	m_entersound = true;
	(*GetModemConfig) (0, &modemConfig_dialing, modemConfig_clear, modemConfig_init, modemConfig_hangup);
}


void M_ModemConfig_Draw (void)
{
	qpic_t	*p;
	int		basex;

	M_DrawTransPic (16, 4, Draw_CachePic ("gfx/qplaque.lmp") );
	p = Draw_CachePic ("gfx/p_multi.lmp");
	basex = (320-p->width)/2;
	M_DrawTransPic (basex, 4, p);
	basex += 8;

	if (modemConfig_dialing == 'P')
		M_Print (basex, modemConfig_cursor_table[0], "Pulse Dialing");
	else
		M_Print (basex, modemConfig_cursor_table[0], "Touch Tone Dialing");

	M_Print (basex, modemConfig_cursor_table[1], "Clear");
	M_DrawTextBox (basex, modemConfig_cursor_table[1]+4, 16*8, 1*8);
	M_Print (basex+8, modemConfig_cursor_table[1]+12, modemConfig_clear);
	if (modemConfig_cursor == 1)
		M_DrawCharacter (basex+8 + 8*strlen(modemConfig_clear), modemConfig_cursor_table[1]+12, 10+((int)(realtime*4)&1));

	M_Print (basex, modemConfig_cursor_table[2], "Init");
	M_DrawTextBox (basex, modemConfig_cursor_table[2]+4, 30*8, 1*8);
	M_Print (basex+8, modemConfig_cursor_table[2]+12, modemConfig_init);
	if (modemConfig_cursor == 2)
		M_DrawCharacter (basex+8 + 8*strlen(modemConfig_init), modemConfig_cursor_table[2]+12, 10+((int)(realtime*4)&1));

	M_Print (basex, modemConfig_cursor_table[3], "Hangup");
	M_DrawTextBox (basex, modemConfig_cursor_table[3]+4, 16*8, 1*8);
	M_Print (basex+8, modemConfig_cursor_table[3]+12, modemConfig_hangup);
	if (modemConfig_cursor == 3)
		M_DrawCharacter (basex+8 + 8*strlen(modemConfig_hangup), modemConfig_cursor_table[3]+12, 10+((int)(realtime*4)&1));

	M_DrawTextBox (basex, modemConfig_cursor_table[4]-8, 2*8, 1*8);
	M_Print (basex+8, modemConfig_cursor_table[4], "OK");

	M_DrawCharacter (basex-8, modemConfig_cursor_table [modemConfig_cursor], 12+((int)(realtime*4)&1));
}


void M_ModemConfig_Key (int key)
{
	int		l;

	// mankrip - begin
	if (m_inp_cancel)
		M_SerialConfig_f ();
	// mankrip - end
	switch (key)
	{
	case K_UPARROW:
		S_LocalSound ("misc/menu1.wav");
		modemConfig_cursor--;
		if (modemConfig_cursor < 0)
			modemConfig_cursor = NUM_MODEMCONFIG_CMDS-1;
		break;

	case K_DOWNARROW:
		S_LocalSound ("misc/menu1.wav");
		modemConfig_cursor++;
		if (modemConfig_cursor >= NUM_MODEMCONFIG_CMDS)
			modemConfig_cursor = 0;
		break;

	case K_LEFTARROW:
	case K_RIGHTARROW:
		if (modemConfig_cursor == 0)
		{
			if (modemConfig_dialing == 'P')
				modemConfig_dialing = 'T';
			else
				modemConfig_dialing = 'P';
			S_LocalSound ("misc/menu1.wav");
		}
		break;

	case K_ENTER:
		if (modemConfig_cursor == 0)
		{
			if (modemConfig_dialing == 'P')
				modemConfig_dialing = 'T';
			else
				modemConfig_dialing = 'P';
			m_entersound = true;
		}

		if (modemConfig_cursor == 4)
		{
			(*SetModemConfig) (0, va ("%c", modemConfig_dialing), modemConfig_clear, modemConfig_init, modemConfig_hangup);
			m_entersound = true;
			M_SerialConfig_f ();
		}
		break;

	case K_BACKSPACE:
		if (modemConfig_cursor == 1)
		{
			if (strlen(modemConfig_clear))
				modemConfig_clear[strlen(modemConfig_clear)-1] = 0;
		}

		if (modemConfig_cursor == 2)
		{
			if (strlen(modemConfig_init))
				modemConfig_init[strlen(modemConfig_init)-1] = 0;
		}

		if (modemConfig_cursor == 3)
		{
			if (strlen(modemConfig_hangup))
				modemConfig_hangup[strlen(modemConfig_hangup)-1] = 0;
		}
		break;

	default:
		if (key < 32 || key > 127)
			break;

		if (modemConfig_cursor == 1)
		{
			l = strlen(modemConfig_clear);
			if (l < 15)
			{
				modemConfig_clear[l+1] = 0;
				modemConfig_clear[l] = key;
			}
		}

		if (modemConfig_cursor == 2)
		{
			l = strlen(modemConfig_init);
			if (l < 29)
			{
				modemConfig_init[l+1] = 0;
				modemConfig_init[l] = key;
			}
		}

		if (modemConfig_cursor == 3)
		{
			l = strlen(modemConfig_hangup);
			if (l < 15)
			{
				modemConfig_hangup[l+1] = 0;
				modemConfig_hangup[l] = key;
			}
		}
	}
}

//=============================================================================
/* LAN CONFIG MENU */

int		lanConfig_cursor = -1;
//int		lanConfig_cursor_table [] = {72, 92, 124};
int		lanConfig_cursor_table [] = {40, 60, 92};
#define NUM_LANCONFIG_CMDS	3

int 	lanConfig_port;
char	lanConfig_portname[6];
char	lanConfig_joinname[22];

void M_LanConfig_f (void)
{
	key_dest = key_menu;
	m_state = m_lanconfig;
	m_entersound = true;
	if (lanConfig_cursor == -1)
	{
		if (JoiningGame && TCPIPConfig)
			lanConfig_cursor = 2;
		else
			lanConfig_cursor = 1;
	}
	if (StartingGame && lanConfig_cursor == 2)
		lanConfig_cursor = 1;
	lanConfig_port = DEFAULTnet_hostport;
	sprintf(lanConfig_portname, "%u", lanConfig_port);

	m_return_onerror = false;
	m_return_reason[0] = 0;
}


void M_LanConfig_Draw (void)
{
	qpic_t	*p;
	int		basex;
	char	*startJoin;
	char	*protocol;

	int		f = 28; // mankrip
	M_DrawPlaque ("gfx/p_multi.lmp", true); // mankrip

//	M_DrawTransPic (16, 4, Draw_CachePic ("gfx/qplaque.lmp") );
	p = Draw_CachePic ("gfx/p_multi.lmp");
	basex = (320-p->width)/2;
//	M_DrawTransPic (basex, 4, p);

	if (StartingGame)
		startJoin = "New Game";
	else
		startJoin = "Join Game";
	if (IPXConfig)
		protocol = "IPX";
	else
		protocol = "TCP/IP";
	M_Print (basex, f/*32*/, va ("%s - %s", startJoin, protocol));
	basex += 8;

	M_Print (basex, f+20/*52*/, "Address:");
	if (IPXConfig)
		M_Print (basex+9*8, f+20/*52*/, my_ipx_address);
	else
		M_Print (basex+9*8, f+20/*52*/, my_tcpip_address);

	M_Print (basex, f+lanConfig_cursor_table[0], "Port");
	M_DrawTextBox (basex+8*8, f+lanConfig_cursor_table[0]-8, 6*8, 1*8);
	M_Print (basex+9*8, f+lanConfig_cursor_table[0], lanConfig_portname);

	if (JoiningGame)
	{
		M_Print (basex, f+lanConfig_cursor_table[1], "Search for local games...");
		M_Print (basex, f+76/*108*/, "Join game at:");
		M_DrawTextBox (basex+8, f+lanConfig_cursor_table[2]-8, 22*8, 1*8);
		M_Print (basex+16, f+lanConfig_cursor_table[2], lanConfig_joinname);
	}
	else
	{
		M_DrawTextBox (basex, f+lanConfig_cursor_table[1]-8, 2*8, 1*8);
		M_Print (basex+8, f+lanConfig_cursor_table[1], "OK");
	}

	M_DrawCharacter (basex-8, f+lanConfig_cursor_table [lanConfig_cursor], 12+((int)(realtime*4)&1));

	if (lanConfig_cursor == 0)
		M_DrawCharacter (basex+9*8 + 8*strlen(lanConfig_portname), f+lanConfig_cursor_table [0], 10+((int)(realtime*4)&1));

	if (lanConfig_cursor == 2)
		M_DrawCharacter (basex+16 + 8*strlen(lanConfig_joinname), f+lanConfig_cursor_table [2], 10+((int)(realtime*4)&1));

	if (*m_return_reason)
		M_PrintWhite (basex, f+116/*148*/, m_return_reason);
}


void M_LanConfig_Key (int key)
{
	int		l;

	// mankrip - begin
	if (m_inp_cancel)
		M_Net_f ();
	// mankrip - end
	switch (key)
	{
	case K_UPARROW:
		S_LocalSound ("misc/menu1.wav");
		lanConfig_cursor--;
		if (lanConfig_cursor < 0)
			lanConfig_cursor = NUM_LANCONFIG_CMDS-1;
		break;

	case K_DOWNARROW:
		S_LocalSound ("misc/menu1.wav");
		lanConfig_cursor++;
		if (lanConfig_cursor >= NUM_LANCONFIG_CMDS)
			lanConfig_cursor = 0;
		break;

	case K_ENTER:
		if (lanConfig_cursor == 0)
			break;

		m_entersound = true;

		M_ConfigureNetSubsystem ();

		if (lanConfig_cursor == 1)
		{
			if (StartingGame)
			{
				M_GameOptions_f ();
				break;
			}
			M_Search_f();
			break;
		}

		if (lanConfig_cursor == 2)
		{
			m_return_state = m_state;
			m_return_onerror = true;
			// BlackAura - Menu background fading
			fade_level = 0;
			key_dest = key_game;
			m_state = m_none;
			Cbuf_AddText ( va ("connect \"%s\"\n", lanConfig_joinname) );
			break;
		}

		break;

	case K_BACKSPACE:
		if (lanConfig_cursor == 0)
		{
			if (strlen(lanConfig_portname))
				lanConfig_portname[strlen(lanConfig_portname)-1] = 0;
		}

		if (lanConfig_cursor == 2)
		{
			if (strlen(lanConfig_joinname))
				lanConfig_joinname[strlen(lanConfig_joinname)-1] = 0;
		}
		break;

	default:
		if (key < 32 || key > 127)
			break;

		if (lanConfig_cursor == 2)
		{
			l = strlen(lanConfig_joinname);
			if (l < 21)
			{
				lanConfig_joinname[l+1] = 0;
				lanConfig_joinname[l] = key;
			}
		}

		if (key < '0' || key > '9')
			break;
		if (lanConfig_cursor == 0)
		{
			l = strlen(lanConfig_portname);
			if (l < 5)
			{
				lanConfig_portname[l+1] = 0;
				lanConfig_portname[l] = key;
			}
		}
	}

	if (StartingGame && lanConfig_cursor == 2)
		if (key == K_UPARROW)
			lanConfig_cursor = 1;
		else
			lanConfig_cursor = 0;

	l =  Q_atoi(lanConfig_portname);
	if (l > 65535)
		l = lanConfig_port;
	else
		lanConfig_port = l;
	sprintf(lanConfig_portname, "%u", lanConfig_port);
}

//=============================================================================
/* SEARCH MENU */

qboolean	searchComplete = false;
double		searchCompleteTime;

void M_Search_f (void)
{
	key_dest = key_menu;
	m_state = m_search;
	m_entersound = false;
	slistSilent = true;
	slistLocal = false;
	searchComplete = false;
	NET_Slist_f();

}


void M_Search_Draw (void)
{
	int x;

	M_DrawPlaque ("gfx/p_multi.lmp", false); // mankrip

	x = (320/2) - ((12*8)/2) + 4;
	M_DrawTextBox (x-8, 32, 12*8, 1*8);
	M_Print (x, 40, "Searching...");

	if(slistInProgress)
	{
		NET_Poll();
		return;
	}

	if (! searchComplete)
	{
		searchComplete = true;
		searchCompleteTime = realtime;
	}

	if (hostCacheCount)
	{
		M_ServerList_f ();
		return;
	}

	M_PrintWhite ((320/2) - ((22*8)/2), 64, "No Quake servers found");
	if ((realtime - searchCompleteTime) < 3.0)
		return;

	M_LanConfig_f ();
}


void M_Search_Key (int key)
{
}

//=============================================================================
/* SLIST MENU */

int		slist_cursor;
qboolean slist_sorted;

void M_ServerList_f (void)
{
	key_dest = key_menu;
	m_state = m_slist;
	m_entersound = true;
	slist_cursor = 0;
	m_return_onerror = false;
	m_return_reason[0] = 0;
	slist_sorted = false;
}


void M_ServerList_Draw (void)
{
	int		n;
	char	string [64];

	M_DrawPlaque ("gfx/p_multi.lmp", false); // mankrip

	if (!slist_sorted)
	{
		if (hostCacheCount > 1)
		{
			int	i,j;
			hostcache_t temp;
			for (i = 0; i < hostCacheCount; i++)
				for (j = i+1; j < hostCacheCount; j++)
					if (strcmp(hostcache[j].name, hostcache[i].name) < 0)
					{
						Q_memcpy(&temp, &hostcache[j], sizeof(hostcache_t));
						Q_memcpy(&hostcache[j], &hostcache[i], sizeof(hostcache_t));
						Q_memcpy(&hostcache[i], &temp, sizeof(hostcache_t));
					}
		}
		slist_sorted = true;
	}

	for (n = 0; n < hostCacheCount; n++)
	{
		if (hostcache[n].maxusers)
			sprintf(string, "%-15.15s %-15.15s %2u/%2u\n", hostcache[n].name, hostcache[n].map, hostcache[n].users, hostcache[n].maxusers);
		else
			sprintf(string, "%-15.15s %-15.15s\n", hostcache[n].name, hostcache[n].map);
		M_Print (16, 32 + 8*n, string);
	}
	M_DrawCharacter (0, 32 + slist_cursor*8, 12+((int)(realtime*4)&1));

	if (*m_return_reason)
		M_PrintWhite (16, 148, m_return_reason);
}


void M_ServerList_Key (int k)
{
	// mankrip - begin
	if (m_inp_cancel)
		M_LanConfig_f ();
	// mankrip - end
	switch (k)
	{
	case K_SPACE:
		M_Search_f ();
		break;

	case K_UPARROW:
	case K_LEFTARROW:
		S_LocalSound ("misc/menu1.wav");
		slist_cursor--;
		if (slist_cursor < 0)
			slist_cursor = hostCacheCount - 1;
		break;

	case K_DOWNARROW:
	case K_RIGHTARROW:
		S_LocalSound ("misc/menu1.wav");
		slist_cursor++;
		if (slist_cursor >= hostCacheCount)
			slist_cursor = 0;
		break;

	case K_ENTER:
		S_LocalSound ("misc/menu2.wav");
		m_return_state = m_state;
		m_return_onerror = true;
		slist_sorted = false;
		// BlackAura - Menu background fading
		fade_level = 0;
		key_dest = key_game;
		m_state = m_none;
		Cbuf_AddText ( va ("connect \"%s\"\n", hostcache[slist_cursor].cname) );
		break;

	default:
		break;
	}

}

void M_ConfigureNetSubsystem(void)
{
// enable/disable net systems to match desired config

	Cbuf_AddText ("stopdemo\n");
	if (SerialConfig || DirectConfig)
	{
		Cbuf_AddText ("com1 enable\n");
	}

	if (IPXConfig || TCPIPConfig)
		net_hostport = lanConfig_port;
}
#endif

// mankrip - begin
//=============================================================================
/* CONSOLE MENU */

/*
features:
-	syntax highlighting?
-	as telas de typing, autocomplete, OSK e ao menos a última linha de comando devem ser exibidas ao mesmo tempo
modos do console:
-	scrolling/leitura
-	typing
-	autocomplete selection (autocomplete menu?)
	-	exibir valores atuais das cvars, e formatação de parâmetros dos comandos
	-	auto-completar parâmetros
	-	ignorar parâmetros entre parênteses?
	-	puxar a função do autocomplete a partir do struct da cvar/comando
*/
#if 0
int		setup_cursor_table[] = {8, 32, 56, 88, 96};

char	setup_myname[16];

#define	NUM_SETUP_CMDS	1

void M_Console_f (void)
{
	key_dest = key_menu;
	m_state = m_console;
	m_entersound = true;
	Q_strcpy(setup_myname, cl_name.string);
	M_OnScreenKeyboard_Reset();
	M_Precache();
}

void M_Console_Draw (void)
{
	int i=0, i1, i2;
	int y = 28 - 4;

	M_Print (64, y + setup_cursor_table[i], "Your name");
	M_DrawTextBox (160, y + setup_cursor_table[i] - 8, 16*8, 1*8);
	M_Print (168, y + setup_cursor_table[i++], setup_myname);

	M_DrawCursor (56, y + setup_cursor_table [m_cursor[m_state]], 0);

	if (m_cursor[m_state] == 0)
		M_DrawCharacter (168 + 8*strlen(setup_myname  ), y + setup_cursor_table [m_cursor[m_state]], 10+((int)(realtime*4)&1));

	M_OnScreenKeyboard_Draw (setup_cursor_table [m_cursor[m_state]] + 12+20/*28 + 16*/);
}


void M_Console_Key (int k)
{
	int			l;

	if (m_cursor[m_state] == 0)
		k = M_OnScreenKeyboard_Key (k);

	if (m_inp_off)
		m_inp_cancel = true;

	if (m_inp_cancel)
	{
		// update setup on exit
		if (Q_strcmp(cl_name.string, setup_myname) != 0)
			Cbuf_AddText ( va ("name \"%s\"\n", setup_myname) );
		if (m_inp_off)
			M_Off ();
		else
			M_Options_f ();
	}
	else if (m_inp_up)
	{
		S_LocalSound ("misc/menu1.wav");
		if (--m_cursor[m_state] < 0)
			m_cursor[m_state] = NUM_SETUP_CMDS-1;
	}
	else if (m_inp_down)
	{
		S_LocalSound ("misc/menu1.wav");
		if (++m_cursor[m_state] >= NUM_SETUP_CMDS)
			m_cursor[m_state] = 0;
	}
	else if (m_inp_left)
	{
		if (m_cursor[m_state] < NUM_SETUP_CMDS-4)
			return;
		S_LocalSound ("misc/menu3.wav");
		if (m_cursor[m_state] == NUM_SETUP_CMDS-4)
			;
	}
	else if (m_inp_right)
	{
		if (m_cursor[m_state] < NUM_SETUP_CMDS-4)
			return;
forward:
		S_LocalSound ("misc/menu3.wav");
		if (m_cursor[m_state] == NUM_SETUP_CMDS-4)
			;
	}
	else if (m_inp_ok)
	{
		if (m_cursor[m_state] == 0)
			return;
		if (m_cursor[m_state] > 0 && m_cursor[m_state] < NUM_SETUP_CMDS)
			goto forward;
	}
	else if (k == K_BACKSPACE)
	{
		if (m_cursor[m_state] == 0)
		{
			if (strlen(setup_myname))
				setup_myname[strlen(setup_myname)-1] = 0;
		}
	}
	else if (k > 31 && k < 128)
	{
		if (m_cursor[m_state] == 0)
		{
			l = strlen(setup_myname);
			if (l < 15)
			{
				setup_myname[l+1] = 0;
				setup_myname[l] = k;
			}
		}
	}
	if (m_cursor[m_state] != 0)
		M_OnScreenKeyboard_Reset();
}
#endif
// mankrip - end


//=============================================================================
/* Menu Subsystem */

// BlackAura (28-12-2002) - Fade menu background
void M_FadeBackground()
{
	fade_level += 1; // mankrip - edited
	if(fade_level > 20)
		fade_level = 20;
	Draw_FadeScreen (/*fade_level*/);
}

void M_Draw (void)
{
	scr_copyeverything = 1;

	if (m_state == m_none || key_dest != key_menu)
	{
		#if 0
		// BlackAura (28-12-2002) - Fade menu background
		if(fade_level > 0)
		{
			scr_copyeverything = 1;
			fade_level -= 1; // mankrip - edited
			if(fade_level < 0)
				fade_level = 0;
			scr_fullupdate = 1;
			Draw_FadeScreen (fade_level);
		}
		#endif
		return;
	}

	if (!m_recursiveDraw)
	{
		scr_copyeverything = 1;

		if (scr_con_current)
		{
			Draw_ConsoleBackground (vid.height);
			VID_UnlockBuffer ();
			S_ExtraUpdate ();
			VID_LockBuffer ();
		}
		else
			// BlackAura (28-12-2002) - Fade menu background
			M_FadeBackground();

		scr_fullupdate = 0;
	}
	else
	{
		m_recursiveDraw = false;
	}
	Draw_UpdateAlignment (1, 1); // mankrip

	switch (m_state)
	{
	case m_none:
		break;

	case m_main:
		M_Main_Draw ();
		break;

	case m_fightoon:
		M_Fightoon_Draw ();
		break;

	case m_singleplayer:
		M_SinglePlayer_Draw ();
		break;

	case m_loadsmall: // mankrip - small saves
	case m_savesmall: // mankrip - small saves
	case m_load:
	case m_save:
		M_Save_Draw ();
		break;

	case m_gameoptions:
		M_GameOptions_Draw ();
		break;

	case m_options:
		M_Options_Draw ();
		break;

	case m_setup:
		M_Setup_Draw ();
		break;

	case m_keys:
		M_Keys_Draw ();
		break;

// mankrip - begin
	// mankrip - VMU saves - begin
#ifdef _arch_dreamcast
	case m_vmu:
		M_SelectVMU_Draw ();
		break;
#endif
	// mankrip - VMU saves - end

	case m_controller:
		M_Controller_Draw ();
		break;

	case m_mouse:
		M_Mouse_Draw ();
		break;

	case m_gameplay:
		M_Gameplay_Draw ();
		break;

	case m_audio:
		M_Audio_Draw ();
		break;

	case m_developer:
		M_Developer_Draw ();
		break;

	case m_popup:
		M_PopUp_Draw ();
		break;

	case m_display:
		M_Display_Draw ();
		break;

	case m_videomodes:
		(*vid_menudrawfn) ();
		break;

	case m_videosize:
		M_VideoSize_Draw ();
		break;
// mankrip - end

	case m_video:
		M_Video_Draw ();
		break;

	case m_credits: // mankrip
	case m_help:
		M_Help_Draw ();
		break;
#if NET_MENUS // mankrip - removed multiplayer menus
	case m_multiplayer:
		M_MultiPlayer_Draw ();
		break;

	case m_net:
		M_Net_Draw ();
		break;

	case m_serialconfig:
		M_SerialConfig_Draw ();
		break;

	case m_modemconfig:
		M_ModemConfig_Draw ();
		break;

	case m_lanconfig:
		M_LanConfig_Draw ();
		break;

	case m_search:
		M_Search_Draw ();
		break;

	case m_slist:
		M_ServerList_Draw ();
		break;
#endif
	default:
		break;
	}
	if (m_entersound)
	{
		S_LocalSound ("misc/menu2.wav");
		m_entersound = false;
	}

	VID_UnlockBuffer ();
	S_ExtraUpdate ();
	VID_LockBuffer ();
}


void M_Keydown (int key)
{
	M_CheckInput (key); // sets input
	// mankrip - menus which aren't turned off by m_inp_off
	switch (m_state)
	{
	// mankrip - begin
	case m_popup: // must be verified before all other menus!
		M_PopUp_Key (key);
		return;

	case m_credits:
//		if (m_cursor[m_state] != 3) // ignore input while loading quake.rc
			M_Credits_Key (key);
		return;
	// mankrip - end

	case m_setup:
		M_Setup_Key (key);
		return;

	case m_keys:
		M_Keys_Key (key);
		return;
#if NET_MENUS // mankrip - removed multiplayer menus
	case m_search:
		M_Search_Key (key);
		return;
#endif
	default:
		break;
	}

	// mankrip - begin
	if (m_inp_off)
	{
		M_Off ();
		return;
	}
#ifdef _WIN32
	if (key == '-')
	{
		Cbuf_AddText ("vid_minimize\n");
		return;
	}
#endif
	// mankrip - end

	switch (m_state)
	{
	case m_none:
		return;

	case m_main:
		M_Main_Key (key);
		return;

	case m_fightoon:
		M_Fightoon_Key (key);
		return;

	case m_singleplayer:
		M_SinglePlayer_Key (key);
		return;

	case m_loadsmall: // mankrip - small saves
	case m_savesmall: // mankrip - small saves
	case m_load:
	case m_save:
		M_Save_Key (key);
		return;

	case m_options:
		M_Options_Key (key);
		return;

// mankrip - begin
	// mankrip - VMU saves - begin
#ifdef _arch_dreamcast
	case m_vmu:
	M_SelectVMU_Key (key);
		return;
#endif
	// mankrip - VMU saves - end

	case m_controller:
		M_Controller_Key (key);
		return;

	case m_mouse:
		M_Mouse_Key (key);
		return;

	case m_gameplay:
		M_Gameplay_Key (key);
		return;

	case m_audio:
		M_Audio_Key (key);
		return;

	case m_developer:
		M_Developer_Key (key);
		return;

	case m_display:
		M_Display_Key (key);
		return;

	case m_videomodes:
		(*vid_menukeyfn) (key);
		return;

	case m_videosize:
		M_VideoSize_Key (key);
		return;
// mankrip - end

	case m_video:
		M_Video_Key (key);
		return;

	case m_help:
		M_Help_Key (key);
		return;

	case m_gameoptions:
		M_GameOptions_Key (key);
		return;
#if NET_MENUS // mankrip - removed multiplayer menus
	case m_multiplayer:
		M_MultiPlayer_Key (key);
		return;

	case m_net:
		M_Net_Key (key);
		return;

	case m_serialconfig:
		M_SerialConfig_Key (key);
		return;

	case m_modemconfig:
		M_ModemConfig_Key (key);
		return;

	case m_lanconfig:
		M_LanConfig_Key (key);
		return;

	case m_slist:
		M_ServerList_Key (key);
		return;
#endif
	default:
		break;
	}
}


void M_Init (void)
{
	// mankrip - setting default menu items - begin
	int i;

	M_Precache(); // mankrip - precaching menu pics
	M_Keys_SetDefaultCmds();
	if (hipnotic)
		M_SetDefaultEpisodes(hipnoticepisodes, 5, hipnoticlevels, 17);
	else if (rogue)
		M_SetDefaultEpisodes(rogueepisodes, 3, roguelevels, 16);
	else
	{
		if (registered.value)
			M_SetDefaultEpisodes(id1episodes, 6, id1levels, 37);
		else
			M_SetDefaultEpisodes(id1episodes, 1, id1levels, 8);
	}
	// mankrip - setting default menu items - end

	// mankrip - registering stuff - begin
	Cvar_RegisterVariable (&help_pages);
	Cvar_RegisterVariableWithCallback (&savename, SetSavename);
#ifdef _arch_dreamcast // mankrip
	if (Q_strcmp("id1", COM_SkipPath(com_gamedir)))
		Cvar_Set("savename", COM_SkipPath(com_gamedir));
	Cvar_RegisterVariableWithCallback (&vmupath, SetVMUpath);
	VMU_FindConfig();
#endif
	for (i=0 ; i<MAX_SAVEGAMES ; i++)
		m_fileinfo[i] = NULL;

	dont_bind['~'] = true;
	dont_bind['`'] = true;
	Cmd_AddCommand ("menu_keys_clear", M_Keys_Clear);
	Cmd_AddCommand ("menu_keys_addcmd", M_Keys_AddCmd);
	Cmd_AddCommand ("bindable", M_Keys_Bindable);
	Cmd_AddCommand ("menu_clearmaps", M_Maps_Clear);
	Cmd_AddCommand ("menu_addmap", M_AddMap);
	Cmd_AddCommand ("menu_addepisode", M_AddEpisode);
	// mankrip - VMU saves - begin
#ifdef _arch_dreamcast
	Cmd_AddCommand ("menu_loadsmall", M_VMUloadsmall_f);
	Cmd_AddCommand ("menu_savesmall", M_VMUsavesmall_f);
#else
	// mankrip - VMU saves - end
	Cmd_AddCommand ("menu_loadsmall", M_LoadSmall_f);
	Cmd_AddCommand ("menu_savesmall", M_SaveSmall_f);
#endif		// mankrip - VMU saves
	// mankrip - registering stuff - end

	Cmd_AddCommand ("togglemenu", M_ToggleMenu_f);

	Cmd_AddCommand ("menu_fightoon", M_Fightoon_f); // mankrip - Fightoon
	Cmd_AddCommand ("menu_main", M_Main_f);
	Cmd_AddCommand ("menu_singleplayer", M_SinglePlayer_f);
	// mankrip - VMU saves - begin
#ifdef _arch_dreamcast
	Cmd_AddCommand ("menu_load", M_VMUload_f);
	Cmd_AddCommand ("menu_save", M_VMUsave_f);
#else
	// mankrip - VMU saves - end
	Cmd_AddCommand ("menu_load", M_Load_f);
	Cmd_AddCommand ("menu_save", M_Save_f);
#endif		// mankrip - VMU saves
	Cmd_AddCommand ("menu_multiplayer", M_GameOptions_f); // edited by mankrip
	Cmd_AddCommand ("menu_setup", M_Setup_f);
	Cmd_AddCommand ("menu_options", M_Options_f);
	Cmd_AddCommand ("menu_keys", M_Keys_f);
	Cmd_AddCommand ("menu_video", M_Video_f);
	Cmd_AddCommand ("menu_developer", M_Developer_f); // mankrip
	Cmd_AddCommand ("help", M_Help_f);
	Cmd_AddCommand ("menu_quit", M_Quit_f);
}
