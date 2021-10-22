#ifndef DRAWTEXT
#define DRAWTEXT

#include <psptypes.h>
#include <stddef.h>

/**
 * Draws a string on screen (GU)
 * text - The string to draw
 * x - string X position
 * y - string Y position
 * color - string color
 * fw - width between each character
 * 
 * The function, while can be used, is not recommended compared to libraries like intraFont and is mostly used for debugging purposes
**/
void gaasGFXDebugString(const char* text, int x, int y, unsigned int color, int fw);

#endif