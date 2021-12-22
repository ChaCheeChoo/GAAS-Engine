#ifndef BILLBOARD
#define BILLBOARD

#include <psptypes.h>
#include <stddef.h>
#include "imageloader.h"

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