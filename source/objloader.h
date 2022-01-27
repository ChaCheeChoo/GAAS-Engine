#ifndef OBJLOADER
#define OBJLOADER

#include <psptypes.h>
#include <stddef.h>

/**
 * Load OBJ file
 * filename - The file to load
 * usesoffset - flag that tells the loader wheter to load from a specific file or from the PSAR in EBOOT
 *      if latter: filename needs to be "./EBOOT.PBP"
 * fileoffset - location of file in EBOOT, only used when usesoffset==1
 * filesize - size of file in EBOOT, only used when usesoffset==1
 * 
 * returns: obj id
**/
int gaasOBJLoad(const char *filename, int usesoffset, int fileoffset, int filesize);

/**
 * Free's all loaded objs
**/
void gaasOBJFreeAll();

/**
 * Free's a specific obj
 * i - obj id
**/
void gaasOBJFreeSingle(int i);

/**
 * renders loaded obj
 * obj - obj id
 * color - custom color for obj (0xFFFFFFFF)
 * usebb - sets wheter to use bounding boxes or not
 *      bounding boxes are invisible boxes generated around a mesh;
 *      if a bounding box is within the camera's region the mesh will be rendered;
 *      otherwise the mesh won't be rendered;
**/
void gaasOBJRender(int obj, int color, int usebb);

#endif