/*
* libtcod 1.5.1
* Copyright (c) 2008,2009,2010 Jice & Mingos
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*     * Redistributions of source code must retain the above copyright
*       notice, this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright
*       notice, this list of conditions and the following disclaimer in the
*       documentation and/or other materials provided with the distribution.
*     * The name of Jice or Mingos may not be used to endorse or promote products
*       derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY JICE AND MINGOS ``AS IS'' AND ANY
* EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL JICE OR MINGOS BE LIABLE FOR ANY
* DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
* ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifdef __HAIKU__
#include <SDL.h>
#else
#include <SDL/SDL.h>
#endif
#include <png.h>
#include <SDL_image.h>
#include "libtcod.h"
#include "libtcod_int.h"

#define png_infopp_NULL (png_infop*)NULL
#define int_p_NULL (int*)NULL

bool TCOD_sys_check_png(const char *filename) {
	static uint8 magic_number[]={137, 80, 78, 71, 13, 10, 26, 10};
	return TCOD_sys_check_magic_number(filename,sizeof(magic_number),magic_number);
}

SDL_Surface *TCOD_sys_read_png(const char *filename) {
    IMG_Init(IMG_INIT_PNG);
    return IMG_Load(filename);
}

void TCOD_sys_write_png(const SDL_Surface *surf, const char *filename) {
	png_structp png_ptr;
	png_infop info_ptr;
	png_bytep *row_pointers;
	int y,x;
	FILE *fp=fopen(filename,"wb");
	if (!fp) return;

	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING,NULL, NULL, NULL);

	if (png_ptr == NULL)
	{
		fclose(fp);
		return;
	}

	/* Allocate/initialize the memory for image information.  REQUIRED. */
	info_ptr = png_create_info_struct(png_ptr);
	if (info_ptr == NULL)
	{
		fclose(fp);
		png_destroy_write_struct(&png_ptr, png_infopp_NULL);
		return;
	}

	if (setjmp(png_jmpbuf(png_ptr)))
	{
		/* Free all of the memory associated with the png_ptr and info_ptr */
		png_destroy_read_struct(&png_ptr, &info_ptr, png_infopp_NULL);
		fclose(fp);
		/* If we get here, we had a problem reading the file */
		return;
	}

	png_init_io(png_ptr, fp);

	png_set_IHDR(png_ptr,info_ptr,surf->w, surf->h,
		8,PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

	/* get row data */
	row_pointers=(png_bytep *)malloc(sizeof(png_bytep)*surf->h);

	for (y=0; y<  surf->h; y++ ) {
/*		TODO : we should be able to use directly the surface data... */
/*		row_pointers[y]=(png_bytep)(Uint8 *)(surf->pixels) + y * surf->pitch; */
		row_pointers[y]=(png_bytep)malloc(sizeof(png_byte)*surf->w*3);
		for (x=0; x < surf->w; x++ ) {
			Uint8 *pixel=(Uint8 *)(surf->pixels) + y * surf->pitch + x * surf->format->BytesPerPixel;
			row_pointers[y][x*3]=*((pixel)+surf->format->Rshift/8);
			row_pointers[y][x*3+1]=*((pixel)+surf->format->Gshift/8);
			row_pointers[y][x*3+2]=*((pixel)+surf->format->Bshift/8);
		}
	}

	png_set_rows(png_ptr,info_ptr,row_pointers);

	png_write_png(png_ptr,info_ptr,PNG_TRANSFORM_IDENTITY,NULL);

	fclose(fp);
	/* clean up, and free any memory allocated - REQUIRED */
	png_destroy_write_struct(&png_ptr, &info_ptr);

	for (y=0; y<  surf->h; y++ ) {
		free(row_pointers[y]);
	}
	free(row_pointers);
}

