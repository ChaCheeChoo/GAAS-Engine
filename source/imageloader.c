#include <pspgu.h>
#include <stdio.h>
#include <png.h>
#include <stdlib.h>
#include <string.h>
#include <pspkernel.h>

#include "imageloader.h"

//internal functions
unsigned int GetNextPower2(unsigned int n) {
    n--;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;

    return n+1;
}

//black magic fuckery
void Swizzle(unsigned char *dest, unsigned char *source, int width, int height) {
    int i, j;
    int rowblocks = (width / 16);
    int rowblocks_add = (rowblocks-1) * 128;
    unsigned int block_address = 0;
    unsigned int *img = (unsigned int*)source;

    for (j=0; j<height; j++)
    {
        unsigned int *block = (unsigned int*)(dest + block_address);

        for (i = 0; i < rowblocks; i++)
        {
            *block++ = *img++;
            *block++ = *img++;
            *block++ = *img++;
            *block++ = *img++;

            block += 28;
        }

        if ((j & 0x7) == 0x7)
            block_address += rowblocks_add;

        block_address += 16;
    }
}

gaasImage* ImageCreate(int w, int h) {
    gaasImage *tex = malloc(sizeof(gaasImage));
    if (tex == NULL) {
        return NULL;
    }

    tex->format = GU_PSM_8888;
    tex->tw = GetNextPower2(w);
    tex->th = GetNextPower2(h);
    tex->w = w;
    tex->h = h;
    tex->swizzled = 0;

    tex->data = malloc(tex->tw * tex->th * sizeof(gaasColor));
    if (tex->data == NULL)
    {
        free(tex);
        return NULL;
    }

    memset(tex->data, 0, tex->tw * tex->th * sizeof(gaasColor));

    return tex;
}

//load png from file
gaasImage* LoadPNG(const char* file, int usesoffset, int offset, int filesize) {
    FILE* fp;

    fp = fopen(file, "rb");

    if(!fp) {
        printf("failed to load png\n");
        return NULL;
    }
 
    if(usesoffset==1) {
        fseek(fp, offset, SEEK_SET);
    }

    png_structp png_ptr;
    png_infop info_ptr;
    unsigned int sig_read = 0;
    png_uint_32 width, height;
    int bit_depth, color_type, interlace_type;
    u32 x, y;
    gaasColor *line;
    gaasImage *tex;

    png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    png_set_error_fn(png_ptr, NULL, NULL, NULL);
    info_ptr = png_create_info_struct(png_ptr);

    png_init_io(png_ptr, fp); //load from file

    png_set_sig_bytes(png_ptr, sig_read);
    png_read_info(png_ptr, info_ptr);
    png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type, &interlace_type, NULL, NULL);
    png_set_strip_16(png_ptr);
    png_set_packing(png_ptr);

    if (color_type == PNG_COLOR_TYPE_PALETTE) {
        png_set_palette_to_rgb(png_ptr);
    }

    if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)) {
        png_set_tRNS_to_alpha(png_ptr);
    }

    png_set_filler(png_ptr, 0xff, PNG_FILLER_AFTER);
    
    tex = ImageCreate(width, height);
    line = malloc(width * 4);

    for (y = 0; y < height; y++) {
        png_read_row(png_ptr, (u8*) line, NULL);
        
        for (x = 0; x < width; x++) {
            tex->data[x + y*tex->tw] = line[x];
        }
    }

    free(line);
    png_read_end(png_ptr, info_ptr);
    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);

    fclose(fp);

    return tex;
}

static char* anotherTempBuffer;
static int readOffset = 0;

void readMemCallback(png_structp png_ptr, png_bytep destination, png_size_t bytesToRead) {
    memcpy((unsigned char*)destination, (char*)&anotherTempBuffer[readOffset], bytesToRead);
    //printf("fuck 1  %d %d\n", bytesToRead, readOffset);
    readOffset+=bytesToRead;
}

gaasImage* LoadPNGMemory(unsigned char *buffer, int size) {
    const int PNG_SIG_BYTES = 8;
    readOffset=PNG_SIG_BYTES;
    char pngSignature[PNG_SIG_BYTES];
    memcpy(pngSignature, buffer, PNG_SIG_BYTES * sizeof(char));

    anotherTempBuffer=(char*)malloc(size);
    memcpy(anotherTempBuffer, buffer, size);
    
    if(!png_check_sig((png_bytep)pngSignature, PNG_SIG_BYTES)){
        printf("png signature is fucked\n");
        return NULL;
    }

    png_structp png_ptr;
    png_infop info_ptr;
    png_bytep* row_pointers = NULL;
    unsigned int sig_read = 0;
    png_uint_32 width, height;
    int interlace_type, bit_depth, color_type;
    u32 x, y;
    gaasColor *line;
    gaasImage *tex;

    png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (png_ptr == NULL) {
        printf("png_ptr fucked\n");
        return NULL;
    }
    png_set_error_fn(png_ptr, NULL, NULL, NULL);
    info_ptr = png_create_info_struct(png_ptr);
    if (info_ptr == NULL) {
        png_destroy_read_struct(&png_ptr, NULL, NULL);
        printf("info_ptr fucked\n");
        return NULL;
    }

    if (setjmp(png_jmpbuf(png_ptr))) {
        printf("Image loader fucked\n");
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        return NULL;
    }

    png_set_read_fn(png_ptr, (void*)&buffer, readMemCallback);//load from memory
    png_set_sig_bytes(png_ptr, 8); 
    png_read_info(png_ptr, info_ptr);
    png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type, &interlace_type, NULL, NULL);
    png_set_strip_16(png_ptr);
    png_set_packing(png_ptr);

    if (color_type == PNG_COLOR_TYPE_PALETTE) {
        png_set_palette_to_rgb(png_ptr);
    }

    if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)) {
        png_set_tRNS_to_alpha(png_ptr);
    }

    png_set_filler(png_ptr, 0xff, PNG_FILLER_AFTER);
    
    tex = ImageCreate(width, height);
    line = malloc(width * 4);

    for (y = 0; y < height; y++) {
        png_read_row(png_ptr, (u8*) line, NULL);
        
        for (x = 0; x < width; x++) {
            tex->data[x + y*tex->tw] = line[x];
        }
    }

    free(line);
    free(anotherTempBuffer);
    png_read_end(png_ptr, info_ptr);
    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);

    return tex;
}

gaasImage* DownscaleImage(gaasImage *tex) {
    gaasImage *temp = NULL;
    int width = tex->w/2;
    int height = tex->h/2;
    temp = ImageCreate(width, height);

    for (int y = 0; y<height; y++) {  
        temp->data[y*temp->tw] = tex->data[y*2*tex->tw];
        for (int x = 0; x<width; x++) {
            temp->data[x+y*temp->tw] = tex->data[x*2+y*2*tex->tw];
        }
    }
    
    return temp;
}

//external functions
void gaasIMAGEFree(gaasImage *tex) {
    if (tex == NULL) {
        return;
    }

    free(tex->data);
    free(tex);

    tex = NULL;
}

void gaasIMAGEFreeMipmap(gaasImageMipmap *tex) {
    if (tex == NULL) {
        return;
    }

    for(int i=0; i<tex->levels; i++) {
        if(tex->image[i]!=NULL) {
            gaasIMAGEFree(tex->image[i]);
        }
    }

    free(tex);
    tex = NULL;
}

gaasImage* gaasIMAGELoad(const char* file, int swizzle, int usesoffset, int offset, int filesize) {
    gaasImage *tex = NULL;

    tex = LoadPNG(file, usesoffset, offset, filesize);

    if (tex == NULL) {
        printf("png loader returned NULL\n");
        gaasIMAGEFree(tex);
        return NULL;
    }

    if (tex->w > 512 || tex->h > 512) {
        printf("image too big\n");
        gaasIMAGEFree(tex);
        return NULL;
    }

    if (swizzle==1 && (tex->w >= 16 || tex->h >= 16)) {
        u8 *tmp = malloc(tex->tw*tex->th*4);
        Swizzle(tmp, (u8*)tex->data, tex->tw*4, tex->th);
        free(tex->data);
        tex->data = (gaasColor*)tmp;
        tex->swizzled = 1;
    } else {
        tex->swizzled = 0;
    }

    sceKernelDcacheWritebackRange(tex->data, tex->tw*tex->th*4);

    return tex;
}

gaasImage* gaasIMAGELoadFromBuffer(unsigned char *buffer, int size, int swizzle) {
    gaasImage *tex = NULL;

    tex = LoadPNGMemory(buffer, size);

    if (tex == NULL) {
        printf("png loader returned NULL\n");
        gaasIMAGEFree(tex);
        return NULL;
    }

    if (tex->w > 512 || tex->h > 512) {
        printf("image too big\n");
        gaasIMAGEFree(tex);
        return NULL;
    }

    if (swizzle==1 && (tex->w >= 16 || tex->h >= 16)) {
        u8 *tmp = malloc(tex->tw*tex->th*4);
        Swizzle(tmp, (u8*)tex->data, tex->tw*4, tex->th);
        free(tex->data);
        tex->data = (gaasColor*)tmp;
        tex->swizzled = 1;
    } else {
        tex->swizzled = 0;
    }

    sceKernelDcacheWritebackRange(tex->data, tex->tw*tex->th*4);

    return tex;
}

gaasImageMipmap* gaasIMAGELoadImageMipmap(const char* file, int swizzle, int mipmaplevels, int usesoffset, int offset, int filesize) {
    gaasImageMipmap *tex = NULL;
    tex = malloc(sizeof(gaasImageMipmap));

    tex->levels = mipmaplevels+1;
    tex->image[0] = LoadPNG(file, usesoffset, offset, filesize);

    if (tex->image[0] == NULL) {
        printf("png loader returned NULL\n");
        gaasIMAGEFreeMipmap(tex);
        return NULL;
    }

    if (tex->image[0]->w > 512 || tex->image[0]->h > 512) {
        printf("image too big\n");
        gaasIMAGEFreeMipmap(tex);
        return NULL;
    }

    for(int i=0; i<mipmaplevels; i++) {
        tex->image[i+1] = DownscaleImage(tex->image[i]);
    }

    if (swizzle==1 && (tex->image[0]->w >= 16 || tex->image[0]->h >= 16)) {
        for(int i=0; i<mipmaplevels+1; i++) {
            u8 *tmp = malloc(tex->image[i]->tw*tex->image[i]->th*4);
            Swizzle(tmp, (u8*)tex->image[i]->data, tex->image[i]->tw*4, tex->image[i]->th);
            free(tex->image[i]->data);
            tex->image[i]->data = (gaasColor*)tmp;
            tex->image[i]->swizzled = 1;
        }
    } else {
        for(int i=0; i<mipmaplevels+1; i++) {
            tex->image[i]->swizzled = 0;
        }
    }

    for(int i=0; i<mipmaplevels+1; i++) {
        sceKernelDcacheWritebackRange(tex->image[i]->data, tex->image[i]->tw*tex->image[i]->th*4);
    }

    return tex;
}

gaasImageMipmap* gaasIMAGELoadImageMipmapFromBuffer(unsigned char *buffer, int size, int swizzle, int mipmaplevels) {
    gaasImageMipmap *tex = NULL;
    tex = malloc(sizeof(gaasImageMipmap));

    tex->levels = mipmaplevels+1;
    tex->image[0] = LoadPNGMemory(buffer, size);

    if (tex->image[0] == NULL) {
        printf("png loader returned NULL\n");
        gaasIMAGEFreeMipmap(tex);
        return NULL;
    }

    if (tex->image[0]->w > 512 || tex->image[0]->h > 512) {
        printf("image too big\n");
        gaasIMAGEFreeMipmap(tex);
        return NULL;
    }

    for(int i=0; i<mipmaplevels; i++) {
        tex->image[i+1] = DownscaleImage(tex->image[i]);
    }

    if (swizzle==1 && (tex->image[0]->w >= 16 || tex->image[0]->h >= 16)) {
        for(int i=0; i<mipmaplevels+1; i++) {
            u8 *tmp = malloc(tex->image[i]->tw*tex->image[i]->th*4);
            Swizzle(tmp, (u8*)tex->image[i]->data, tex->image[i]->tw*4, tex->image[i]->th);
            free(tex->image[i]->data);
            tex->image[i]->data = (gaasColor*)tmp;
            tex->image[i]->swizzled = 1;
        }
    } else {
        for(int i=0; i<mipmaplevels+1; i++) {
            tex->image[i]->swizzled = 0;
        }
    }

    for(int i=0; i<mipmaplevels+1; i++) {
        sceKernelDcacheWritebackRange(tex->image[i]->data, tex->image[i]->tw*tex->image[i]->th*4);
    }

    return tex;
}

void gaasIMAGESavePNG(const char* filename, gaasColor* data, int width, int height, int lineSize, int saveAlpha) {
	png_structp png_ptr;
	png_infop info_ptr;
	FILE* fp;
	int i, x, y;
	unsigned char* line;

    fp = fopen(filename, "wb");

	if (fp == NULL) {
        return;
    }

	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!png_ptr) {
        return;
    }

	info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr) {
		png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
		return;
	}
	png_init_io(png_ptr, fp);
	png_set_IHDR(png_ptr, info_ptr, width, height, 8, saveAlpha ? PNG_COLOR_TYPE_RGBA : PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
	png_write_info(png_ptr, info_ptr);
	line = (unsigned char *) malloc(width * (saveAlpha ? 4 : 3));
	for (y = 0; y < height; y++) {
		for (i = 0, x = 0; x < width; x++) {
			gaasColor color = data[x + y * lineSize];
			unsigned char r = color & 0xff;
			unsigned char g = (color >> 8) & 0xff;
			unsigned char b = (color >> 16) & 0xff;
			unsigned char a = saveAlpha ? (color >> 24) & 0xff : 0xff;
			line[i++] = r;
			line[i++] = g;
			line[i++] = b;
			if (saveAlpha) line[i++] = a;
		}
		png_write_row(png_ptr, line);
	}
	free(line);
	png_write_end(png_ptr, info_ptr);
	png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
	fclose(fp);
}