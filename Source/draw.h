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

// draw.h -- these are the only functions outside the refresh allowed
// to touch the vid buffer
extern	qpic_t		*draw_disc;	// also used on sbar
// mankrip - begin
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
// mankrip - end
void Draw_Init (void);
void Draw_Character (int x, int y, int num);
#if 0
void Draw_DebugChar (char num);
#endif
void Draw_TransPic (int x, int y, qpic_t *pic);
void Draw_TransPicMirror (int x, int y, qpic_t *pic); // mankrip
void Draw_TransPicTranslate (int x, int y, qpic_t *pic, byte *translation);
void Draw_ConsoleBackground (int lines);
void Draw_BeginDisc (void);
void Draw_EndDisc (void);
void Draw_TileClear (int x, int y, int w, int h);
void Draw_Fill (int x, int y, int w, int h, byte c); // mankrip - edited
void Draw_FadeScreen ();
void Draw_String (int x, int y, char *str);
qpic_t *Draw_PicFromWad (char *name);
qpic_t *Draw_CachePic (char *path);
// mankrip - begin
extern void
	M_DrawCharacter (int cx, int line, int num)
,	M_Print (int cx, int cy, char *str)
,	M_PrintWhite (int cx, int cy, char *str)
,	M_DrawTransPic (int x, int y, qpic_t *pic)
,	M_DrawTransPicMirror (int x, int y, qpic_t *pic)
,	M_DrawTransPicTranslate (int x, int y, qpic_t *pic)
,	M_DrawTransPicTranslateMirror (int x, int y, qpic_t *pic)
,	M_DrawTextBox (int x, int y, int width, int lines)
,	M_DrawPlaque (char *c, qboolean b)
,	M_Draw_Fill (int x, int y, int w, int h, byte c)
,	Draw_UpdateBorders (void)
,	Draw_UpdateAlignment (int h, int v) // aligns the drawing area
	;
//#define M_Draw_Fill(x,y,w,h,c) Draw_Fill(scr_2d_padding_x+x*scr_2d_scale_h,scr_2d_padding_y+y*scr_2d_scale_v,w*scr_2d_scale_h,h*scr_2d_scale_v,c)
// mankrip - end
