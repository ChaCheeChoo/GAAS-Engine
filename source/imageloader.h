#ifndef IMAGELOADER
#define IMAGELOADER

typedef unsigned int gaasColor; 

typedef struct {
    int format;
    int tw;             /**< Real texture width. A power of two. */
    int th;             /**< Real texture height. A power of two. */
    int w;              /**< Texture width, as seen when drawing. */
    int h;              /**< Texture height, as seen when drawing. */
    int swizzled;       /**< Is the texture swizzled ? */
    int vram;           /**< Is the texture located in vram ? */
    gaasColor *data;    /**< Pointer to raw data. */
} gaasImage;

typedef struct {
    gaasImage* image[8];
    int levels;
} gaasImageMipmap;

/**
 * Free's loaded image from ram
 * tex - pointer to loaded texture
**/
void gaasIMAGEFree(gaasImage *tex);

/**
 * Free's loaded mipmap image from ram
 * tex - pointer to loaded texture
**/
void gaasIMAGEFreeMipmap(gaasImageMipmap *tex);

/**
 * Loads a texture
 * Textures are recommended to be power^2 in width and height
 * Also grayscale images will look corrupted when loaded, I'm not arsed to fix that, so just set the image to RGB
 * 
 * file - The file to load
 * usesoffset - flag that tells the loader whether to load from a specific file or from the PSAR in EBOOT
 *      if latter: file needs to be "./EBOOT.PBP"
 * offset - location of file in EBOOT, only used when usesoffset==1
 * filesize - size of file in EBOOT, only used when usesoffset==1
 * swizzle - whether to swizzle a texture or not; swizzling speeds up rendering time but isn't needed for 16x16 texture or lower
 * 
 * returns: gaasImage structure
**/
gaasImage* gaasIMAGELoad(const char* file, int swizzle, int usesoffset, int offset, int filesize);

/**
 * Loads a texture from a buffer in memory
 * Textures are recommended to be power^2 in width and height
 * Also grayscale images will look corrupted when loaded, I'm not arsed to fix that, so just set the image to RGB
 * With buffered loading you may need to do some fiddling with the export settings before a texture loads properly
 * 
 * buffer - buffer containing raw png data
 * size - size of buffer
 * swizzle - whether to swizzle a texture or not; swizzling speeds up rendering time but isn't needed for 16x16 texture or lower
 * 
 * returns: gaasImage structure
**/
gaasImage* gaasIMAGELoadFromBuffer(unsigned char *buffer, int size, int swizzle);

/**
 * Loads a texture and generates mipmaps
 * Textures are recommended to be power^2 in width and height
 * Also grayscale images will look corrupted when loaded, I'm not arsed to fix that, so just set the image to RGB
 * 
 * file - The file to load
 * usesoffset - flag that tells the loader whether to load from a specific file or from the PSAR in EBOOT
 *      if latter: file needs to be "./EBOOT.PBP"
 * offset - location of file in EBOOT, only used when usesoffset==1
 * filesize - size of file in EBOOT, only used when usesoffset==1
 * swizzle - whether to swizzle a texture or not; swizzling speeds up rendering time but isn't needed for 16x16 texture or lower
 * mipmaplevels - how many levels to generate
 * 
 * returns: gaasImageMipmap structure
**/
gaasImageMipmap* gaasIMAGELoadImageMipmap(const char* file, int swizzle, int mipmaplevels, int usesoffset, int offset, int filesize);

gaasImageMipmap* gaasIMAGELoadImageMipmapFromBuffer(unsigned char *buffer, int size, int swizzle, int mipmaplevels);

/**
 * Saves texture as png file
 * 
 * filename - The file to save
 * data - pointer to color data to save
 * width - width of texture to save
 * height - height of texture to save
 * lineSize - buffer width of data
 * saveAlpha - flag that determines if image includes alpha data or not
**/
void gaasIMAGESavePNG(const char* filename, gaasColor* data, int width, int height, int lineSize, int saveAlpha);

/**
 * moves given image to vram
 * there's a bug where the first texture you upload to vram will be corrupted
 * currently the only way to mitigate this is by uploading a sacrificial texture that's 64x64 or bigger in size
**/
void gaasIMAGEMoveToVram(gaasImage* source);

/**
 * moves given mipmap to vram
**/
void gaasIMAGEMoveMipmapToVram(gaasImageMipmap* source);

#endif