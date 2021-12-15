#ifndef IMAGELOADER
#define IMAGELOADER

typedef unsigned int gaasColor;

typedef struct {
    int format;
    int tw;             /**< Real texture width. A power of two. */
    int th;             /**< Real texture height. A power of two. */
    int w;              /**< Texture width, as seen when drawing. */
    int h;              /**< Texture height, as seen when drawing. */
    int swizzled;      /**< Is the texture swizzled ? */
    gaasColor *data;     /**< Pointer to raw data. */
} gaasImage;

/**
 * Free's loaded image from ram
 * tex - pointer to loaded texture
**/
void gaasIMAGEFree(gaasImage *tex);

/**
 * Loads a texture
 * Texture are recommended to be power^2 in width and height
 * Also grayscale images will look corrupted when loaded, I'm not arsed to fix that, so just set the image to RGB
 * 
 * filename - The file to load
 * usesoffset - flag that tells the loader wether to load from a specific file or from the PSAR in EBOOT
 *      if latter: filename needs to be "./EBOOT.PBP"
 * offset - location of file in EBOOT, only used when usesoffset==1
 * filesize - size of file in EBOOT, only used when usesoffset==1
 * swizzle - wether to swizzle a texture or not; swizzling speeds up rendering time but isn't needed for 16x16 texture or lower
 * 
 * returns: gaasImage structure
**/
gaasImage* gaasIMAGELoad(const char* file, int swizzle, int usesoffset, int offset, int filesize);

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

#endif