#ifndef GRAPHICS
#define GRAPHICS

#include <psptypes.h>
#include <stddef.h>

#include "imageloader.h"

//texture struct used for render targets
typedef struct Texture {
	int format;
	int mipmap;
	int width, height, stride;
	const void* data;
} Texture;

static unsigned int __attribute__((aligned(16))) DisplayList[262144];

void* fbp0;
const void* fbp1;
const void* zbp;

/**
 * initializes GU for 3d and 2d rendering
 * keep in mind you can manually change the settings after initialization
**/
void gaasGFXInit();

/**
 * Draws an animated sprite on screen using a sprite sheet
 * collumns - how many collumns in a sprite sheet
 * rows - how many rows in a sprite sheet
 * width - width of a single sprite in the sheet
 * height - height of a single sprite in the sheet
 * source - sprite sheet texture
 * x - sprite x position
 * y - sprite y position
 * update - Tells the engine how many frames to wait before switching to next sprite: 0=update every frame; 1=wait 1 frame before updating; 2=wait 2 frames...
**/
void gaasGFXAnimatedSpriteAlpha(int collumns, int rows, int width, int height, gaasImage* source, int x, int y, int update);

/**
 * Sets up a texture to put on a 3d model
 * source - texture
 * filter - GU_NEAREST or GU_LINEAR
 * repeat - GU_CLAMP or GU_REPEAT
 * tfx - GU_TFX_...
**/
void gaasGFXTexture(gaasImage* source, int filter, int repeat, int tfx);

/**
 * Sets up a texture to put on a 3d model with mip-mapping
 * source - texture
 * source1 - texture mip 1
 * source2 - texture mip 2
 * source3 - texture mip 3
 * source4 - texture mip 4
 * max - how many mip maps to use
 * filter - GU_NEAREST or GU_LINEAR
 * repeat - GU_CLAMP or GU_REPEAT
 * tfx - GU_TFX_...
 * mode - mip mapping mode GU_TEXTURE_AUTO,GU_TEXTURE_SLOPE or GU_TEXTURE_CONST
 * bias - mip-map bias
**/
void gaasGFXTextureMip(gaasImage* source, gaasImage* source1, gaasImage* source2, gaasImage* source3, gaasImage* source4, int max, int filter, int repeat, int tfx, int mode, float bias);

/**
 * Draws a 2d rectangle
 * x - rectangle x position
 * y - rectangle y position
 * width - rectangle width
 * height - rectangle height
 * color - rectangle color
**/
void gaasGFXFilledRect(int x, int y, int width, int height, unsigned int color);

/**
 * Crops part of a texture and renders that as a sprite
 * startx - X postion of where to start cropping
 * starty - Y postion of where to start cropping
 * width - width of crop from sheet
 * height - height of crop from sheet
 * source - sprite sheet texture
 * x - sprite x position
 * y - sprite y position
**/
void gaasGFXSprite(int startx, int starty, int width, int height, gaasImage* source, int x, int y);

/**
 * Crops part of a texture and renders that as a sprite with tint applied
 * startx - X postion of where to start cropping
 * starty - Y postion of where to start cropping
 * width - width of crop from sheet
 * height - height of crop from sheet
 * source - sprite sheet texture
 * x - sprite x position
 * y - sprite y position
 * tint - tint color
**/
void gaasGFXSpriteTinted(int startx, int starty, int width, int height, gaasImage* source, int x, int y, unsigned int tint);

/**
 * Sets up a scrolling texture to put on a 3d model
 * source - texture
 * scrollx - how much to scroll on the X axis
 * scrolly - how much to scroll on the Y axis
 * maxscroll - how far either scroll value can go before getting reset to 0
 * filter - GU_NEAREST or GU_LINEAR
 * repeat - GU_CLAMP or GU_REPEAT
 * tfx - GU_TFX_...
**/
void gaasGFXTextureScroller(gaasImage* source, float scrollx, float scrolly, float maxscroll, int filter, int repeat, int tfx);

/**
 * Use a texture generated from render target as sprite
 * texture - texture generated from render target
 * width - width of crop from texture
 * height - height of crop from texture
 * x - sprite x position
 * y - sprite y position
**/
void gaasGFXRenderTargetSprite(Texture* texture, int x, int y, int width, int height);

/**
 * Sets up a texture generated from render target to put on a 3d model
 * texture - texture generated from render target
 * filter - GU_NEAREST or GU_LINEAR
 * repeat - GU_CLAMP or GU_REPEAT
 * tfx - GU_TFX_...
**/
void gaasGFXRenderTargetTexture(Texture* texture, int filter, int repeat, int tfx);

/**
 * Draws billboard (2d texture rendered in 3d space that's always facing the camera)
 * pos - Position of billboard in 3d space
 * source - valid gaasImage struct
 * scale - size of billboard
 * color - billboard color
 * viewMatrix - matrix that will be used to calculate billboard rotation, to obtain this matrix, use sceGumStoreMatrix(&viewMatrix) after setting up the camera in GU
 *      Example:
 *          sceGumMatrixMode(GU_VIEW);
 *          {
 *          	ScePspFVector3 eye = {5, 5, 5};
 *          	ScePspFVector3 center = {0, 0, 0};
 *          	ScePspFVector3 up = {0, 1, 0};
 *  
 *          	sceGumLoadIdentity();
 *          	sceGumLookAt(&eye, &center, &up);
 *          	sceGumStoreMatrix(&viewMatrix);
 *          }
**/
void gaasGFXBillboard(ScePspFVector3 pos, gaasImage* source, float scale, unsigned int color, ScePspFMatrix4 viewMatrix);

/**
 * Draws animated billboard (2d texture rendered in 3d space that's always facing the camera, but animated)
 * pos - Position of billboard in 3d space
 * source - valid gaasImage struct
 * collumns - how many collumns in a sprite sheet
 * rows - how many rows in a sprite sheet
 * scale - size of billboard
 * color - billboard color
 * viewMatrix - matrix that will be used to calculate billboard rotation, instructions to obtain this matrix can be found above
 * update - Tells the engine how many frames to wait before switching to next sprite: 0=update every frame; 1=wait 1 frame before updating; 2=wait 2 frames...
**/
void gaasGFXAnimatedBillboard(ScePspFVector3 pos, gaasImage* source, int collumns, int rows, float scale, unsigned int color, ScePspFMatrix4 viewMatrix, float update);

#endif