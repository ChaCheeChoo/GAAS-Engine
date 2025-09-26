/* if something breakes it's almost always the image loader
anytime any part of the engine gets modified in any way there's a solid 5% chance the image loader will break 
I hate png's with a seething passion but aparently no one ever tried shoving a .tga into a gltf file because lmao*/
#include <pspgu.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pspkernel.h>
#include <vram.h>

#include "lodepng/lodepng.h"
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
    tex->vram = 0;

    tex->data = malloc(tex->tw * tex->th * sizeof(gaasColor));
    if (tex->data == NULL) {
        free(tex);
        return NULL;
    }

    memset(tex->data, 0, tex->tw * tex->th * sizeof(gaasColor));

    return tex;
}

static unsigned char* anotherTempBuffer;

//load png from file
gaasImage* LoadPNG(const char* file, int usesoffset, int offset, int filesize) {
    unsigned error;
    FILE* fp;

    gaasImage *tex = NULL;
    unsigned char* tempImage = 0;
    unsigned tempWidth, tempHeight;

    if(usesoffset==0) { //if loading from raw png file
        error = lodepng_decode32_file(&tempImage, &tempWidth, &tempHeight, file);
        if(error) {
            printf("error %u: %s\n", error, lodepng_error_text(error));
        }
    } else { //if loading via GWD
        fp = fopen(file, "rb");
        fseek(fp, offset, SEEK_SET);
        anotherTempBuffer = malloc(filesize);

        fread(anotherTempBuffer, filesize, 1, fp);
        fclose(fp);

        error = lodepng_decode32(&tempImage, &tempWidth, &tempHeight, anotherTempBuffer, filesize);
        if(error) {
            printf("error %u: %s\n", error, lodepng_error_text(error));
        }

        free(anotherTempBuffer);
    }

    tex = ImageCreate(tempWidth, tempHeight);
    memcpy(tex->data, tempImage, tempWidth*tempHeight*4);

    free(tempImage);
    return tex;
}

gaasImage* LoadPNGMemory(unsigned char *buffer, int size) {
    unsigned error;

    gaasImage *tex = NULL;
    unsigned char* tempImage = 0;
    unsigned tempWidth, tempHeight;

    error = lodepng_decode32(&tempImage, &tempWidth, &tempHeight, buffer, size);
    if(error) {
        printf("error %u: %s\n", error, lodepng_error_text(error));
    }

    tex = ImageCreate(tempWidth, tempHeight);
    memcpy(tex->data, tempImage, tempWidth*tempHeight*4);

    free(tempImage);
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

    if(tex->vram==0) {
        free(tex->data);
    } else {
        vfree(tex->data);
    }
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

void gaasIMAGEMoveToVram(gaasImage* source) {
    if (source == NULL) {
        return;
    }

    int texSize = source->tw*source->h*sizeof(gaasColor);
    void* dest = vramalloc(texSize);
    memcpy((void*)((gaasColor)dest|0x40000000), source->data, texSize);
    free(source->data);

    if(dest>0x4200000) {
        printf("image allocated outside vram, making the vram transfer redundent\n");
    }

    printf("VRam used: 0x%x | %u bytes  %u kb\n", dest-0x4000000+texSize, (unsigned int)(dest-0x4000000+texSize), (unsigned int)(dest-0x4000000+texSize)/1000);
    source->vram = 1;
    source->data = dest;
}

void gaasIMAGEMoveMipmapToVram(gaasImageMipmap* source) {
    if (source == NULL) {
        return;
    }

    for(int i=0; i<source->levels; i++) {
        if(source->image[i]!=NULL) {
            gaasIMAGEMoveToVram(source->image[i]);
        }
    }
}

void gaasIMAGEWritePNG(gaasImage* source, const char* out) {
    unsigned error = lodepng_encode32_file(out, (unsigned char*)source->data, source->w, source->h);

    if(error) printf("error %u: %s\n", error, lodepng_error_text(error));
}