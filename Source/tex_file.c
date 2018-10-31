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
#include "tex_file.h" // mankrip
extern byte		BestColor (int r, int g, int b, int start, int stop); // mankrip

/*
=============
LoadTGA
=============
*/
#ifndef _arch_dreamcast
#pragma pack(push,1)
#endif
typedef struct
{
	byte
		id_length
	,	colormap_type
	,	image_type
		;
	unsigned short
		colormap_index
	,	colormap_length
		;
	byte
		colormap_size
		;
	unsigned short
		x_origin
	,	y_origin
	,	width
	,	height
		;
	byte
		pixel_size
	,	attributes
		;
} TargaHeader;
#ifndef _arch_dreamcast
#pragma pack(pop)
#endif

void LoadTGA_as8bit (char *filename, byte **pic, int *width, int *height)
{
	TargaHeader		*filedata;					//pcx_t	*pcx;
	byte			*input, *output;			//*out,

	int				row, column;				//int		x, y;
//	int				mark;						// for hunk
	int				columns, rows, numPixels;	//int		dataByte, runLength;
	byte			*pixbuf;					//byte	*pix;
	byte
		red
	,	green
	,	blue
	,	alpha
	,	packetColor
		;
	float
		new_r
	,	new_g
	,	new_b
	,	new_a
		;
	int
		inputcols
		;
	loadedfile_t	*fileinfo;

	free (pic);
	*pic = NULL;

//	mark = Hunk_LowMark ();

	fileinfo = COM_LoadTempFile (filename);
//	fileinfo = COM_LoadHunkFile (filename);
	if (!fileinfo)
		return;

	// parse the file
	filedata = (TargaHeader *) fileinfo->data;

	if (filedata->colormap_type != 0)
		Sys_Error ("LoadTGA: No colormaps supported\non \"%s\"\n", filename);

	if (filedata->pixel_size != 24)
		if (filedata->pixel_size != 32)
			Sys_Error ("LoadTGA: Only 32 or 24 bit images supported\non \"%s\",\n%i not supported", filename, (int) (filedata->pixel_size));

	columns	= LittleShort (filedata->width	);
	rows	= LittleShort (filedata->height	);
	numPixels = columns * rows;
	inputcols = (filedata->pixel_size / 8) * columns;

	if (width)
		*width = columns;
	if (height)
		*height = rows;

	*pic = output = malloc (numPixels);
	input = (byte *) filedata + sizeof (TargaHeader) + filedata->id_length; // skip TARGA image comment

	if (filedata->image_type == 2 /* && developer.value == 0 */) // Uncompressed, RGB images, Frankie Sierra Lite dithering
	{
		for(row = rows - 1 ; row >= 0 ; row--)
		{
			pixbuf = output + row * columns;
			for (column = 0 ; column < columns ; column++)
			{
				blue	= input[0];
				green	= input[1];
				red		= input[2];
				alpha = (filedata->pixel_size == 32) ? input[3] : 255;

				*pixbuf++ = packetColor = (alpha == 0) ? 255 : BestColor ( (int)red, (int)green, (int)blue, 0, 254);
				if (row < rows -1)
				{
					new_r = (float) (red	- host_basepal[packetColor * 3    ]);
					new_g = (float) (green	- host_basepal[packetColor * 3 + 1]);
					new_b = (float) (blue	- host_basepal[packetColor * 3 + 2]);
					new_a = (float) (alpha	- ( (packetColor == 255) ? 0 : 255));

					switch (filedata->pixel_size)
					{
						case 24:
							// current line
							input[0 + 3] = (byte) minmaxclamp (0.0f, (float) input[0 + 3] + new_b * 2.0f / 4.0f, 255.0f);
							// next line
							input[0 - 3 + inputcols] = (byte) minmaxclamp (0.0f, (float) input[0 - 3 + inputcols] + new_b * 1.0f / 4.0f, 255.0f);
							input[0 + 0 + inputcols] = (byte) minmaxclamp (0.0f, (float) input[0 + 0 + inputcols] + new_b * 1.0f / 4.0f, 255.0f);

							// current line
							input[1 + 3] = (byte) minmaxclamp (0.0f, (float) input[1 + 3] + new_g * 2.0f / 4.0f, 255.0f);
							// next line
							input[1 - 3 + inputcols] = (byte) minmaxclamp (0.0f, (float) input[1 - 3 + inputcols] + new_g * 1.0f / 4.0f, 255.0f);
							input[1 + 0 + inputcols] = (byte) minmaxclamp (0.0f, (float) input[1 + 0 + inputcols] + new_g * 1.0f / 4.0f, 255.0f);

							// current line
							input[2 + 3] = (byte) minmaxclamp (0.0f, (float) input[2 + 3] + new_r * 2.0f / 4.0f, 255.0f);
							// next line
							input[2 - 3 + inputcols] = (byte) minmaxclamp (0.0f, (float) input[2 - 3 + inputcols] + new_r * 1.0f / 4.0f, 255.0f);
							input[2 + 0 + inputcols] = (byte) minmaxclamp (0.0f, (float) input[2 + 0 + inputcols] + new_r * 1.0f / 4.0f, 255.0f);
							break;
						case 32:
							// current line
							input[0 + 3] = (byte) minmaxclamp (0.0f, (float) input[0 + 3] + new_b * 2.0f / 4.0f, 255.0f);
							// next line
							input[0 - 3 + inputcols] = (byte) minmaxclamp (0.0f, (float) input[0 - 3 + inputcols] + new_b * 1.0f / 4.0f, 255.0f);
							input[0 + 0 + inputcols] = (byte) minmaxclamp (0.0f, (float) input[0 + 0 + inputcols] + new_b * 1.0f / 4.0f, 255.0f);

							// current line
							input[1 + 3] = (byte) minmaxclamp (0.0f, (float) input[1 + 3] + new_g * 2.0f / 4.0f, 255.0f);
							// next line
							input[1 - 3 + inputcols] = (byte) minmaxclamp (0.0f, (float) input[1 - 3 + inputcols] + new_g * 1.0f / 4.0f, 255.0f);
							input[1 + 0 + inputcols] = (byte) minmaxclamp (0.0f, (float) input[1 + 0 + inputcols] + new_g * 1.0f / 4.0f, 255.0f);

							// current line
							input[2 + 3] = (byte) minmaxclamp (0.0f, (float) input[2 + 3] + new_r * 2.0f / 4.0f, 255.0f);
							// next line
							input[2 - 3 + inputcols] = (byte) minmaxclamp (0.0f, (float) input[2 - 3 + inputcols] + new_r * 1.0f / 4.0f, 255.0f);
							input[2 + 0 + inputcols] = (byte) minmaxclamp (0.0f, (float) input[2 + 0 + inputcols] + new_r * 1.0f / 4.0f, 255.0f);

							// current line
							input[3 + 3] = (byte) minmaxclamp (0.0f, (float) input[3 + 3] + new_a * 2.0f / 4.0f, 255.0f);
							// next line
							input[3 - 3 + inputcols] = (byte) minmaxclamp (0.0f, (float) input[3 - 3 + inputcols] + new_a * 1.0f / 4.0f, 255.0f);
							input[3 + 0 + inputcols] = (byte) minmaxclamp (0.0f, (float) input[3 + 0 + inputcols] + new_a * 1.0f / 4.0f, 255.0f);
							break;
					}
				}

				input += (filedata->pixel_size / 8);
			}
		}
	}
	#if 0
	else if (filedata->image_type == 2 && developer.value == 1) // Uncompressed, RGB images, Floyd–Steinberg dithering
	{
		for(row = rows - 1 ; row >= 0 ; row--)
		{
			pixbuf = output + row * columns;
			for (column = 0 ; column < columns ; column++)
			{
				blue	= input[0];
				green	= input[1];
				red		= input[2];
				alpha = (filedata->pixel_size == 32) ? input[3] : 255;

				*pixbuf++ = packetColor = (alpha == 0) ? 255 : BestColor ( (int)red, (int)green, (int)blue, 0, 254);
				if (row < rows -2)
				{
					new_r = (float) (red	- host_basepal[packetColor * 3    ]);
					new_g = (float) (green	- host_basepal[packetColor * 3 + 1]);
					new_b = (float) (blue	- host_basepal[packetColor * 3 + 2]);
					new_a = (float) (alpha	- ( (packetColor == 255) ? 0 : 255));

					switch (filedata->pixel_size)
					{
						case 24:
							input[0 + 3				] = (byte) minmaxclamp (0.0f, (float) input[0 + 3				] + new_b * 7.0f / 16.0f, 255.0f);
							input[0 - 3 + inputcols	] = (byte) minmaxclamp (0.0f, (float) input[0 - 3 + inputcols	] + new_b * 3.0f / 16.0f, 255.0f);
							input[0 + 0 + inputcols	] = (byte) minmaxclamp (0.0f, (float) input[0 + 0 + inputcols	] + new_b * 5.0f / 16.0f, 255.0f);
							input[0 + 3 + inputcols	] = (byte) minmaxclamp (0.0f, (float) input[0 + 3 + inputcols	] + new_b * 1.0f / 16.0f, 255.0f);

							input[1 + 3				] = (byte) minmaxclamp (0.0f, (float) input[1 + 3				] + new_g * 7.0f / 16.0f, 255.0f);
							input[1 - 3 + inputcols	] = (byte) minmaxclamp (0.0f, (float) input[1 - 3 + inputcols	] + new_g * 3.0f / 16.0f, 255.0f);
							input[1 + 0 + inputcols	] = (byte) minmaxclamp (0.0f, (float) input[1 + 0 + inputcols	] + new_g * 5.0f / 16.0f, 255.0f);
							input[1 + 3 + inputcols	] = (byte) minmaxclamp (0.0f, (float) input[1 + 3 + inputcols	] + new_g * 1.0f / 16.0f, 255.0f);

							input[2 + 3				] = (byte) minmaxclamp (0.0f, (float) input[2 + 3				] + new_r * 7.0f / 16.0f, 255.0f);
							input[2 - 3 + inputcols	] = (byte) minmaxclamp (0.0f, (float) input[2 - 3 + inputcols	] + new_r * 3.0f / 16.0f, 255.0f);
							input[2 + 0 + inputcols	] = (byte) minmaxclamp (0.0f, (float) input[2 + 0 + inputcols	] + new_r * 5.0f / 16.0f, 255.0f);
							input[2 + 3 + inputcols	] = (byte) minmaxclamp (0.0f, (float) input[2 + 3 + inputcols	] + new_r * 1.0f / 16.0f, 255.0f);
							break;
						case 32:
							input[0 + 4				] = (byte) minmaxclamp (0.0f, (float) input[0 + 4				] + new_b * 7.0f / 16.0f, 255.0f);
							input[0 - 4 + inputcols	] = (byte) minmaxclamp (0.0f, (float) input[0 - 4 + inputcols	] + new_b * 3.0f / 16.0f, 255.0f);
							input[0 + 0 + inputcols	] = (byte) minmaxclamp (0.0f, (float) input[0 + 0 + inputcols	] + new_b * 5.0f / 16.0f, 255.0f);
							input[0 + 4 + inputcols	] = (byte) minmaxclamp (0.0f, (float) input[0 + 4 + inputcols	] + new_b * 1.0f / 16.0f, 255.0f);

							input[1 + 4				] = (byte) minmaxclamp (0.0f, (float) input[1 + 4				] + new_g * 7.0f / 16.0f, 255.0f);
							input[1 - 4 + inputcols	] = (byte) minmaxclamp (0.0f, (float) input[1 - 4 + inputcols	] + new_g * 3.0f / 16.0f, 255.0f);
							input[1 + 0 + inputcols	] = (byte) minmaxclamp (0.0f, (float) input[1 + 0 + inputcols	] + new_g * 5.0f / 16.0f, 255.0f);
							input[1 + 4 + inputcols	] = (byte) minmaxclamp (0.0f, (float) input[1 + 4 + inputcols	] + new_g * 1.0f / 16.0f, 255.0f);

							input[2 + 4				] = (byte) minmaxclamp (0.0f, (float) input[2 + 4				] + new_r * 7.0f / 16.0f, 255.0f);
							input[2 - 4 + inputcols	] = (byte) minmaxclamp (0.0f, (float) input[2 - 4 + inputcols	] + new_r * 3.0f / 16.0f, 255.0f);
							input[2 + 0 + inputcols	] = (byte) minmaxclamp (0.0f, (float) input[2 + 0 + inputcols	] + new_r * 5.0f / 16.0f, 255.0f);
							input[2 + 4 + inputcols	] = (byte) minmaxclamp (0.0f, (float) input[2 + 4 + inputcols	] + new_r * 1.0f / 16.0f, 255.0f);

							input[3 + 4				] = (byte) minmaxclamp (0.0f, (float) input[3 + 4				] + new_a * 7.0f / 16.0f, 255.0f);
							input[3 - 4 + inputcols	] = (byte) minmaxclamp (0.0f, (float) input[3 - 4 + inputcols	] + new_a * 3.0f / 16.0f, 255.0f);
							input[3 + 0 + inputcols	] = (byte) minmaxclamp (0.0f, (float) input[3 + 0 + inputcols	] + new_a * 5.0f / 16.0f, 255.0f);
							input[3 + 4 + inputcols	] = (byte) minmaxclamp (0.0f, (float) input[3 + 4 + inputcols	] + new_a * 1.0f / 16.0f, 255.0f);
							break;
					}
				}

				input += (filedata->pixel_size / 8);
			}
		}
	}
	else if (filedata->image_type == 2 && developer.value == 2) // Uncompressed, RGB images, Bill Atkinson dithering
	{
		for(row = rows - 1 ; row >= 0 ; row--)
		{
			pixbuf = output + row * columns;
			for (column = 0 ; column < columns ; column++)
			{
				blue	= input[0];
				green	= input[1];
				red		= input[2];
				alpha = (filedata->pixel_size == 32) ? input[3] : 255;

				*pixbuf++ = packetColor = (alpha == 0) ? 255 : BestColor ( (int)red, (int)green, (int)blue, 0, 254);
				if (row < rows -2)
				{
					new_r = (float) (red	- host_basepal[packetColor * 3    ]) / 8.0f;
					new_g = (float) (green	- host_basepal[packetColor * 3 + 1]) / 8.0f;
					new_b = (float) (blue	- host_basepal[packetColor * 3 + 2]) / 8.0f;
					new_a = (float) (alpha	- ( (packetColor == 255) ? 0 : 255)) / 8.0f;

					switch (filedata->pixel_size)
					{
						case 24:
							input[0 + 3				] = (byte) minmaxclamp (0.0f, (float) input[0 + 3				] + new_b, 255.0f);
							input[0 + 6				] = (byte) minmaxclamp (0.0f, (float) input[0 + 6				] + new_b, 255.0f);
							input[0 - 3 + inputcols	] = (byte) minmaxclamp (0.0f, (float) input[0 - 3 + inputcols	] + new_b, 255.0f);
							input[0 + 0 + inputcols	] = (byte) minmaxclamp (0.0f, (float) input[0 + 0 + inputcols	] + new_b, 255.0f);
							input[0 + 3 + inputcols	] = (byte) minmaxclamp (0.0f, (float) input[0 + 3 + inputcols	] + new_b, 255.0f);
							input[0 + 2 * inputcols ] = (byte) minmaxclamp (0.0f, (float) input[0 + 2 * inputcols	] + new_b, 255.0f);

							input[1 + 3				] = (byte) minmaxclamp (0.0f, (float) input[1 + 3				] + new_g, 255.0f);
							input[1 + 6				] = (byte) minmaxclamp (0.0f, (float) input[1 + 6				] + new_g, 255.0f);
							input[1 - 3 + inputcols	] = (byte) minmaxclamp (0.0f, (float) input[1 - 3 + inputcols	] + new_g, 255.0f);
							input[1 + 0 + inputcols	] = (byte) minmaxclamp (0.0f, (float) input[1 + 0 + inputcols	] + new_g, 255.0f);
							input[1 + 3 + inputcols	] = (byte) minmaxclamp (0.0f, (float) input[1 + 3 + inputcols	] + new_g, 255.0f);
							input[1 + 2 * inputcols ] = (byte) minmaxclamp (0.0f, (float) input[1 + 2 * inputcols	] + new_g, 255.0f);

							input[2 + 3				] = (byte) minmaxclamp (0.0f, (float) input[2 + 3				] + new_r, 255.0f);
							input[2 + 6				] = (byte) minmaxclamp (0.0f, (float) input[2 + 6				] + new_r, 255.0f);
							input[2 - 3 + inputcols	] = (byte) minmaxclamp (0.0f, (float) input[2 - 3 + inputcols	] + new_r, 255.0f);
							input[2 + 0 + inputcols	] = (byte) minmaxclamp (0.0f, (float) input[2 + 0 + inputcols	] + new_r, 255.0f);
							input[2 + 3 + inputcols	] = (byte) minmaxclamp (0.0f, (float) input[2 + 3 + inputcols	] + new_r, 255.0f);
							input[2 + 2 * inputcols ] = (byte) minmaxclamp (0.0f, (float) input[2 + 2 * inputcols	] + new_r, 255.0f);
							break;
						case 32:
							input[0 + 4				] = (byte) minmaxclamp (0.0f, (float) input[0 + 4				] + new_b, 255.0f);
							input[0 + 8				] = (byte) minmaxclamp (0.0f, (float) input[0 + 8				] + new_b, 255.0f);
							input[0 - 4 + inputcols	] = (byte) minmaxclamp (0.0f, (float) input[0 - 4 + inputcols	] + new_b, 255.0f);
							input[0 + 0 + inputcols	] = (byte) minmaxclamp (0.0f, (float) input[0 + 0 + inputcols	] + new_b, 255.0f);
							input[0 + 4 + inputcols	] = (byte) minmaxclamp (0.0f, (float) input[0 + 4 + inputcols	] + new_b, 255.0f);
							input[0 + 2 * inputcols ] = (byte) minmaxclamp (0.0f, (float) input[0 + 2 * inputcols	] + new_b, 255.0f);

							input[1 + 4				] = (byte) minmaxclamp (0.0f, (float) input[1 + 4				] + new_g, 255.0f);
							input[1 + 8				] = (byte) minmaxclamp (0.0f, (float) input[1 + 8				] + new_g, 255.0f);
							input[1 - 4 + inputcols	] = (byte) minmaxclamp (0.0f, (float) input[1 - 4 + inputcols	] + new_g, 255.0f);
							input[1 + 0 + inputcols	] = (byte) minmaxclamp (0.0f, (float) input[1 + 0 + inputcols	] + new_g, 255.0f);
							input[1 + 4 + inputcols	] = (byte) minmaxclamp (0.0f, (float) input[1 + 4 + inputcols	] + new_g, 255.0f);
							input[1 + 2 * inputcols ] = (byte) minmaxclamp (0.0f, (float) input[1 + 2 * inputcols	] + new_g, 255.0f);

							input[2 + 4				] = (byte) minmaxclamp (0.0f, (float) input[2 + 4				] + new_r, 255.0f);
							input[2 + 8				] = (byte) minmaxclamp (0.0f, (float) input[2 + 8				] + new_r, 255.0f);
							input[2 - 4 + inputcols	] = (byte) minmaxclamp (0.0f, (float) input[2 - 4 + inputcols	] + new_r, 255.0f);
							input[2 + 0 + inputcols	] = (byte) minmaxclamp (0.0f, (float) input[2 + 0 + inputcols	] + new_r, 255.0f);
							input[2 + 4 + inputcols	] = (byte) minmaxclamp (0.0f, (float) input[2 + 4 + inputcols	] + new_r, 255.0f);
							input[2 + 2 * inputcols ] = (byte) minmaxclamp (0.0f, (float) input[2 + 2 * inputcols	] + new_r, 255.0f);

							input[3 + 4				] = (byte) minmaxclamp (0.0f, (float) input[3 + 4				] + new_a, 255.0f);
							input[3 + 8				] = (byte) minmaxclamp (0.0f, (float) input[3 + 8				] + new_a, 255.0f);
							input[3 - 4 + inputcols	] = (byte) minmaxclamp (0.0f, (float) input[3 - 4 + inputcols	] + new_a, 255.0f);
							input[3 + 0 + inputcols	] = (byte) minmaxclamp (0.0f, (float) input[3 + 0 + inputcols	] + new_a, 255.0f);
							input[3 + 4 + inputcols	] = (byte) minmaxclamp (0.0f, (float) input[3 + 4 + inputcols	] + new_a, 255.0f);
							input[3 + 2 * inputcols ] = (byte) minmaxclamp (0.0f, (float) input[3 + 2 * inputcols	] + new_a, 255.0f);
							break;
					}
				}

				input += (filedata->pixel_size / 8);
			}
		}
	}
	else if (filedata->image_type == 2 && developer.value == 3) // Uncompressed, RGB images, P. Stucki dithering
	{
		for(row = rows - 1 ; row >= 0 ; row--)
		{
			pixbuf = output + row * columns;
			for (column = 0 ; column < columns ; column++)
			{
				blue	= input[0];
				green	= input[1];
				red		= input[2];
				alpha = (filedata->pixel_size == 32) ? input[3] : 255;

				*pixbuf++ = packetColor = (alpha == 0) ? 255 : BestColor ( (int)red, (int)green, (int)blue, 0, 254);
				if (row < rows -2)
				{
					new_r = (float) (red	- host_basepal[packetColor * 3    ]);
					new_g = (float) (green	- host_basepal[packetColor * 3 + 1]);
					new_b = (float) (blue	- host_basepal[packetColor * 3 + 2]);
					new_a = (float) (alpha	- ( (packetColor == 255) ? 0 : 255));

					switch (filedata->pixel_size)
					{
						case 24:
							// current line
							input[0 + 3] = (byte) minmaxclamp (0.0f, (float) input[0 + 3] + new_b * 8.0f / 42.0f, 255.0f);
							input[0 + 6] = (byte) minmaxclamp (0.0f, (float) input[0 + 6] + new_b * 4.0f / 42.0f, 255.0f);
							// next line
							input[0 - 6 + inputcols] = (byte) minmaxclamp (0.0f, (float) input[0 - 6 + inputcols] + new_b * 2.0f / 42.0f, 255.0f);
							input[0 - 3 + inputcols] = (byte) minmaxclamp (0.0f, (float) input[0 - 3 + inputcols] + new_b * 4.0f / 42.0f, 255.0f);
							input[0 + 0 + inputcols] = (byte) minmaxclamp (0.0f, (float) input[0 + 0 + inputcols] + new_b * 8.0f / 42.0f, 255.0f);
							input[0 + 3 + inputcols] = (byte) minmaxclamp (0.0f, (float) input[0 + 3 + inputcols] + new_b * 4.0f / 42.0f, 255.0f);
							input[0 + 6 + inputcols] = (byte) minmaxclamp (0.0f, (float) input[0 + 6 + inputcols] + new_b * 2.0f / 42.0f, 255.0f);
							// after-next line
							input[0 - 6 + 2 * inputcols] = (byte) minmaxclamp (0.0f, (float) input[0 - 6 + 2 * inputcols] + new_b * 1.0f / 42.0f, 255.0f);
							input[0 - 3 + 2 * inputcols] = (byte) minmaxclamp (0.0f, (float) input[0 - 3 + 2 * inputcols] + new_b * 2.0f / 42.0f, 255.0f);
							input[0 + 0 + 2 * inputcols] = (byte) minmaxclamp (0.0f, (float) input[0 + 0 + 2 * inputcols] + new_b * 4.0f / 42.0f, 255.0f);
							input[0 + 3 + 2 * inputcols] = (byte) minmaxclamp (0.0f, (float) input[0 + 3 + 2 * inputcols] + new_b * 2.0f / 42.0f, 255.0f);
							input[0 + 6 + 2 * inputcols] = (byte) minmaxclamp (0.0f, (float) input[0 + 6 + 2 * inputcols] + new_b * 1.0f / 42.0f, 255.0f);

							// current line
							input[1 + 3] = (byte) minmaxclamp (0.0f, (float) input[1 + 3] + new_g * 8.0f / 42.0f, 255.0f);
							input[1 + 6] = (byte) minmaxclamp (0.0f, (float) input[1 + 6] + new_g * 4.0f / 42.0f, 255.0f);
							// next line
							input[1 - 6 + inputcols] = (byte) minmaxclamp (0.0f, (float) input[1 - 6 + inputcols] + new_g * 2.0f / 42.0f, 255.0f);
							input[1 - 3 + inputcols] = (byte) minmaxclamp (0.0f, (float) input[1 - 3 + inputcols] + new_g * 4.0f / 42.0f, 255.0f);
							input[1 + 0 + inputcols] = (byte) minmaxclamp (0.0f, (float) input[1 + 0 + inputcols] + new_g * 8.0f / 42.0f, 255.0f);
							input[1 + 3 + inputcols] = (byte) minmaxclamp (0.0f, (float) input[1 + 3 + inputcols] + new_g * 4.0f / 42.0f, 255.0f);
							input[1 + 6 + inputcols] = (byte) minmaxclamp (0.0f, (float) input[1 + 6 + inputcols] + new_g * 2.0f / 42.0f, 255.0f);
							// after-next line
							input[1 - 6 + 2 * inputcols] = (byte) minmaxclamp (0.0f, (float) input[1 - 6 + 2 * inputcols] + new_g * 1.0f / 42.0f, 255.0f);
							input[1 - 3 + 2 * inputcols] = (byte) minmaxclamp (0.0f, (float) input[1 - 3 + 2 * inputcols] + new_g * 2.0f / 42.0f, 255.0f);
							input[1 + 0 + 2 * inputcols] = (byte) minmaxclamp (0.0f, (float) input[1 + 0 + 2 * inputcols] + new_g * 4.0f / 42.0f, 255.0f);
							input[1 + 3 + 2 * inputcols] = (byte) minmaxclamp (0.0f, (float) input[1 + 3 + 2 * inputcols] + new_g * 2.0f / 42.0f, 255.0f);
							input[1 + 6 + 2 * inputcols] = (byte) minmaxclamp (0.0f, (float) input[1 + 6 + 2 * inputcols] + new_g * 1.0f / 42.0f, 255.0f);

							// current line
							input[2 + 3] = (byte) minmaxclamp (0.0f, (float) input[2 + 3] + new_r * 8.0f / 42.0f, 255.0f);
							input[2 + 6] = (byte) minmaxclamp (0.0f, (float) input[2 + 6] + new_r * 4.0f / 42.0f, 255.0f);
							// next line
							input[2 - 6 + inputcols] = (byte) minmaxclamp (0.0f, (float) input[2 - 6 + inputcols] + new_r * 2.0f / 42.0f, 255.0f);
							input[2 - 3 + inputcols] = (byte) minmaxclamp (0.0f, (float) input[2 - 3 + inputcols] + new_r * 4.0f / 42.0f, 255.0f);
							input[2 + 0 + inputcols] = (byte) minmaxclamp (0.0f, (float) input[2 + 0 + inputcols] + new_r * 8.0f / 42.0f, 255.0f);
							input[2 + 3 + inputcols] = (byte) minmaxclamp (0.0f, (float) input[2 + 3 + inputcols] + new_r * 4.0f / 42.0f, 255.0f);
							input[2 + 6 + inputcols] = (byte) minmaxclamp (0.0f, (float) input[2 + 6 + inputcols] + new_r * 2.0f / 42.0f, 255.0f);
							// after-next line
							input[2 - 6 + 2 * inputcols] = (byte) minmaxclamp (0.0f, (float) input[2 - 6 + 2 * inputcols] + new_r * 1.0f / 42.0f, 255.0f);
							input[2 - 3 + 2 * inputcols] = (byte) minmaxclamp (0.0f, (float) input[2 - 3 + 2 * inputcols] + new_r * 2.0f / 42.0f, 255.0f);
							input[2 + 0 + 2 * inputcols] = (byte) minmaxclamp (0.0f, (float) input[2 + 0 + 2 * inputcols] + new_r * 4.0f / 42.0f, 255.0f);
							input[2 + 3 + 2 * inputcols] = (byte) minmaxclamp (0.0f, (float) input[2 + 3 + 2 * inputcols] + new_r * 2.0f / 42.0f, 255.0f);
							input[2 + 6 + 2 * inputcols] = (byte) minmaxclamp (0.0f, (float) input[2 + 6 + 2 * inputcols] + new_r * 1.0f / 42.0f, 255.0f);
							break;
						case 32:
							// current line
							input[0 + 4] = (byte) minmaxclamp (0.0f, (float) input[0 + 4] + new_b * 8.0f / 42.0f, 255.0f);
							input[0 + 8] = (byte) minmaxclamp (0.0f, (float) input[0 + 8] + new_b * 4.0f / 42.0f, 255.0f);
							// next line
							input[0 - 8 + inputcols] = (byte) minmaxclamp (0.0f, (float) input[0 - 8 + inputcols] + new_b * 2.0f / 42.0f, 255.0f);
							input[0 - 4 + inputcols] = (byte) minmaxclamp (0.0f, (float) input[0 - 4 + inputcols] + new_b * 4.0f / 42.0f, 255.0f);
							input[0 + 0 + inputcols] = (byte) minmaxclamp (0.0f, (float) input[0 + 0 + inputcols] + new_b * 8.0f / 42.0f, 255.0f);
							input[0 + 4 + inputcols] = (byte) minmaxclamp (0.0f, (float) input[0 + 4 + inputcols] + new_b * 4.0f / 42.0f, 255.0f);
							input[0 + 8 + inputcols] = (byte) minmaxclamp (0.0f, (float) input[0 + 8 + inputcols] + new_b * 2.0f / 42.0f, 255.0f);
							// after-next line
							input[0 - 8 + 2 * inputcols] = (byte) minmaxclamp (0.0f, (float) input[0 - 8 + 2 * inputcols] + new_b * 1.0f / 42.0f, 255.0f);
							input[0 - 4 + 2 * inputcols] = (byte) minmaxclamp (0.0f, (float) input[0 - 4 + 2 * inputcols] + new_b * 2.0f / 42.0f, 255.0f);
							input[0 + 0 + 2 * inputcols] = (byte) minmaxclamp (0.0f, (float) input[0 + 0 + 2 * inputcols] + new_b * 4.0f / 42.0f, 255.0f);
							input[0 + 4 + 2 * inputcols] = (byte) minmaxclamp (0.0f, (float) input[0 + 4 + 2 * inputcols] + new_b * 2.0f / 42.0f, 255.0f);
							input[0 + 8 + 2 * inputcols] = (byte) minmaxclamp (0.0f, (float) input[0 + 8 + 2 * inputcols] + new_b * 1.0f / 42.0f, 255.0f);

							// current line
							input[1 + 4] = (byte) minmaxclamp (0.0f, (float) input[1 + 4] + new_g * 8.0f / 42.0f, 255.0f);
							input[1 + 8] = (byte) minmaxclamp (0.0f, (float) input[1 + 8] + new_g * 4.0f / 42.0f, 255.0f);
							// next line
							input[1 - 8 + inputcols] = (byte) minmaxclamp (0.0f, (float) input[1 - 8 + inputcols] + new_g * 2.0f / 42.0f, 255.0f);
							input[1 - 4 + inputcols] = (byte) minmaxclamp (0.0f, (float) input[1 - 4 + inputcols] + new_g * 4.0f / 42.0f, 255.0f);
							input[1 + 0 + inputcols] = (byte) minmaxclamp (0.0f, (float) input[1 + 0 + inputcols] + new_g * 8.0f / 42.0f, 255.0f);
							input[1 + 4 + inputcols] = (byte) minmaxclamp (0.0f, (float) input[1 + 4 + inputcols] + new_g * 4.0f / 42.0f, 255.0f);
							input[1 + 8 + inputcols] = (byte) minmaxclamp (0.0f, (float) input[1 + 8 + inputcols] + new_g * 2.0f / 42.0f, 255.0f);
							// after-next line
							input[1 - 8 + 2 * inputcols] = (byte) minmaxclamp (0.0f, (float) input[1 - 8 + 2 * inputcols] + new_g * 1.0f / 42.0f, 255.0f);
							input[1 - 4 + 2 * inputcols] = (byte) minmaxclamp (0.0f, (float) input[1 - 4 + 2 * inputcols] + new_g * 2.0f / 42.0f, 255.0f);
							input[1 + 0 + 2 * inputcols] = (byte) minmaxclamp (0.0f, (float) input[1 + 0 + 2 * inputcols] + new_g * 4.0f / 42.0f, 255.0f);
							input[1 + 4 + 2 * inputcols] = (byte) minmaxclamp (0.0f, (float) input[1 + 4 + 2 * inputcols] + new_g * 2.0f / 42.0f, 255.0f);
							input[1 + 8 + 2 * inputcols] = (byte) minmaxclamp (0.0f, (float) input[1 + 8 + 2 * inputcols] + new_g * 1.0f / 42.0f, 255.0f);

							// current line
							input[2 + 4] = (byte) minmaxclamp (0.0f, (float) input[2 + 4] + new_r * 8.0f / 42.0f, 255.0f);
							input[2 + 8] = (byte) minmaxclamp (0.0f, (float) input[2 + 8] + new_r * 4.0f / 42.0f, 255.0f);
							// next line
							input[2 - 8 + inputcols] = (byte) minmaxclamp (0.0f, (float) input[2 - 8 + inputcols] + new_r * 2.0f / 42.0f, 255.0f);
							input[2 - 4 + inputcols] = (byte) minmaxclamp (0.0f, (float) input[2 - 4 + inputcols] + new_r * 4.0f / 42.0f, 255.0f);
							input[2 + 0 + inputcols] = (byte) minmaxclamp (0.0f, (float) input[2 + 0 + inputcols] + new_r * 8.0f / 42.0f, 255.0f);
							input[2 + 4 + inputcols] = (byte) minmaxclamp (0.0f, (float) input[2 + 4 + inputcols] + new_r * 4.0f / 42.0f, 255.0f);
							input[2 + 8 + inputcols] = (byte) minmaxclamp (0.0f, (float) input[2 + 8 + inputcols] + new_r * 2.0f / 42.0f, 255.0f);
							// after-next line
							input[2 - 8 + 2 * inputcols] = (byte) minmaxclamp (0.0f, (float) input[2 - 8 + 2 * inputcols] + new_r * 1.0f / 42.0f, 255.0f);
							input[2 - 4 + 2 * inputcols] = (byte) minmaxclamp (0.0f, (float) input[2 - 4 + 2 * inputcols] + new_r * 2.0f / 42.0f, 255.0f);
							input[2 + 0 + 2 * inputcols] = (byte) minmaxclamp (0.0f, (float) input[2 + 0 + 2 * inputcols] + new_r * 4.0f / 42.0f, 255.0f);
							input[2 + 4 + 2 * inputcols] = (byte) minmaxclamp (0.0f, (float) input[2 + 4 + 2 * inputcols] + new_r * 2.0f / 42.0f, 255.0f);
							input[2 + 8 + 2 * inputcols] = (byte) minmaxclamp (0.0f, (float) input[2 + 8 + 2 * inputcols] + new_r * 1.0f / 42.0f, 255.0f);

							// current line
							input[3 + 4] = (byte) minmaxclamp (0.0f, (float) input[3 + 4] + new_a * 8.0f / 42.0f, 255.0f);
							input[3 + 8] = (byte) minmaxclamp (0.0f, (float) input[3 + 8] + new_a * 4.0f / 42.0f, 255.0f);
							// next line
							input[3 - 8 + inputcols] = (byte) minmaxclamp (0.0f, (float) input[3 - 8 + inputcols] + new_a * 2.0f / 42.0f, 255.0f);
							input[3 - 4 + inputcols] = (byte) minmaxclamp (0.0f, (float) input[3 - 4 + inputcols] + new_a * 4.0f / 42.0f, 255.0f);
							input[3 + 0 + inputcols] = (byte) minmaxclamp (0.0f, (float) input[3 + 0 + inputcols] + new_a * 8.0f / 42.0f, 255.0f);
							input[3 + 4 + inputcols] = (byte) minmaxclamp (0.0f, (float) input[3 + 4 + inputcols] + new_a * 4.0f / 42.0f, 255.0f);
							input[3 + 8 + inputcols] = (byte) minmaxclamp (0.0f, (float) input[3 + 8 + inputcols] + new_a * 2.0f / 42.0f, 255.0f);
							// after-next line
							input[3 - 8 + 2 * inputcols] = (byte) minmaxclamp (0.0f, (float) input[3 - 8 + 2 * inputcols] + new_a * 1.0f / 42.0f, 255.0f);
							input[3 - 4 + 2 * inputcols] = (byte) minmaxclamp (0.0f, (float) input[3 - 4 + 2 * inputcols] + new_a * 2.0f / 42.0f, 255.0f);
							input[3 + 0 + 2 * inputcols] = (byte) minmaxclamp (0.0f, (float) input[3 + 0 + 2 * inputcols] + new_a * 4.0f / 42.0f, 255.0f);
							input[3 + 4 + 2 * inputcols] = (byte) minmaxclamp (0.0f, (float) input[3 + 4 + 2 * inputcols] + new_a * 2.0f / 42.0f, 255.0f);
							input[3 + 8 + 2 * inputcols] = (byte) minmaxclamp (0.0f, (float) input[3 + 8 + 2 * inputcols] + new_a * 1.0f / 42.0f, 255.0f);
							break;
					}
				}

				input += (filedata->pixel_size / 8);
			}
		}
	}
	else if (filedata->image_type == 2 && developer.value == 4) // Uncompressed, RGB images, Frankie Sierra dithering
	{
		for(row = rows - 1 ; row >= 0 ; row--)
		{
			pixbuf = output + row * columns;
			for (column = 0 ; column < columns ; column++)
			{
				blue	= input[0];
				green	= input[1];
				red		= input[2];
				alpha = (filedata->pixel_size == 32) ? input[3] : 255;

				*pixbuf++ = packetColor = (alpha == 0) ? 255 : BestColor ( (int)red, (int)green, (int)blue, 0, 254);
				if (row < rows -2)
				{
					new_r = (float) (red	- host_basepal[packetColor * 3    ]);
					new_g = (float) (green	- host_basepal[packetColor * 3 + 1]);
					new_b = (float) (blue	- host_basepal[packetColor * 3 + 2]);
					new_a = (float) (alpha	- ( (packetColor == 255) ? 0 : 255));

					switch (filedata->pixel_size)
					{
						case 24:
							// current line
							input[0 + 3] = (byte) minmaxclamp (0.0f, (float) input[0 + 3] + new_b * 5.0f / 32.0f, 255.0f);
							input[0 + 6] = (byte) minmaxclamp (0.0f, (float) input[0 + 6] + new_b * 3.0f / 32.0f, 255.0f);
							// next line
							input[0 - 6 + inputcols] = (byte) minmaxclamp (0.0f, (float) input[0 - 6 + inputcols] + new_b * 2.0f / 32.0f, 255.0f);
							input[0 - 3 + inputcols] = (byte) minmaxclamp (0.0f, (float) input[0 - 3 + inputcols] + new_b * 4.0f / 32.0f, 255.0f);
							input[0 + 0 + inputcols] = (byte) minmaxclamp (0.0f, (float) input[0 + 0 + inputcols] + new_b * 5.0f / 32.0f, 255.0f);
							input[0 + 3 + inputcols] = (byte) minmaxclamp (0.0f, (float) input[0 + 3 + inputcols] + new_b * 4.0f / 32.0f, 255.0f);
							input[0 + 6 + inputcols] = (byte) minmaxclamp (0.0f, (float) input[0 + 6 + inputcols] + new_b * 2.0f / 32.0f, 255.0f);
							// after-next line
							input[0 - 3 + 2 * inputcols] = (byte) minmaxclamp (0.0f, (float) input[0 - 3 + 2 * inputcols] + new_b * 2.0f / 32.0f, 255.0f);
							input[0 + 0 + 2 * inputcols] = (byte) minmaxclamp (0.0f, (float) input[0 + 0 + 2 * inputcols] + new_b * 3.0f / 32.0f, 255.0f);
							input[0 + 3 + 2 * inputcols] = (byte) minmaxclamp (0.0f, (float) input[0 + 3 + 2 * inputcols] + new_b * 2.0f / 32.0f, 255.0f);

							// current line
							input[1 + 3] = (byte) minmaxclamp (0.0f, (float) input[1 + 3] + new_g * 5.0f / 32.0f, 255.0f);
							input[1 + 6] = (byte) minmaxclamp (0.0f, (float) input[1 + 6] + new_g * 3.0f / 32.0f, 255.0f);
							// next line
							input[1 - 6 + inputcols] = (byte) minmaxclamp (0.0f, (float) input[1 - 6 + inputcols] + new_g * 2.0f / 32.0f, 255.0f);
							input[1 - 3 + inputcols] = (byte) minmaxclamp (0.0f, (float) input[1 - 3 + inputcols] + new_g * 4.0f / 32.0f, 255.0f);
							input[1 + 0 + inputcols] = (byte) minmaxclamp (0.0f, (float) input[1 + 0 + inputcols] + new_g * 5.0f / 32.0f, 255.0f);
							input[1 + 3 + inputcols] = (byte) minmaxclamp (0.0f, (float) input[1 + 3 + inputcols] + new_g * 4.0f / 32.0f, 255.0f);
							input[1 + 6 + inputcols] = (byte) minmaxclamp (0.0f, (float) input[1 + 6 + inputcols] + new_g * 2.0f / 32.0f, 255.0f);
							// after-next line
							input[1 - 3 + 2 * inputcols] = (byte) minmaxclamp (0.0f, (float) input[1 - 3 + 2 * inputcols] + new_g * 2.0f / 32.0f, 255.0f);
							input[1 + 0 + 2 * inputcols] = (byte) minmaxclamp (0.0f, (float) input[1 + 0 + 2 * inputcols] + new_g * 3.0f / 32.0f, 255.0f);
							input[1 + 3 + 2 * inputcols] = (byte) minmaxclamp (0.0f, (float) input[1 + 3 + 2 * inputcols] + new_g * 2.0f / 32.0f, 255.0f);

							// current line
							input[2 + 3] = (byte) minmaxclamp (0.0f, (float) input[2 + 3] + new_r * 5.0f / 32.0f, 255.0f);
							input[2 + 6] = (byte) minmaxclamp (0.0f, (float) input[2 + 6] + new_r * 3.0f / 32.0f, 255.0f);
							// next line
							input[2 - 6 + inputcols] = (byte) minmaxclamp (0.0f, (float) input[2 - 6 + inputcols] + new_r * 2.0f / 32.0f, 255.0f);
							input[2 - 3 + inputcols] = (byte) minmaxclamp (0.0f, (float) input[2 - 3 + inputcols] + new_r * 4.0f / 32.0f, 255.0f);
							input[2 + 0 + inputcols] = (byte) minmaxclamp (0.0f, (float) input[2 + 0 + inputcols] + new_r * 5.0f / 32.0f, 255.0f);
							input[2 + 3 + inputcols] = (byte) minmaxclamp (0.0f, (float) input[2 + 3 + inputcols] + new_r * 4.0f / 32.0f, 255.0f);
							input[2 + 6 + inputcols] = (byte) minmaxclamp (0.0f, (float) input[2 + 6 + inputcols] + new_r * 2.0f / 32.0f, 255.0f);
							// after-next line
							input[2 - 3 + 2 * inputcols] = (byte) minmaxclamp (0.0f, (float) input[2 - 3 + 2 * inputcols] + new_r * 2.0f / 32.0f, 255.0f);
							input[2 + 0 + 2 * inputcols] = (byte) minmaxclamp (0.0f, (float) input[2 + 0 + 2 * inputcols] + new_r * 3.0f / 32.0f, 255.0f);
							input[2 + 3 + 2 * inputcols] = (byte) minmaxclamp (0.0f, (float) input[2 + 3 + 2 * inputcols] + new_r * 2.0f / 32.0f, 255.0f);
							break;

						case 32:
							// current line
							input[0 + 4] = (byte) minmaxclamp (0.0f, (float) input[0 + 4] + new_b * 5.0f / 32.0f, 255.0f);
							input[0 + 8] = (byte) minmaxclamp (0.0f, (float) input[0 + 8] + new_b * 3.0f / 32.0f, 255.0f);
							// next line
							input[0 - 8 + inputcols] = (byte) minmaxclamp (0.0f, (float) input[0 - 8 + inputcols] + new_b * 2.0f / 32.0f, 255.0f);
							input[0 - 4 + inputcols] = (byte) minmaxclamp (0.0f, (float) input[0 - 4 + inputcols] + new_b * 4.0f / 32.0f, 255.0f);
							input[0 + 0 + inputcols] = (byte) minmaxclamp (0.0f, (float) input[0 + 0 + inputcols] + new_b * 5.0f / 32.0f, 255.0f);
							input[0 + 4 + inputcols] = (byte) minmaxclamp (0.0f, (float) input[0 + 4 + inputcols] + new_b * 4.0f / 32.0f, 255.0f);
							input[0 + 8 + inputcols] = (byte) minmaxclamp (0.0f, (float) input[0 + 8 + inputcols] + new_b * 2.0f / 32.0f, 255.0f);
							// after-next line
							input[0 - 4 + 2 * inputcols] = (byte) minmaxclamp (0.0f, (float) input[0 - 4 + 2 * inputcols] + new_b * 2.0f / 32.0f, 255.0f);
							input[0 + 0 + 2 * inputcols] = (byte) minmaxclamp (0.0f, (float) input[0 + 0 + 2 * inputcols] + new_b * 3.0f / 32.0f, 255.0f);
							input[0 + 4 + 2 * inputcols] = (byte) minmaxclamp (0.0f, (float) input[0 + 4 + 2 * inputcols] + new_b * 2.0f / 32.0f, 255.0f);

							// current line
							input[1 + 4] = (byte) minmaxclamp (0.0f, (float) input[1 + 4] + new_g * 5.0f / 32.0f, 255.0f);
							input[1 + 8] = (byte) minmaxclamp (0.0f, (float) input[1 + 8] + new_g * 3.0f / 32.0f, 255.0f);
							// next line
							input[1 - 8 + inputcols] = (byte) minmaxclamp (0.0f, (float) input[1 - 8 + inputcols] + new_g * 2.0f / 32.0f, 255.0f);
							input[1 - 4 + inputcols] = (byte) minmaxclamp (0.0f, (float) input[1 - 4 + inputcols] + new_g * 4.0f / 32.0f, 255.0f);
							input[1 + 0 + inputcols] = (byte) minmaxclamp (0.0f, (float) input[1 + 0 + inputcols] + new_g * 5.0f / 32.0f, 255.0f);
							input[1 + 4 + inputcols] = (byte) minmaxclamp (0.0f, (float) input[1 + 4 + inputcols] + new_g * 4.0f / 32.0f, 255.0f);
							input[1 + 8 + inputcols] = (byte) minmaxclamp (0.0f, (float) input[1 + 8 + inputcols] + new_g * 2.0f / 32.0f, 255.0f);
							// after-next line
							input[1 - 4 + 2 * inputcols] = (byte) minmaxclamp (0.0f, (float) input[1 - 4 + 2 * inputcols] + new_g * 2.0f / 32.0f, 255.0f);
							input[1 + 0 + 2 * inputcols] = (byte) minmaxclamp (0.0f, (float) input[1 + 0 + 2 * inputcols] + new_g * 3.0f / 32.0f, 255.0f);
							input[1 + 4 + 2 * inputcols] = (byte) minmaxclamp (0.0f, (float) input[1 + 4 + 2 * inputcols] + new_g * 2.0f / 32.0f, 255.0f);

							// current line
							input[2 + 4] = (byte) minmaxclamp (0.0f, (float) input[2 + 4] + new_r * 5.0f / 32.0f, 255.0f);
							input[2 + 8] = (byte) minmaxclamp (0.0f, (float) input[2 + 8] + new_r * 3.0f / 32.0f, 255.0f);
							// next line
							input[2 - 8 + inputcols] = (byte) minmaxclamp (0.0f, (float) input[2 - 8 + inputcols] + new_r * 2.0f / 32.0f, 255.0f);
							input[2 - 4 + inputcols] = (byte) minmaxclamp (0.0f, (float) input[2 - 4 + inputcols] + new_r * 4.0f / 32.0f, 255.0f);
							input[2 + 0 + inputcols] = (byte) minmaxclamp (0.0f, (float) input[2 + 0 + inputcols] + new_r * 5.0f / 32.0f, 255.0f);
							input[2 + 4 + inputcols] = (byte) minmaxclamp (0.0f, (float) input[2 + 4 + inputcols] + new_r * 4.0f / 32.0f, 255.0f);
							input[2 + 8 + inputcols] = (byte) minmaxclamp (0.0f, (float) input[2 + 8 + inputcols] + new_r * 2.0f / 32.0f, 255.0f);
							// after-next line
							input[2 - 4 + 2 * inputcols] = (byte) minmaxclamp (0.0f, (float) input[2 - 4 + 2 * inputcols] + new_r * 2.0f / 32.0f, 255.0f);
							input[2 + 0 + 2 * inputcols] = (byte) minmaxclamp (0.0f, (float) input[2 + 0 + 2 * inputcols] + new_r * 3.0f / 32.0f, 255.0f);
							input[2 + 4 + 2 * inputcols] = (byte) minmaxclamp (0.0f, (float) input[2 + 4 + 2 * inputcols] + new_r * 2.0f / 32.0f, 255.0f);
							break;

							// current line
							input[3 + 4] = (byte) minmaxclamp (0.0f, (float) input[3 + 4] + new_a * 5.0f / 32.0f, 255.0f);
							input[3 + 8] = (byte) minmaxclamp (0.0f, (float) input[3 + 8] + new_a * 3.0f / 32.0f, 255.0f);
							// next line
							input[3 - 8 + inputcols] = (byte) minmaxclamp (0.0f, (float) input[3 - 8 + inputcols] + new_a * 2.0f / 32.0f, 255.0f);
							input[3 - 4 + inputcols] = (byte) minmaxclamp (0.0f, (float) input[3 - 4 + inputcols] + new_a * 4.0f / 32.0f, 255.0f);
							input[3 + 0 + inputcols] = (byte) minmaxclamp (0.0f, (float) input[3 + 0 + inputcols] + new_a * 5.0f / 32.0f, 255.0f);
							input[3 + 4 + inputcols] = (byte) minmaxclamp (0.0f, (float) input[3 + 4 + inputcols] + new_a * 4.0f / 32.0f, 255.0f);
							input[3 + 8 + inputcols] = (byte) minmaxclamp (0.0f, (float) input[3 + 8 + inputcols] + new_a * 2.0f / 32.0f, 255.0f);
							// after-next line
							input[3 - 4 + 2 * inputcols] = (byte) minmaxclamp (0.0f, (float) input[3 - 4 + 2 * inputcols] + new_a * 2.0f / 32.0f, 255.0f);
							input[3 + 0 + 2 * inputcols] = (byte) minmaxclamp (0.0f, (float) input[3 + 0 + 2 * inputcols] + new_a * 3.0f / 32.0f, 255.0f);
							input[3 + 4 + 2 * inputcols] = (byte) minmaxclamp (0.0f, (float) input[3 + 4 + 2 * inputcols] + new_a * 2.0f / 32.0f, 255.0f);
					}
				}

				input += (filedata->pixel_size / 8);
			}
		}
	}
	else if (filedata->image_type == 2 && developer.value == 5) // Uncompressed, RGB images, mankrip dithering
	{
		for(row = rows - 1 ; row >= 0 ; row--)
		{
			pixbuf = output + row * columns;
			for (column = 0 ; column < columns ; column++)
			{
				blue	= input[0];
				green	= input[1];
				red		= input[2];
				alpha = (filedata->pixel_size == 32) ? input[3] : 255;

				*pixbuf++ = packetColor = (alpha == 0) ? 255 : BestColor ( (int)red, (int)green, (int)blue, 0, 254);
				if (row < rows -2)
				{
					new_r = (float) (red	- host_basepal[packetColor * 3    ]);
					new_g = (float) (green	- host_basepal[packetColor * 3 + 1]);
					new_b = (float) (blue	- host_basepal[packetColor * 3 + 2]);
					new_a = (float) (alpha	- ( (packetColor == 255) ? 0 : 255));

					switch (filedata->pixel_size)
					{
						#if 1
						case 24:
							input[0 + 3] = (byte) minmaxclamp (0.0f, (float) input[0 + 3] + new_b, 255.0f);

							input[1 + 3] = (byte) minmaxclamp (0.0f, (float) input[1 + 3] + new_g, 255.0f);

							input[2 + 3] = (byte) minmaxclamp (0.0f, (float) input[2 + 3] + new_r, 255.0f);
							break;
						case 32:
							input[0 + 4] = (byte) minmaxclamp (0.0f, (float) input[0 + 4] + new_b, 255.0f);

							input[1 + 4] = (byte) minmaxclamp (0.0f, (float) input[1 + 4] + new_g, 255.0f);

							input[2 + 4] = (byte) minmaxclamp (0.0f, (float) input[2 + 4] + new_r, 255.0f);

							input[3 + 4] = (byte) minmaxclamp (0.0f, (float) input[3 + 4] + new_a, 255.0f);
							break;
						#else
						case 24:
							input[0 + 3 + inputcols] = (byte) minmaxclamp (0.0f, (float) input[0 + 3 + inputcols] + new_b, 255.0f);

							input[1 + 3 + inputcols] = (byte) minmaxclamp (0.0f, (float) input[1 + 3 + inputcols] + new_g, 255.0f);

							input[2 + 3 + inputcols] = (byte) minmaxclamp (0.0f, (float) input[2 + 3 + inputcols] + new_r, 255.0f);
							break;
						case 32:
							input[0 + 4 + inputcols] = (byte) minmaxclamp (0.0f, (float) input[0 + 4 + inputcols] + new_b, 255.0f);

							input[1 + 4 + inputcols] = (byte) minmaxclamp (0.0f, (float) input[1 + 4 + inputcols] + new_g, 255.0f);

							input[2 + 4 + inputcols] = (byte) minmaxclamp (0.0f, (float) input[2 + 4 + inputcols] + new_r, 255.0f);

							input[3 + 4 + inputcols] = (byte) minmaxclamp (0.0f, (float) input[3 + 4 + inputcols] + new_a, 255.0f);
							break;
						#endif
					}
				}

				input += (filedata->pixel_size / 8);
			}
		}
	}
	else if (filedata->image_type == 2 && developer.value < 0.0f) // Uncompressed, RGB images
	{
		for(row = rows - 1 ; row >= 0 ; row--)
		{
			pixbuf = output + row * columns;
			for (column = 0 ; column < columns ; column++)
			{
				blue	= input[0];
				green	= input[1];
				red		= input[2];
				alpha = (filedata->pixel_size == 32) ? input[3] : 255;

				*pixbuf++ = (alpha == 0) ? 255 : BestColor ( (int)red, (int)green, (int)blue, 0, 254);
				input += (filedata->pixel_size / 8);
			}
		}
	}
	#endif
	else if (filedata->image_type == 10) // Runlength encoded RGB images
	{
		byte
			packetHeader
		,	packetSize
		,	j
			;
		for (row = rows - 1 ; row >= 0 ; row--)
		{
			pixbuf = output + row * columns;
			for (column = 0 ; column < columns ; )
			{
				packetHeader = input[0];
				packetSize = 1 + (packetHeader & 0x7f);
				if (packetHeader & 0x80) // run-length packet
				{
					blue	= input[1];
					green	= input[2];
					red		= input[3];
					alpha = (filedata->pixel_size == 32) ? input[4] : 255;

					packetColor = (alpha == 0) ? 255 : BestColor ( (int)red, (int)green, (int)blue, 0, 254);

					for(j = 0 ; j < packetSize ; j++)
					{
						*pixbuf++ = packetColor;
						column++;
						if (column == columns) // run spans across rows
						{
							column = 0;
							if (row > 0)
								row--;
							else
								goto breakOut;
							pixbuf = output + row * columns;
						}
					}
				}
				else // non run-length packet
				{
					for(j = 0 ; j < packetSize ; j++)
					{
						blue	= input[1];
						green	= input[2];
						red		= input[3];
						alpha = (filedata->pixel_size == 32) ? input[4] : 255;

						*pixbuf++ = (alpha == 0) ? 255 : BestColor ( (int)red, (int)green, (int)blue, 0, 254);
						column++;
						if (column == columns) // pixel packet run spans across rows
						{
							column = 0;
							if (row > 0)
								row--;
							else
								goto breakOut;
							pixbuf = output + row * columns;
						}
					}
				}
			}
			breakOut:;
		}
	}
	else // if (filedata->image_type != 2 && filedata->image_type != 10)
		Sys_Error ("LoadTGA: Only type 2 and 10 targa RGB images supported\n");
//	Hunk_FreeToLowMark (mark);
}

typedef struct
{
    char	manufacturer;
    char	version;
    char	encoding;
    char	bits_per_pixel;
    unsigned short	xmin,ymin,xmax,ymax;
    unsigned short	hres,vres;
    unsigned char	palette[48];
    char	reserved;
    char	color_planes;
    unsigned short	bytes_per_line;
    unsigned short	palette_type;
    char	filler[58];
    unsigned char	data;			// unbounded
} pcx_t;

// mankrip - skyboxes - begin
// Code taken from the ToChriS engine - Author: Vic (vic@quakesrc.org) (http://hkitchen.quakesrc.org/)
/*
============
LoadPCX
============
*/
void LoadPCX (char *filename, byte **pic, int *width, int *height)
{
	pcx_t	*filedata;
	byte	*out, *pix;
	int		x, y;
	int		dataByte, runLength;
	loadedfile_t	*fileinfo; // mankrip

	free (pic);
	*pic = NULL;
	// mankrip - begin
//	pcxbuf = COM_LoadTempFile (filename); // removed
//	if (!pcxbuf) // removed
	fileinfo = COM_LoadTempFile (filename);
	if (!fileinfo)
	// mankrip - end
		return;

//
// parse the PCX file
//
	filedata = (pcx_t *)fileinfo->data; // mankrip
	filedata->xmax = LittleShort (filedata->xmax);
	filedata->xmin = LittleShort (filedata->xmin);
	filedata->ymax = LittleShort (filedata->ymax);
	filedata->ymin = LittleShort (filedata->ymin);
	filedata->hres = LittleShort (filedata->hres);
	filedata->vres = LittleShort (filedata->vres);
	filedata->bytes_per_line = LittleShort (filedata->bytes_per_line);
	filedata->palette_type = LittleShort (filedata->palette_type);

	pix = &filedata->data;

	if (filedata->manufacturer != 0x0a
		|| filedata->version != 5
		|| filedata->encoding != 1
		|| filedata->bits_per_pixel != 8) // mankrip - edited
	//	|| filedata->xmax >= 640 // mankrip - removed
	//	|| filedata->ymax >= 480) // mankrip - removed
	{
		Con_Printf ("Bad pcx file\n");
		return;
	}

	if (width)
		*width = filedata->xmax+1;
	if (height)
		*height = filedata->ymax+1;

	*pic = out = malloc ((filedata->xmax+1) * (filedata->ymax+1));

	for (y=0 ; y<=filedata->ymax ; y++, out += filedata->xmax+1)
	{
		for (x=0 ; x<=filedata->xmax ; )
		{
			dataByte = *pix++;

			if((dataByte & 0xC0) == 0xC0)
			{
				runLength = dataByte & 0x3F;
				dataByte = *pix++;
			}
			else
				runLength = 1;
			RemapBackwards_Pixels ((byte *)&dataByte, 1); // mankrip

			while(runLength-- > 0)
				out[x++] = dataByte;
		}
	}
}
// mankrip - skyboxes - end

/*
==============
WritePCXfile
==============
*/
void WritePCXfile (char *filename, byte *data, int width, int height,
	int rowbytes, byte *palette)
{
	int		i, j, length;
	pcx_t	*pcx;
	byte		*pack;

	pcx = Hunk_TempAlloc (width*height*2+1000);
	if (pcx == NULL)
	{
		Con_Printf("SCR_ScreenShot_f: not enough memory\n");
		return;
	}

	pcx->manufacturer = 0x0a;	// PCX id
	pcx->version = 5;			// 256 color
 	pcx->encoding = 1;		// uncompressed
	pcx->bits_per_pixel = 8;		// 256 color
	pcx->xmin = 0;
	pcx->ymin = 0;
	pcx->xmax = LittleShort((short)(width-1));
	pcx->ymax = LittleShort((short)(height-1));
	pcx->hres = LittleShort((short)width);
	pcx->vres = LittleShort((short)height);
	Q_memset (pcx->palette,0,sizeof(pcx->palette));
	pcx->color_planes = 1;		// chunky image
	pcx->bytes_per_line = LittleShort((short)width);
	pcx->palette_type = LittleShort(2);		// not a grey scale
	Q_memset (pcx->filler,0,sizeof(pcx->filler));

// pack the image
	pack = &pcx->data;

	for (i=0 ; i<height ; i++)
	{
		for (j=0 ; j<width ; j++)
		{
			if ( (*data & 0xc0) == 0xc0)
				*pack++ = 0xc1;
			*pack++ = *data++;
			RemapBackwards_Pixels (pack-1, 1); // mankrip
		}

		data += rowbytes - width;
	}

// write the palette
	*pack++ = 0x0c;	// palette ID byte
	for (i=0 ; i<768 ; i++)
		*pack++ = *palette++;
	RemapBackwards_Palette (pack - 768); // mankrip

// write output file
	length = pack - (byte *)pcx;
	COM_WriteFile (filename, pcx, length);
}
